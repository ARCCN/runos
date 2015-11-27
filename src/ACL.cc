#include "ACL.hh"
#include "Match.hh"
#include "Controller.hh"
#include "AppObject.hh"

#include <algorithm>

REGISTER_APPLICATION(ACL, {"controller", ""})

void ACL::init(Loader *loader, const Config& config)
{
    Controller* ctrl = Controller::get(loader);
    ctrl->registerHandler(this);

    _aclId = 0;

    addEntry({
        ACLAction::DENY, 255,
        of13::IPv4Src("10.0.0.0", "255.255.0.0"), of13::EthSrc("00:00:00:00:00:00", "00:00:00:00:00:00"), 0,
        of13::IPv4Dst("0.0.0.0", "0.0.0.0"), of13::EthDst("00:00:00:00:00:00", "00:00:00:00:00:00"), 0, 0});

    addEntry({
        ACLAction::ALLOW, 255,
        of13::IPv4Src("0.0.0.0", "0.0.0.0"), of13::EthSrc("00:00:00:00:00:00", "00:00:00:00:00:00"), 0,
        of13::IPv4Dst("0.0.0.0", "0.0.0.0"), of13::EthDst("00:00:00:00:00:00", "00:00:00:00:00:00"), 0, 0});
}

std::string ACL::orderingName() const
{
    return "acl-filtering";
}

std::unique_ptr<OFMessageHandler> ACL::makeOFMessageHandler()
{
    return std::unique_ptr<OFMessageHandler>(new Handler(this));
}

bool ACL::isPrereq(const std::string &name) const
{
    return false;
}

bool ACL::isPostreq(const std::string &name) const
{
    return name == "forwarding";
}

/* handlers */
int32_t ACL::addEntry(const ACLEntry &acl)
{
    _acls[_aclId] = acl;
    _aclId = _aclId + 1;
    return _aclId - 1;
}

int32_t ACL::modEntry(int32_t id, const ACLEntry &acl)
{
    return id;
}

int32_t ACL::delEntry(int32_t id)
{
    return 0;
}

OFMessageHandler::Action ACL::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    std::vector<int32_t> acls;

    for(const auto &acl_entry : app->_acls) {
        const auto &acl = acl_entry.second;

        if(!of13::IPProto(255).equals(acl.proto) && !oxm_match(acl.proto, flow->pkt()->readIPProto()))
            continue;

        if(!oxm_match(acl.src_ip, flow->pkt()->readIPv4Src()) || !oxm_match(acl.dst_ip, flow->pkt()->readIPv4Dst()))
            continue;

        if(!oxm_match(acl.src_mac, flow->pkt()->readEthSrc()) || !oxm_match(acl.dst_mac, flow->pkt()->readEthDst()))
            continue;

        acls.push_back(acl_entry.first);
    }

    auto popcnt = [](uint32_t v) {
        uint32_t bits = 0;
        for(bits = 0; v; bits++)
            v &= (v - 1);

        return bits;
    };

    std::sort(acls.begin(), acls.end(), [this, popcnt](int32_t idx_a, int32_t idx_b) {
        const ACLEntry &a = app->_acls[idx_a];
        const ACLEntry &b = app->_acls[idx_b];

        uint32_t a_mask_src = popcnt(const_cast<ACLEntry &>(a).src_ip.mask().getIPv4());
        uint32_t b_mask_src = popcnt(const_cast<ACLEntry &>(b).src_ip.mask().getIPv4());

        uint32_t a_mask_dst = popcnt(const_cast<ACLEntry &>(a).dst_ip.mask().getIPv4());
        uint32_t b_mask_dst = popcnt(const_cast<ACLEntry &>(b).dst_ip.mask().getIPv4());

        return a_mask_src > b_mask_src || (a_mask_src == b_mask_src && a_mask_dst > b_mask_dst);
    });

    bool deny = acls.empty() || app->_acls[acls[0]].action == ACLAction::DENY;

    if(deny) {
        ACLEntry &acl = app->_acls[acls[0]];

        const auto mac_src = flow->pkt()->readEthSrc();
        const auto mac_dst = flow->pkt()->readEthDst();

        LOG(INFO) << "Flow: " << mac_src.to_string() << " " << mac_dst.to_string() << " denied";

        flow->match(acl.src_ip);
        flow->match(acl.dst_ip);

        flow->match(of13::EthSrc(mac_src));
        flow->match(of13::EthDst(mac_dst));

        return Stop;
    } else
        return Continue;
}
