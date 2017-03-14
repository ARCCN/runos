/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Maple.hh"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <sstream>
#include <memory>
#include <functional>

#include <boost/assert.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/get.hpp>

#include "maple/Runtime.hh"
#include "oxm/field_set.hh"
#include "types/exception.hh"

#include "Controller.hh"
#include "OFMsgUnion.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"
#include "PacketParser.hh"
#include "FluidOXMAdapter.hh"


REGISTER_APPLICATION(Maple, {"controller", ""})

using namespace runos;
using namespace std::placeholders;

typedef std::vector< std::pair<std::string, PacketMissHandler> >
    PacketMissPipeline;
typedef std::vector< std::pair<std::string, PacketMissHandlerFactory> >
    PacketMissPipelineFactory;

typedef boost::error_info< struct tag_pi_handler, std::string >
    errinfo_packetin_handler;



struct ModTrackingPacket final : public PacketProxy {
    oxm::field_set m_mods;
public:
    explicit ModTrackingPacket(Packet& pkt)
        : PacketProxy(pkt)
    { }

    void modify(const oxm::field<> patch) override
    {
        pkt.modify(patch);
        m_mods.modify(patch);
    }

    const oxm::field_set& mods() const
    { return m_mods; }
};

// Enable protected methods
struct DecisionImpl : Decision
{
    DecisionImpl() = default;
    const Base& base() const
    { return Decision::base(); }

    const DecisionData& data() const
    { return m_data; }
};

struct unhandled_packet: runtime_error
{
};

class FlowImpl final : public Flow
                     , public maple::Flow
{
    SwitchConnectionPtr conn;

    uint8_t m_table{0};
    uint32_t m_in_port;
    Decision m_decision {DecisionImpl()};
    oxm::field_set m_match;
    oxm::field_set m_mods;

    bool m_packet_in {false};
    uint32_t m_xid {0};
    uint32_t m_buffer_id {OFP_NO_BUFFER};
    class DecisionCompiler : public boost::static_visitor<void> {
        ActionList& ret;
        uint64_t dpid;
    public:
        explicit DecisionCompiler(ActionList& ret, uint64_t dpid)
            : ret(ret), dpid(dpid)
        { }

        void operator()(const Decision::Undefined&) const
        {
            RUNOS_THROW(unhandled_packet());
        }

        void operator()(const Decision::Drop&) const
        { }

        void operator()(const Decision::Unicast& u) const
        {
            ret.add_action(new of13::OutputAction(u.port, 0));
        }

        void operator()(const Decision::Multicast& m) const
        {
            for (uint32_t port : m.ports) {
                ret.add_action(new of13::OutputAction(port, 0));
            }
        }

        void operator()(const Decision::Broadcast& b) const
        {
            ret.add_action(new of13::OutputAction(of13::OFPP_FLOOD, 0));
        }

        void operator()(const Decision::Inspect& i) const
        {
            DVLOG(30) << "Added rule inpecting. Len : " << int(i.send_bytes_len);
            ret.add_action(new of13::OutputAction(of13::OFPP_CONTROLLER,
                                                  i.send_bytes_len));
        }

        void operator()(const Decision::Custom& c) const
        {
            c.body->apply(ret, dpid);
        }
   };

    ActionList actions(uint64_t dpid) const
    {
        ActionList ret;

        for (const oxm::field<>& f : m_mods) {
            ret.add_action(new of13::SetFieldAction(new FluidOXMAdapter(f)));
        }

        boost::apply_visitor(DecisionCompiler(ret, dpid), m_decision.data());
        return ret;
    }

    void install()
    {
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        if (state() == State::Evicted && not m_packet_in)
            return;

        if (m_decision.idle_timeout() <= Decision::duration::zero()) {
            if (m_packet_in) {
                of13::PacketOut po;
                po.xid(m_xid);
                po.buffer_id(m_buffer_id);
                po.actions(actions(conn->dpid()));
                po.in_port(m_in_port);

                conn->send(po);
                m_state = State::Evicted; // Flow is evicted to controller
            }
        } else {
            of13::FlowMod fm;

            fm.command(of13::OFPFC_ADD);
            fm.xid(m_xid);
            fm.buffer_id(m_buffer_id);

            fm.table_id(m_table);
            fm.priority(m_priority);
            fm.cookie(cookie());
            fm.match(make_of_match(m_match));

            auto ito = m_decision.idle_timeout();
            auto hto = m_decision.hard_timeout();
            long long ito_seconds = duration_cast<seconds>(ito).count();
            long long hto_seconds = duration_cast<seconds>(hto).count();

            if (ito == Decision::duration::max())
                fm.idle_timeout(0);
            else
                fm.idle_timeout(std::min(ito_seconds, 65535LL));

            if (hto == Decision::duration::max())
                fm.hard_timeout(0);
            else
                fm.hard_timeout(std::min(hto_seconds, 65535LL));

            fm.flags( of13::OFPFF_CHECK_OVERLAP |
                      of13::OFPFF_SEND_FLOW_REM );

            of13::ApplyActions applyActions;
            applyActions.actions(actions(conn->dpid()));
            fm.add_instruction(applyActions);

            conn->send(fm);
            m_state = State::Active; //Flow is installed
        }

        m_packet_in = false;
        m_xid = 0;
        m_buffer_id = OFP_NO_BUFFER;
        m_in_port = of13::OFPP_CONTROLLER;
    }

public:
    explicit FlowImpl(SwitchConnectionPtr conn, uint8_t table)
        : conn(conn), m_table(table)
    { }

    void mods(oxm::field_set mod)
    {
        BOOST_ASSERT(state() != State::Active);
        m_mods = std::move(mod);
    }

    void decision(Decision d)
    {
        //BOOST_ASSERT(state() != State::Active);
        m_decision = std::move(d);
    }

    maple::Flow& operator=(const maple::Flow& other_) override
    {
        const FlowImpl& other = dynamic_cast<const FlowImpl&>(other_);
        return *this = other;
    }

    // Transitions
    void packet_in(of13::PacketIn& pi)
    {
        //BOOST_ASSERT(state() == State::Egg ||
        //             state() == State::Evicted ||
        //             state() == State::Idle ||
        //             (state() == State::Active &&
        //                boost::get<Decision::Inspect>(&m_decision.data()))
        //           );

        m_packet_in = true;
        m_xid = pi.xid();
        m_buffer_id = pi.buffer_id();
        m_in_port = pi.match().in_port()->value();
    }

    void flow_removed(of13::FlowRemoved& fr)
    {
        switch (fr.reason()) {
        case of13::OFPRR_DELETE:
        case of13::OFPRR_METER_DELETE:
            m_state = State::Evicted;
            VLOG(30) << "Evicted Flow";
            break;
        case of13::OFPRR_IDLE_TIMEOUT:
            m_state = State::Idle;
            VLOG(30) << "Deleted flow by idle timeout";
            break;
        case of13::OFPRR_HARD_TIMEOUT:
            m_state = State::Expired;
            VLOG(30) << "Deted flow by hard timeout";
            break;
        }
    }

    void packet_out(uint16_t priority, oxm::field_set match)
    {
        m_priority = priority;
        m_match = std::move(match);
        install();
    }

    void wakeup()
    {
       // BOOST_ASSERT( state() == State::Idle
       //     || state() == State::Evicted
       //     || disposable() );
        install();
    }
    bool disposable(){
        return m_decision.idle_timeout() <= Decision::duration::zero();
    }
    bool preprocess(Packet& pkt, FlowPtr flow){
        auto data = m_decision.data();
        DVLOG(20) << "preprocessing packet";
        Decision::Inspect *i = boost::get<Decision::Inspect>(&data);
        if (i != nullptr){
            VLOG(20) << "static handle packet";
            return i->handler(pkt, flow);
        }
        return false;
    }
};

typedef std::shared_ptr<FlowImpl> FlowImplPtr;
typedef std::weak_ptr<FlowImpl> FlowImplWeakPtr;

class MapleBackend : public maple::Backend {
    SwitchConnectionPtr conn;
    uint8_t table{0};
    FlowImplPtr miss;
    std::unordered_map<uint64_t, uint16_t> miss_rules; //and their prioritets

    static FlowImplPtr flow_cast(maple::FlowPtr flow)
    {
        FlowImplPtr ret
            = std::dynamic_pointer_cast<FlowImpl>(flow);
        BOOST_ASSERT(ret);
        return ret;
    }

public:
    explicit MapleBackend(SwitchConnectionPtr conn, uint8_t table)
        : conn(conn), table(table), miss{new FlowImpl(conn, table) }
    {
        miss->decision( DecisionImpl{}.inspect(128,
                    [](Packet&, FlowPtr){return false;} )); // TODO: unhardcode
    }

    uint64_t miss_cookie() const { return miss->cookie(); }

    virtual void install(unsigned priority,
                         oxm::expirementer::full_field_set const& matchs,
                         maple::FlowPtr flow_) override
    {
        auto flow = flow_cast(flow_);
        for (auto& match : matchs.included().fields()){
            DVLOG(20) << "Installing prio=" << priority
                     << ", match={" << match << "}"
                     << " => cookie = " << std::setbase(16) << flow->cookie() << " on switch " << conn->dpid();
            flow->packet_out(priority, match);
        }
    }

    virtual void barrier_rule(unsigned priority,
                              oxm::expirementer::full_field_set const& match,
                              oxm::field<> const& test,
                              uint64_t id)
    {
        auto it = miss_rules.find(id);
        if (it == miss_rules.end()){
            miss_rules.insert({id, priority});
            install(priority, match, miss); // test is repeated in match
        } else if (it->second != priority) {
            it->second = priority;
            install(priority, match, miss);
        }
    }

    void remove(oxm::field_set const& match) override
    {
        DVLOG(20) << "Removing flows matching {" << match << "}" << " on switch " << conn->dpid();

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);

        fm.table_id(table);
        fm.cookie(Flow::cookie_space().first);
        fm.cookie_mask(Flow::cookie_space().second);
        fm.match(make_of_match(match));

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void remove(unsigned priority,
                oxm::field_set const& match) override
    {
        DVLOG(20) << "Removing flows matching prio=" << priority
                  << " with " << match;

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE_STRICT);

        fm.table_id(table);
        fm.cookie(Flow::cookie_space().first);
        fm.cookie_mask(Flow::cookie_space().second);
        fm.match(make_of_match(match));
        fm.priority(priority);

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void remove(maple::FlowPtr flow_) override
    {
        auto flow = flow_cast(flow_);
        DVLOG(20) << "Removing flow with cookie=" << flow->cookie();

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);

        fm.table_id(table);
        fm.cookie(flow->cookie());
        fm.cookie_mask(uint64_t(-1));

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void barrier() override
    {
        conn->send(of13::BarrierRequest());
    }
};

typedef boost::error_info< struct tag_pi_handler, std::string >
    errinfo_packetin_handler;

struct SwitchScope {
    SwitchConnectionPtr connection;
    MapleBackend backend;
    maple::Runtime<DecisionImpl, FlowImpl> runtime;
    PacketMissPipeline pipeline;
    std::unordered_map<uint64_t, FlowImplPtr> flows;
    uint8_t handler_table;
    std::function <Action*()> flood;

    SwitchScope(PacketMissPipelineFactory const& pipeline_factory,
                SwitchConnectionPtr connection,
                uint64_t dpid,
                uint8_t handler_table)
        : connection{connection}
        , backend{connection, handler_table}
        , runtime{std::bind(&SwitchScope::process, this, _1, _2), backend}
        , handler_table{handler_table}
    {
        for (auto &factory : pipeline_factory) {
            pipeline.emplace_back(factory.first,
                                  std::move(factory.second(connection)));
        }
    }

    DecisionImpl process(Packet& pkt, FlowImplPtr flow) const
    {
        DecisionImpl ret = DecisionImpl{};
        for (auto& handler: pipeline) {
            try {
                ret = (DecisionImpl&&)(handler.second(pkt, flow, ret));
                if (ret.base().return_)
                    return ret;
            } catch (boost::exception & e) {
                e << errinfo_packetin_handler(handler.first);
                throw;
            }
        }
        return ret;
    }

    bool isTableMiss(of13::PacketIn& pi) const
    {
        if (pi.reason() == of13::OFPR_NO_MATCH)
            return true;
        if (pi.reason() == of13::OFPR_ACTION &&
            pi.cookie() == backend.miss_cookie())
            return true;
        return false;
    }

    void processPacketIn(of13::PacketIn& pi);
    void processFlowRemoved(of13::FlowRemoved& fr);
};

void SwitchScope::processPacketIn(of13::PacketIn& pi)
{
    DVLOG(10) << "Packet-in on switch " << connection->dpid()
              << (isTableMiss(pi) ? " (miss)" : " (inspect)");

    // Serializes to/from raw buffer
    PacketParser pkt { pi, connection->dpid() };
    // Find flow in the trace tree
    std::shared_ptr<FlowImpl> flow = runtime(pkt);

    DVLOG(30) << "flow cookie is : " << std::setbase(16)
              << flow->cookie() << " packet cookie : " << pi.cookie();
    // Delete flow if it doesn't found or expired
    if (flow == nullptr || flow->state() == Flow::State::Expired) {
        flow = std::make_shared<FlowImpl>(connection, handler_table);
        flows[flow->cookie()] = flow;
    }
    if (flow->preprocess(pkt, flow)){
        return;
    }
    flow->packet_in(pi);

    switch (flow->state()) {
        case Flow::State::Egg: // If flow just created
        case Flow::State::Idle: // If flow was idle
        case Flow::State::Evicted: // If flow evicted from switches
                            // For exmaple : switch cannon handle this packet
                            // Or Packet Out needed
        {
            ModTrackingPacket mpkt {pkt};
            maple::Installer installer;
            try{
                std::tie(flow, installer) = runtime.augment(mpkt, flow);
            } catch (maple::priority_exceeded& e){
                LOG(WARNING) << "Exceeded priority, Trying update trace tree"
                             << "On switch : " << connection->dpid();
                try {
                    runtime.update();
                    std::tie(flow, installer) = runtime.augment(mpkt, flow);
                } catch (...) {
                    LOG(ERROR) << "Exceeded priority range."
                               << "Too many test functions"
                               << "on switch : " << connection->dpid();
                    // nothing we can do
                    throw;
                }
                    LOG(INFO) << "Updating trace tree succesful";
            }
            flow->mods( std::move(mpkt.mods()) );
            installer();
        }
        break;

        case Flow::State::Expired:
            BOOST_ASSERT(false);
        break;

        case Flow::State::Active:
        {
            DLOG_IF(ERROR, isTableMiss(pi))
                << "Table-miss on active non-inspect flow, "
                << "cookie = " << std::setbase(16) << flow->cookie()
                << ", reason = " << unsigned(pi.reason()) << " disposable : " << flow->disposable();
            // BOOST_ASSERT(not isTableMiss(pi));
            if (not isTableMiss(pi)){
                flow->decision(process(pkt, flow));
            } else {
                flow->wakeup();
            }
            // Maybe this packet arrived on switch when maple reload table, but may be from remowed flows
            // TODO: implement FSM of flow
            // TODO: make that packets arrived Only when reloading table */
        }
        break;
    }
}

void SwitchScope::processFlowRemoved(of13::FlowRemoved& fr)
{
    DVLOG(30) << "Flow-remowed message handled on  switch : " << connection->dpid();
    auto it = flows.find( fr.cookie() );
    if (it == flows.end())
        return;
    auto flow = it->second;

    flow->flow_removed(fr);
    if (flow->state() == Flow::State::Expired)
        flows.erase(it);
}

struct runos::MapleImpl {
    bool started{false};
    Maple &app;
    uint8_t handler_table;
    Config config;

    std::unordered_map<std::string, PacketMissHandlerFactory> handlers;
    PacketMissPipelineFactory pipeline_factory;
    std::unordered_map<uint64_t, SwitchScope> switches;
    MapleImpl(Maple& maple)
        : app(maple)
    { }
    ~MapleImpl() = default;
    SwitchScope *createSwitchScope(SwitchConnectionPtr conn)
    {

        // lock mutex is a hard operation
        auto it = switches.find(conn->dpid());
        if (it != switches.end())
            goto ret;
        {
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock(mutex);

            it = switches.find(conn->dpid());
            if (it != switches.end())
                goto ret;

            it = switches.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(conn->dpid()),
                                  std::forward_as_tuple(pipeline_factory,
                                                        conn,
                                                        conn->dpid(),
                                                        handler_table))
                         .first;
            return &it->second;
        }

        ret:
        SwitchScope *ctx = &it->second;
        return ctx;
   }

};

void Maple::init(Loader* loader, const Config& root_config)
{
    auto ctrl = Controller::get(loader);
    impl.reset(new MapleImpl(*this));
    impl->handler_table = ctrl->getTable("maple");
    impl->config = config_cd(root_config, "maple");
    ctrl->registerHandler<of13::PacketIn>(
            [=](of13::PacketIn &pi, SwitchConnectionPtr conn){
                //TODO : create a copy of packetIn
                impl->switches.at(conn->dpid()).processPacketIn(pi);
            });
    ctrl->registerHandler<of13::FlowRemoved>(
            [=](of13::FlowRemoved &fr, SwitchConnectionPtr conn){
                impl->switches.at(conn->dpid()).processFlowRemoved(fr);
            });
    QObject::connect(ctrl, &Controller::switchUp, this, &Maple::onSwitchUp);
}

void Maple::startUp(Loader*)
{
    auto config = impl->config;

    LOG(INFO) << "Registered packet-in handlers: ";
    for (const auto& handler : impl->handlers) {
        LOG(INFO) << "\t" << handler.first;
    }
    LOG(INFO) << ".";

    const auto& names = config.at("pipeline").array_items();
    // TODO: check dups
    for (const auto& name_token : names) {
        // TODO: warn if doesn't exists
        auto name = name_token.string_value();
        impl->pipeline_factory.emplace_back(name, impl->handlers.at(name));
    }
    // TODO: print unused handlers

    impl->started = true;
}

void Maple::onSwitchUp(SwitchConnectionPtr conn, of13::FeaturesReply fr)
{
    impl->createSwitchScope(conn);
}

void Maple::registerHandler(const char* name,
                            PacketMissHandlerFactory factory)
{
    if (impl->started) {
        LOG(ERROR) << "Registering handler after startup";
        return;
    }

    VLOG(10) << "Registering flow processor " << name;
    impl->handlers[std::string(name)] = factory;
}

Maple::~Maple() = default;
