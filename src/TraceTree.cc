#include "TraceTree.hh"
#include "Flow.hh"
#include "Match.hh"
#include "FluidDump.hh"

static const uint64_t flowCookieBase = 0x100000000UL;
static const uint64_t flowCookieMask = 0xffffffff00000000UL;
of13::InstructionSet toController;

static struct InitToController {
    InitToController() {
        of13::OutputAction outputController(of13::OFPP_CONTROLLER, 128);
        of13::ApplyActions applyActions;
        applyActions.add_action(outputController);
        toController.add_instruction(applyActions);
    }
} initToController;

TraceTree::TraceTree()
{  }

bool TraceTree::isTableMiss(of13::PacketIn& pi)
{
    if (pi.reason() == of13::OFPR_NO_MATCH)
        return true;
    if (pi.reason() == of13::OFPR_ACTION && pi.cookie() == flowCookieBase)
        return true;
    return false;
}

void TraceTree::cleanFlowTable(OFConnection* ofconn)
{
    of13::FlowMod fm;
    fm.cookie(flowCookieBase);
    fm.cookie_mask(flowCookieMask);
    fm.command(of13::OFPFC_DELETE);
    fm.out_port(of13::OFPP_ANY);
    fm.out_group(of13::OFPG_ANY);

    uint8_t* buf = fm.pack();
    ofconn->send(buf, fm.length());
    OFMsg::free_buffer(buf);
}

struct BuildFTContext {
    uint64_t cookie;
    OFConnection *ofconn;
    std::vector<of13::OXMTLV*> match;
    uint16_t priority;

    uint64_t emitRule(Flow* flow, of13::FlowMod* fm);
    uint64_t emitBarrier();
    void buildFlowTableStep(TraceTreeNode* t);
};

unsigned TraceTree::buildFlowTable(OFConnection* ofconn)
{
    BuildFTContext ctx;
    ctx.ofconn = ofconn;
    ctx.priority = 0;
    ctx.cookie = flowCookieBase;

    ctx.buildFlowTableStep(&root);
    DCHECK_EQ(ctx.match.size(), 0);

    return (unsigned) (ctx.cookie - flowCookieBase);
}

uint64_t BuildFTContext::emitRule(Flow* flow, of13::FlowMod* fm)
{
    fm->cookie(++cookie);
    fm->priority(priority);

    of13::Match m;
    for (of13::OXMTLV* tlv : match)
        m.add_oxm_field(tlv->clone());
    fm->match(m);

    uint8_t* buf = fm->pack();
    ofconn->send(buf, fm->length());
    OFMsg::free_buffer(buf);

    return cookie;
}

uint64_t BuildFTContext::emitBarrier()
{
    DVLOG(10) << "Emitting barrier";
    of13::FlowMod fm;
    of13::Match m;
    for (of13::OXMTLV* tlv : match)
        m.add_oxm_field(tlv->clone());
    fm.match(m);

    fm.command(of13::OFPFC_ADD);
    fm.priority(priority);
    fm.cookie(flowCookieBase);
    fm.instructions(toController);

    uint8_t* buf = fm.pack();
    ofconn->send(buf, fm.length());
    OFMsg::free_buffer(buf);

    return flowCookieBase;
}

void BuildFTContext::buildFlowTableStep(TraceTreeNode* t)
{
    switch (t->type()) {
    case TraceTreeNode::Empty:
        break;
    case TraceTreeNode::Leaf: {
        Flow* flow = t->leaf.flow;
        of13::FlowMod* fm = t->leaf.fm;

        emitRule(flow, fm);
        priority += 1;
        break;
    }
    case TraceTreeNode::Load:
        for (TraceTreeNode::LoadData* l = &t->load; l; l = l->next) {
            match.push_back(l->value);
            buildFlowTableStep(l->child);
            match.pop_back();
        }
        break;
    case TraceTreeNode::Test:
        buildFlowTableStep(t->test.negativeChild);

        // Barrier
        match.push_back(t->test.value);
        emitBarrier();
        priority += 1;

        buildFlowTableStep(t->test.positiveChild);
        match.pop_back();

        break;
    }
}

TraceTreeNode::Type TraceTreeNode::type()
{
    if (m_type == Leaf && leaf.flow->outdated()) {
        assert(leaf.flow->state() != Flow::New);
        makeEmpty();
    }
    return m_type;
}

void TraceTreeNode::makeEmpty()
{
    switch (m_type) {
    case Empty:
        break;
    case Test:
        delete test.value;
        delete test.negativeChild;
        delete test.positiveChild;
        break;
    case Load:
        delete load.value;
        for (LoadData *l = load.next, *p = nullptr;
             l != nullptr;
             l = l->next, delete p->value, delete p, p = l);
        break;
    case Leaf:
        leaf.flow->deleteLater();
        delete leaf.fm;
        break;
    }

    m_type = Empty;
}

void TraceTreeNode::makeTest(of13::OXMTLV *value)
{
    CHECK_EQ(m_type, Empty);
    test.positiveChild = new TraceTreeNode();
    test.negativeChild = new TraceTreeNode();
    test.value = value->clone();
    m_type = Test;
}

void TraceTreeNode::makeLoad(of13::OXMTLV* value)
{
    CHECK_EQ(m_type, Empty);
    load.next = nullptr;
    load.child = new TraceTreeNode();
    load.value = value->clone();
    m_type = Load;
}

void TraceTreeNode::makeLeaf(Flow *flow, of13::FlowMod* fm_base)
{
    CHECK_EQ(m_type, Empty);
    leaf.flow = flow;
    leaf.fm = fm_base;
    m_type = Leaf;
}

TraceTreeNode* TraceTreeNode::move(TraceEntry &op)
{
    switch (type()) {
    case Test: {
        DVLOG(10) << "moving by test branch";
        CHECK_EQ(op.type, TraceEntry::Test);
        CHECK_EQ(test.value->field(), op.tlv->field());
        return op.outcome ? test.positiveChild : test.negativeChild;
    }
    case Load: {
        DVLOG(10) << "moving by load branch";
        CHECK_EQ(op.type, TraceEntry::Load);
        CHECK_EQ(load.value->field(), op.tlv->field());

        // Add new element to the list
        LoadData *l, *p;
        for (l = &load, p = nullptr;
             l != nullptr;
             p = l, l = l->next)
        {
            if (l->value->equals(*op.tlv))
                return l->child;
        }

        if (l == nullptr) {
            l = p->next = new LoadData();
            l->value = op.tlv->clone();
            return l->child = new TraceTreeNode();
        }
    }
    default:
        return nullptr;
    }
}

void TraceTree::augment(Flow* flow, of13::FlowMod* fm_base)
{
    TraceTreeNode* t = &root;

    for (auto& op : flow->trace()) {
        DVLOG(10) << "augment step, item type = " << op.type << " " << ::dump(op.tlv);

        if (t->type() == TraceTreeNode::Empty) {
            DVLOG(10) << "appending this item to the tree";

            if (op.type == TraceEntry::Test) {
                t->makeTest(op.tlv);
            } else if (op.type == TraceEntry::Load) {
                t->makeLoad(op.tlv);
            }
        }

        t = t->move(op);
    }

    CHECK(t->type() == TraceTreeNode::Empty);
    t->makeLeaf(flow, fm_base);
}

std::ostream& TraceTree::dump(std::ostream& out)
{
    return root.dump(out, 0);
}

std::ostream& TraceTreeNode::dump(std::ostream& out, size_t level)
{
    std::string indent(level * 2, ' ');

    switch (m_type) {
    case Leaf:
        out << indent << "Leaf " << leaf.flow << std::endl;
        break;
    case Test:
        out << indent << "Test " << ::dump(test.value) << std::endl;
        test.positiveChild->dump(out, level + 1);
        test.negativeChild->dump(out, level + 1);
        break;
    case Load:
        for (LoadData* l = &load; l != nullptr; l = l->next) {
            out << indent << "Load " << ::dump(l->value) << std::endl;
            l->child->dump(out, level + 1);
        }
    case Empty:
        break;
    }

    return out;
}

TraceTreeNode::LeafData* TraceTree::find(Packet* pkt)
{
    return root.find(pkt);
}

TraceTreeNode::LeafData* TraceTreeNode::find(Packet* pkt)
{
    switch (type()) {
    case Leaf:
        return &leaf;
    case Test: {
        OXMTLVUnion data(test.value->field());
        pkt->read(data);

        bool res = oxm_match(test.value, data.base());
        TraceTreeNode* child = res ? test.positiveChild : test.negativeChild;
        if (child) return child->find(pkt);
    }
    case Load: {
        OXMTLVUnion data(load.value->field());
        pkt->read(data);

        for (LoadData* l = &load; l != nullptr; l = l->next) {
            if (oxm_match(l->value, data.base()))
                return l->child->find(pkt);
        }

        return nullptr;
    }
    default:
        return nullptr;
    }
}

TraceTreeNode::TraceTreeNode()
    : m_type(Empty)
{ }

TraceTreeNode::~TraceTreeNode()
{
    makeEmpty();
}
