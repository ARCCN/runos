#pragma once

#include "Common.hh"
#include "Controller.hh"
#include "Switch.hh"
#include "Application.hh"
#include "json11.hpp"
#include "Rest.hh"
#include "RestListener.hh"
#include "AppObject.hh"

class Rule : public AppObject {
public:
    Rule(Flow* _flow, uint64_t _switch_id);
    bool active;
    Flow* flow;

    uint64_t id() const override;
    json11::Json to_json() const;
private:
    uint64_t rule_id;
    uint64_t switch_id;

    uint32_t in_port;
    EthAddress eth_src;
    EthAddress eth_dst;
    std::vector<int> out_port;
};

typedef std::vector<Rule*> Rules;

class FlowManagerRest : public Rest {
public:
    FlowManagerRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;
private:
    class FlowManager* m;

    friend class FlowManager;
};

class FlowManager : public Application, OFMessageHandlerFactory {
    Q_OBJECT
    SIMPLE_APPLICATION(FlowManager, "flow-manager")
public:
    FlowManager();
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "flow-detector"; }
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override { return std::unique_ptr<OFMessageHandler>(new Handler(this)); }
    bool isPrereq(const std::string &name) const;

protected slots:
    void onStateChanged(Flow::FlowState new_state, Flow::FlowState old_state);
protected:
    void addRule(int conn_id, Rule* rule);
private:
    Controller* ctrl;
    SwitchManager* sw_m;
    std::unordered_map<int, Rules> switch_rules;
    std::unordered_map<Flow*, Rule*> flow_rule;
    FlowManagerRest* r;

    class Handler: public OFMessageHandler {
    public:
        Handler(FlowManager* app_) : app(app_) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        FlowManager* app;
    };

    friend class FlowManagerRest;
};
