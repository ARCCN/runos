#pragma once

#include <map>
#include <array>

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "Rest.hh"

enum class ACLAction : int32_t {
    ALLOW = 0, DENY = 1
};

std::array<std::string, 2> ACLAction_STR = {"allow", "deny"};

struct ACLEntry {
    ACLAction     action;
    of13::IPProto proto;

    of13::IPv4Src src_ip;
    of13::EthSrc  src_mac;
    uint16_t      src_port;

    of13::IPv4Dst dst_ip;
    of13::EthDst  dst_mac;
    uint16_t      dst_port;

    uint32_t      num_flows;

    void from_json(const json11::Json &json);
    json11::Json to_json() const;
};

class ACL : public Application, public OFMessageHandlerFactory, public RestHandler {
SIMPLE_APPLICATION(ACL, "acl")
public:
    void init(Loader* loader, const Config& config) override;

    std::string orderingName() const override;
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override;

    /* REST methods & handlers */
    std::string restName() override;
    bool eventable() override;
    std::string displayedName();
    std::string page() override;
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;
    json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
    json11::Json handleDELETE(std::vector<std::string> params, std::string body) override;

    /* thread safe getters*/
    std::vector<ACLEntry *> rules();

    /* ordering */
    bool isPrereq(const std::string &name) const override;
    bool isPostreq(const std::string &name) const override;
protected:
    std::map<int32_t, ACLEntry> _acls;
    int32_t _aclId;

    int32_t addEntry(const ACLEntry &acl);
    int32_t modEntry(int32_t id, const ACLEntry &acl);
    int32_t delEntry(int32_t id);
private:
    class Handler: public OFMessageHandler {
    private:
        ACL *app;
    public:
        Handler(ACL *a){ app = a; }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };

    friend class ACL::Handler;
};
