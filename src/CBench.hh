#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"

class CBench : public Application, public OFMessageHandlerFactory {
    SIMPLE_APPLICATION(CBench, "cbench")
public:
    void init(Loader* loader, const Config& config) override;

    std::string orderingName() const override { return "cbench"; }
    OFMessageHandler* makeOFMessageHandler() override { return new Handler(); }
    // Hackish way to sure that it's the only processor registered
    bool isPrereq(const std::string& name) const override { return true; }
    bool isPostreq(const std::string& name) const override { return true; }

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };
};
