#include "ACL.hh"
#include "Match.hh"
#include "Controller.hh"
#include "AppObject.hh"
#include "RestListener.hh"

#include <algorithm>

REGISTER_APPLICATION(ACL, {"controller", ""})

class MakeString {
public:
    MakeString() : stream(){}
    template<class T>
    MakeString& operator<< (const T &arg) {
        stream << arg;
        return *this;
    }
    operator std::string() const {
        return stream.str();
    }
protected:
    std::stringstream stream;
};

static const auto ANY_SRC_MAC = of13::EthSrc("00:00:00:00:00:00", "00:00:00:00:00:00");
static const auto ANY_DST_MAC = of13::EthDst("00:00:00:00:00:00", "00:00:00:00:00:00");
static const auto HOST_MAC_MASK = fluid_msg::EthAddress("ff:ff:ff:ff:ff:ff");

static const auto ANY_SRC_IP = of13::IPv4Src("0.0.0.0", "0.0.0.0");
static const auto ANY_DST_IP = of13::IPv4Dst("0.0.0.0", "0.0.0.0");
static const auto HOST_IP_MASK = fluid_msg::IPAddress("255.255.255.255");

static const uint8_t TCP_PROTO = 6;
static const uint8_t UDP_PROTO = 17;
static const uint8_t ICMP_PROTO = 1;
static const uint8_t ANY_PROTO = 255;

template<typename T> std::string stringify(const T& value) {
    return MakeString() << value;
}

template<> std::string stringify(const uint32_t& value) {
    if(value)
        return MakeString() << value;
    else
        return "any";
}

template<> std::string stringify(const uint16_t& value) {
    if(value)
        return MakeString() << value;
    else
        return "any";
}

using of13::EthSrc;
using of13::EthDst;

template<> std::string stringify(const EthSrc &value) {
    if(const_cast<EthSrc &>(value).equals(ANY_SRC_MAC))
        return "any";
    if(const_cast<EthSrc &>(value).mask() == HOST_MAC_MASK)
        return const_cast<EthSrc &>(value).value().to_string();
    else
        return const_cast<EthSrc &>(value).value().to_string() + "/" + const_cast<EthSrc &>(value).mask().to_string();
}

template<> std::string stringify(const EthDst &value) {
    if(const_cast<EthDst &>(value).equals(ANY_DST_MAC))
        return "any";
    if(const_cast<EthDst &>(value).mask() == HOST_MAC_MASK)
        return const_cast<EthDst &>(value).value().to_string();
    else
        return const_cast<EthDst &>(value).value().to_string() + "/" + const_cast<EthDst &>(value).mask().to_string();
}

using of13::IPv4Src;
using of13::IPv4Dst;

template<> std::string stringify(const IPv4Src &value) {
    if(const_cast<IPv4Src &>(value).equals(ANY_SRC_IP))
        return "any";
    if(const_cast<IPv4Src &>(value).mask() == HOST_IP_MASK)
        return AppObject::uint32_t_ip_to_string(const_cast<IPv4Src &>(value).value().getIPv4());
    else
        return AppObject::uint32_t_ip_to_string(const_cast<IPv4Src &>(value).value().getIPv4()) +
            "/" + AppObject::uint32_t_ip_to_string(const_cast<IPv4Src &>(value).mask().getIPv4());
}

template<> std::string stringify(const IPv4Dst &value) {
    if(const_cast<IPv4Dst &>(value).equals(ANY_DST_IP))
        return "any";
    if(const_cast<IPv4Dst &>(value).mask() == HOST_IP_MASK)
        return AppObject::uint32_t_ip_to_string(const_cast<IPv4Dst &>(value).value().getIPv4());
    else
        return AppObject::uint32_t_ip_to_string(const_cast<IPv4Dst &>(value).value().getIPv4()) +
            "/" + AppObject::uint32_t_ip_to_string(const_cast<IPv4Dst &>(value).mask().getIPv4());
}


template<> std::string stringify(const of13::IPProto &value) {
    switch(const_cast<of13::IPProto &>(value).value()) {
    case ICMP_PROTO:
        return "icmp";
    case TCP_PROTO:
        return "tcp";
    case UDP_PROTO:
        return "udp";
    case ANY_PROTO:
        return "any";
    default:
        return "unknown";
    }
}

json11::Json ACLEntry::to_json() const {
    json11::Json src = json11::Json::object {
        {"ip", stringify(src_ip)},
        {"mac", stringify(src_mac)},
        {"port", stringify(src_port)}
    };

    json11::Json dst = json11::Json::object {
        {"ip", stringify(dst_ip)},
        {"mac", stringify(dst_mac)},
        {"port", stringify(dst_port)}
    };

    return json11::Json::object {
        {"action", ACLAction_STR[int(action)]},
        {"proto", stringify(proto)},
        {"src", src},
        {"dst", dst},
        {"flow_limit", stringify(num_flows)}
    };
}

void ACLEntry::from_json(const json11::Json &json)
{
    const auto &j_action = json["action"].string_value();
    if(j_action == "deny")
        action = ACLAction::DENY;
    else
        action = ACLAction::ALLOW;

    using of13::IPProto;

    const auto &j_proto = json["proto"].string_value();
    if(j_proto == "any")
        proto = IPProto(ANY_PROTO);
    else if(j_proto == "icmp")
        proto = IPProto(ICMP_PROTO);
    else if(j_proto == "tcp")
        proto = IPProto(TCP_PROTO);
    else if(j_proto == "udp")
        proto = IPProto(UDP_PROTO);
    else
        LOG(ERROR) << "Unknown proto: \'" << j_proto << "\'";

    const auto &j_flows = json["flow_limit"];
    if(j_flows.is_number())
        num_flows = j_flows.int_value();
    else
        num_flows = 0;

    auto split = [](const std::string &str, const std::string &mask) {
        auto p = str.find('/');
        if(p != std::string::npos)
            return std::make_pair(str.substr(0, p), str.substr(p + 1, std::string::npos));
        else
            return std::make_pair(str, mask);
    };

    const auto &src = json["src"];

    const auto j_src_ip = src["ip"].string_value();
    if(j_src_ip == "any")
        src_ip = ANY_SRC_IP;
    else {
        auto p = split(j_src_ip, "255.255.255.255");
        src_ip = of13::IPv4Src(p.first, p.second);
    }

    const auto j_src_mac = src["mac"].string_value();
    if(j_src_mac == "any")
        src_mac = ANY_SRC_MAC;
    else {
        auto p = split(j_src_mac, "ff:ff:ff:ff:ff:ff");
        src_mac = of13::EthSrc(p.first, p.second);
    }

    if(src["port"].is_number())
        src_port = src["port"].int_value();
    else
        src_port = 0;

    const auto &dst = json["dst"];

    const auto j_dst_ip = dst["ip"].string_value();
    if(j_dst_ip == "any")
        dst_ip = ANY_DST_IP;
    else {
        auto p = split(j_dst_ip, "255.255.255.255");
        dst_ip = of13::IPv4Dst(p.first, p.second);
    }

    const auto j_dst_mac = dst["mac"].string_value();
    if(j_dst_mac == "any")
        dst_mac = ANY_DST_MAC;
    else {
        auto p = split(j_dst_mac, "ff:ff:ff:ff:ff:ff");
        dst_mac = of13::EthDst(p.first, p.second);
    }

    if(dst["port"].is_number())
        dst_port = dst["port"].int_value();
    else
        dst_port = 0;
}

void ACL::init(Loader *loader, const Config& config)
{
    Controller* ctrl = Controller::get(loader);
    ctrl->registerHandler(this);

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "rules/all");

    _aclId = 0;

    auto acl_config = config_cd(config, "acl");
    if (acl_config.find("rules") != acl_config.end()) {
        auto static_rules = acl_config.at("rules");
        for (auto& acl_json : static_rules.array_items()) {
            ACLEntry acl;
            acl.from_json(acl_json);
            addEntry(acl);
        }
    }
}

std::unique_ptr<OFMessageHandler> ACL::makeOFMessageHandler()
{
    return std::unique_ptr<OFMessageHandler>(new Handler(this));
}

std::string ACL::restName()
{
    return "acl";
}

bool ACL::eventable()
{
    return false;
}

std::string ACL::displayedName()
{
    return "ACL";
}

std::string ACL::page()
{
    return "acl.html";
}

json11::Json ACL::handleGET(std::vector<std::string> params, std::string body)
{
    if (params.size() == 2 && params[0] == "rules" && params[1] == "all") {
        return json11::Json(rules());
    }

    return "{}";
}

json11::Json ACL::handlePOST(std::vector<std::string> params, std::string body)
{
    return json11::Json::object {
        {"action", "" }
    };
}

json11::Json ACL::handleDELETE(std::vector<std::string> params, std::string body)
{
    return json11::Json::object {
        {"action", "" }
    };
}

std::vector<ACLEntry *> ACL::rules()
{
    std::vector<ACLEntry*> ret;
    ret.reserve(_acls.size());

    for (auto& pair : _acls) {
        ret.push_back(&pair.second);
    }
    return ret;
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
    if(flow->match(of13::EthType(0x0800))) { /* only IP is supported */
        const auto eth_src = flow->loadEthSrc();
        const auto eth_dst = flow->loadEthDst();

        const auto proto = flow->loadIPProto();
        const auto ip_src = flow->loadIPv4Src();
        const auto ip_dst = flow->loadIPv4Dst();

        uint16_t port_src = 0;
        uint16_t port_dst = 0;
        if(proto == TCP_PROTO) {
            port_src = flow->loadTCPSrc();
            port_dst = flow->loadTCPDst();
        } else if(proto == UDP_PROTO) {
            port_src = flow->loadUDPSrc();
            port_dst = flow->loadUDPDst();
        }

        std::vector<int32_t> acls;
        for(const auto &acl_entry : app->_acls) {
            const auto &acl = acl_entry.second;

            if(!of13::IPProto(255).equals(acl.proto) && !oxm_match(acl.proto, proto))
                continue;

            if(!oxm_match(acl.src_ip, ip_src) || !oxm_match(acl.dst_ip, ip_dst))
                continue;

            if(!oxm_match(acl.src_mac, eth_src) || !oxm_match(acl.dst_mac, eth_dst))
                continue;

            if(port_src && acl.src_port && port_src != acl.src_port)
                continue;

            if(port_dst && acl.dst_port && port_dst != acl.dst_port)
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
            LOG(INFO) << "DENY: " << eth_src.to_string() << " " << eth_dst.to_string();
            return Stop;
        }

        LOG(INFO) << "ALLOW: " << eth_src.to_string() << " " << eth_dst.to_string();
        return Continue;
    } else {
        return Continue;
    }
}
