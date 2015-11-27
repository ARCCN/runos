#pragma once

#include <map>

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"

enum class ACLAction {
    ALLOW, DENY
};

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
};

class ACL : public Application, public OFMessageHandlerFactory {
SIMPLE_APPLICATION(ACL, "acl")
public:
    void init(Loader* loader, const Config& config) override;

    std::string orderingName() const override;
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override;

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
