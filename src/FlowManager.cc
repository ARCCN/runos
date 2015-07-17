#include "FlowManager.hh"

REGISTER_APPLICATION(FlowManager, {"controller", "switch-manager", "rest-listener", ""})

/**
 * ID for rules
 */
class RuleIDs {
    static uint64_t last_event;
public:
    static uint64_t getLastID();
};

uint64_t RuleIDs::last_event = 0;

uint64_t RuleIDs::getLastID()
{ return ++last_event; }


Rule::Rule(Flow* _flow, uint64_t _switch_id) :
    active(true),
    flow(_flow),
    rule_id(RuleIDs::getLastID()),
    switch_id(_switch_id)
{
    Trace& trace = _flow->trace();
    for (TraceEntry& tr_entry : trace) {
        switch (tr_entry.tlv->field()) {
        case of13::OFPXMT_OFB_IN_PORT:
            in_port = ((of13::InPort*)tr_entry.tlv)->value(); break;
        case of13::OFPXMT_OFB_ETH_SRC:
            eth_src = ((of13::EthSrc*)tr_entry.tlv)->value(); break;
        case of13::OFPXMT_OFB_ETH_DST:
            eth_dst = ((of13::EthDst*)tr_entry.tlv)->value(); break;
        }
    }

    ActionList actions = _flow->get_action();
    for (Action* act : actions.action_list()) {
        if (act->type() == of13::OFPAT_OUTPUT) {
            of13::OutputAction* output = static_cast<of13::OutputAction*>(act);
            out_port.push_back((int)output->port());
        }
    }
}

uint64_t Rule::id() const
{ return rule_id; }

json11::Json Rule::to_json() const
{
    return json11::Json::object {
        {"switch_id", uint64_to_string(switch_id)},
        {"in_port", (int)in_port},
        {"eth_src", eth_src.to_string()},
        {"eth_dst", eth_dst.to_string()},
        {"out_port", out_port}
    };
}

FlowManager::FlowManager()
{
    r = new FlowManagerRest("Flow Manager", "none");
    r->m = this;
    r->makeEventApp();
}

void FlowManager::init(Loader *loader, const Config &config)
{
    ctrl = Controller::get(loader);
    sw_m = SwitchManager::get(loader);
    RestListener::get(loader)->newListener("flow", r);
    ctrl->registerHandler(this);
}

void FlowManager::addRule(int conn_id, Rule *rule)
{
    switch_rules[conn_id].push_back(rule);
    flow_rule[rule->flow] = rule;
}

void FlowManager::onStateChanged(Flow::FlowState new_state, Flow::FlowState old_state)
{
    Flow* flow = (Flow*)sender();
    Rule* rule = flow_rule[flow];
    if (old_state == Flow::FlowState::New && new_state == Flow::FlowState::Live) {
        r->addEvent(Event::Add, rule);
    }
    if (old_state == Flow::FlowState::Shadowed && new_state == Flow::FlowState::Live) {
        rule->active = true;
        r->addEvent(Event::Add, rule);
    }
    if (new_state == Flow::FlowState::Shadowed) {
        rule->active = false;
        r->addEvent(Event::Delete, rule);
    }

    if (new_state == Flow::FlowState::Destroyed) {
        //TODO: delete from anywhere
        r->addEvent(Event::Delete, rule);
    }
}

OFMessageHandler::Action FlowManager::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    if (flow->flags() & Flow::Disposable) {
        return Continue;
    }

    uint64_t switch_id = app->sw_m->getSwitch(ofconn)->id();
    Rule* rule = new Rule(flow, switch_id);
    app->addRule(ofconn->get_id(), rule);
    connect(flow, &Flow::stateChanged, app, &FlowManager::onStateChanged);

    return Continue;
}

std::string FlowManagerRest::handle(std::vector<std::string> params)
{
    if (params[0] != "GET")
        return "{\"error\":\"error\"}";

    if (params[2] != "all") {
        uint64_t id = std::stoull(params[2]);
        int conn_id = m->sw_m->getSwitch(id)->ofconn()->get_id();
        Rules active_rules;
        for (auto rule : m->switch_rules[conn_id]) {
            if (rule->active)
                active_rules.push_back(rule);
        }
        return json11::Json(active_rules).dump();
    }
    return "{}";
}

bool FlowManager::isPrereq(const std::string &name) const
{
    return name == "forwarding";
}
