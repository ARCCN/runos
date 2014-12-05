#pragma once
#include "Common.hh"

#include <mutex>
#include <unordered_map>

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "ILinkDiscovery.hh"
#include "FluidUtils.hh"

class LearningSwitch : public Application, OFMessageHandlerFactory {
SIMPLE_APPLICATION(LearningSwitch, "learning-switch")
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "forwarding"; }
    OFMessageHandler* makeOFMessageHandler() override { return new Handler(this); }
    bool isPrereq(const std::string &name) const;


    std::pair<bool, switch_and_port>
    findAndLearn(const switch_and_port& where,
                 const EthAddress& src,
                 const EthAddress& dst);

private:
    class Handler: public OFMessageHandler {
    public:
        Handler(LearningSwitch* app_) : app(app_) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        LearningSwitch* app;
    };

    class Topology* topology;
    class SwitchManager* switch_manager;

    std::mutex db_lock;
    std::unordered_map<EthAddress, switch_and_port> db;
};
