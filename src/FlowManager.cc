/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FlowManager.hh"

#include "Controller.hh"
#include "RestListener.hh"

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
    in_port = 0;
    eth_src = EthAddress();
    eth_dst = EthAddress();
    eth_type = 0;
    ip_src = 0;
    ip_dst = 0;
    if (_flow) {
        Trace& trace = _flow->trace();
        for (TraceEntry& tr_entry : trace) {
            switch (tr_entry.tlv->field()) {
            case of13::OFPXMT_OFB_IN_PORT:
                in_port = ((of13::InPort*)tr_entry.tlv)->value(); break;
            case of13::OFPXMT_OFB_ETH_SRC:
                eth_src = ((of13::EthSrc*)tr_entry.tlv)->value(); break;
            case of13::OFPXMT_OFB_ETH_DST:
                eth_dst = ((of13::EthDst*)tr_entry.tlv)->value(); break;
            case of13::OFPXMT_OFB_ETH_TYPE:
                eth_type = ((of13::EthType*)tr_entry.tlv)->value(); break;
            case of13::OFPXMT_OFB_IPV4_SRC:
                ip_src = ((of13::IPv4Src*)tr_entry.tlv)->value().getIPv4(); break;
            case of13::OFPXMT_OFB_IPV4_DST:
                ip_dst = ((of13::IPv4Dst*)tr_entry.tlv)->value().getIPv4(); break;
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
        {"ip_src", AppObject::uint32_t_ip_to_string(ip_src)},
        {"ip_dst", AppObject::uint32_t_ip_to_string(ip_dst)},
        {"out_port", out_port}
    };
}

void FlowManager::init(Loader *loader, const Config &config)
{
    ctrl = Controller::get(loader);
    sw_m = SwitchManager::get(loader);
    ctrl->registerHandler(this);

    connect(sw_m, &SwitchManager::switchDown, this, &FlowManager::onSwitchDown);

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "[0-9]+");
    acceptPath(Method::DELETE, "[0-9]+/[0-9]+");
}

void FlowManager::addToFlowManager(Flow *flow, uint64_t dpid)
{
    Switch* sw = sw_m->getSwitch(dpid);
    if (!sw)
        return;
    addRule(sw, flow, Rule::Static);

    flow->setLive();
}

void FlowManager::addRule(Switch* sw, Flow* flow, Rule::Type type)
{
    if (sw && flow) {
        Rule* rule = new Rule(flow, sw->id());
        rule->type = type;
        switch_rules[sw->ofconn()->get_id()].push_back(rule);
        flow_rule[rule->flow] = rule;
        connect(flow, &Flow::stateChanged, this, &FlowManager::onStateChanged);
    }
}

void FlowManager::deleteRule(Switch *sw, Rule *rule)
{
    of13::FlowMod fm;
    fm.command(of13::OFPFC_DELETE);
    fm.out_group(of13::OFPG_ANY);
    fm.table_id(of13::OFPTT_ALL);

    if (VLOG_IS_ON(5)) {
        dumpRule(rule);
    }

    if (rule->in_port > 0) {
        fm.add_oxm_field(new of13::InPort(rule->in_port));
    }

    if (rule->eth_src.to_string() != "00:00:00:00:00:00") {
        fm.add_oxm_field(new of13::EthSrc(rule->eth_src));
    }

    if (rule->eth_dst.to_string() != "00:00:00:00:00:00") {
        fm.add_oxm_field(new of13::EthDst(rule->eth_dst));
    }

    if (rule->eth_type > 0) {
        fm.add_oxm_field(new of13::EthType(rule->eth_type));
    }

    if (rule->ip_src > 0) {
        fm.add_oxm_field(new of13::IPv4Src(IPAddress(rule->ip_src)));
    }

    if (rule->ip_dst > 0) {
        fm.add_oxm_field(new of13::IPv4Dst(IPAddress(rule->ip_dst)));
    }

    if (rule->out_port.at(0) > 0) {
        fm.out_port(rule->out_port.at(0));
    }

    uint8_t* buf = fm.pack();
    sw->ofconn()->send(buf, fm.length());
    OFMsg::free_buffer(buf);
}

void FlowManager::dumpRule(Rule *rule)
{
    VLOG(5) << "Deleting rule:";
    VLOG(5) << "  in_port: " << rule->in_port;
    VLOG(5) << "  eth_src: " << rule->eth_src.to_string();
    VLOG(5) << "  eth_dst: " << rule->eth_dst.to_string();
    VLOG(5) << "  eth_type: " << rule->eth_type;
    VLOG(5) << "  ip_src: " << AppObject::uint32_t_ip_to_string(rule->ip_src);
    VLOG(5) << "  ip_dst: " << AppObject::uint32_t_ip_to_string(rule->ip_dst);
    VLOG(5) << "  out_port: " << rule->out_port.at(0);
}

void FlowManager::onStateChanged(Flow::FlowState new_state, Flow::FlowState old_state)
{
    Flow* flow = (Flow*)sender();
    Rule* rule = flow_rule[flow];
    if (new_state == Flow::FlowState::Live) {
        rule->active = true;
        addEvent(Event::Add, rule);
    }
    if (new_state == Flow::FlowState::Shadowed) {
        rule->active = false;
        addEvent(Event::Delete, rule);
    }

    if (new_state == Flow::FlowState::Destroyed) {
        addEvent(Event::Delete, rule);
        flow_rule.erase(flow);
    }
}

void FlowManager::onSwitchDown(Switch *dp)
{
    Rules rules = switch_rules[dp->ofconn()->get_id()];
    for (Rule* rule : rules) {
        addEvent(Event::Delete, rule);
    }
    switch_rules[dp->ofconn()->get_id()].clear();
}

OFMessageHandler::Action FlowManager::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    if (flow->flags() & Flow::Disposable) {
        return Continue;
    }

    app->addRule(app->sw_m->getSwitch(ofconn), flow, Rule::TraceTree);
    return Continue;
}

json11::Json FlowManager::handleGET(std::vector<std::string> params, std::string body)
{
    if (params[0] != "all") {
        uint64_t id = std::stoull(params[0]);
        int conn_id = sw_m->getSwitch(id)->ofconn()->get_id();
        Rules active_rules;
        for (auto rule : switch_rules[conn_id]) {
            if (rule->active)
                active_rules.push_back(rule);
        }
        return json11::Json(active_rules);
    }

    return "{}";
}

json11::Json FlowManager::handleDELETE(std::vector<std::string> params, std::string body)
{
    uint64_t id = std::stoull(params[0]);
    Switch* sw = sw_m->getSwitch(id);
    if (!sw) {
        return "{\"error\": \"switch now found\"}";
    }
    OFConnection* ofconn = sw->ofconn();
    int conn_id = ofconn->get_id();
    uint64_t flow_id = std::stoull(params[1]);
    TraceTree* trace_tree = ctrl->getTraceTree(id);

    if (switch_rules.find(conn_id) == switch_rules.end()) {
        return "{\"error\": \"switch has not flows\"}";
    }
    Rules rules = switch_rules[conn_id];
    int elem = 0;
    for (Rule* rule : rules) {
        if (rule->id() == flow_id) {
            deleteRule(sw_m->getSwitch(id), rule);
            for (auto it : flow_rule) {
                if (it.second == rule) {
                    it.first->setDestroy();
                    switch_rules[conn_id].erase(switch_rules[conn_id].begin() + elem);

                    if (rule->type == Rule::TraceTree) {
                        static const struct Barrier {
                            uint8_t* data;
                            size_t len;
                            Barrier() {
                                of13::BarrierRequest br;
                                data = br.pack();
                                len = br.length();
                            }
                            ~Barrier() { OFMsg::free_buffer(data); }
                        } barrier;

                        trace_tree->cleanFlowTable(ofconn);
                        ofconn->send(barrier.data, barrier.len);
                        trace_tree->buildFlowTable(ofconn);
                    }
                    break;
                }
            }
            return "{\"flow-manager\": \"flow deleted\"}";
        }
        ++elem;
    }
    return "\"error\": \"flow not found\"";
}

bool FlowManager::isPrereq(const std::string &name) const
{
    return name == "forwarding";
}
