#pragma once
#include "Common.hh"

#include <unordered_map>

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "FluidUtils.hh"

class SimpleLearningSwitch : public Application, OFMessageHandlerFactory {
SIMPLE_APPLICATION(SimpleLearningSwitch, "simple-learning-switch")
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override;
    OFMessageHandler* makeOFMessageHandler() override;

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        // MAC -> port mapping for EVERY switch
        std::unordered_map<EthAddress, uint32_t> seen_port;
    };
};
