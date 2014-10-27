#pragma once
#include "Common.hh"

#include <unordered_map>

#include "Application.hh"
#include "OFMessageHandler.hh"
#include "FluidUtils.hh"

class SimpleLearningSwitch : public Application, OFMessageHandlerFactory {
Q_OBJECT
public:
    APPLICATION(SimpleLearningSwitch);

    void init(AppProvider* provider, const Config& config) override;
    void startUp(AppProvider* provider) override;
    std::string orderingName() const override;
    OFMessageHandler* makeOFMessageHandler() override;

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(shared_ptr<OFConnection> ofconn, Flow* flow) override;
    private:
        // conn_id -> MAC -> seen_port for ONE switch
        std::unordered_map<int, std::unordered_map<
            EthAddress, uint32_t> > packet_seen;
    };
};
