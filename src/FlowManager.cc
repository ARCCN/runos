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

#include <boost/lexical_cast.hpp>
#include <algorithm>

REGISTER_APPLICATION(FlowManager, {"controller", "switch-manager", "rest-listener", ""})

/**
 * ID for rules
 */
class RuleIDs {
    static uint64_t last_event;
public:
    static uint64_t getLastID();
};

static bool equalflows(of13::FlowStats &one,
                       of13::FlowStats &two)
{
    return (one.cookie() == two.cookie()             &&
            one.hard_timeout() == two.hard_timeout() &&
            one.idle_timeout() == two.idle_timeout() &&
            one.priority() == two.priority()         &&
            one.table_id() == two.table_id()         &&
            //one.lentgh() == two.lentgh()           &&
            one.get_flags() == two.get_flags()       &&
            one.match() == two.match()               &&
            one.instructions() == two.instructions()
            );
}

uint64_t RuleIDs::last_event = 0;

uint64_t RuleIDs::getLastID()
{ return ++last_event; }

Rule::Rule(uint64_t _switch_id, of13::FlowStats flow) :
    rule_id(RuleIDs::getLastID()),
    switch_id(_switch_id),
    flow(flow),
    active(true)
{ }

json11::Json::object Rule::SetField(of13::OXMTLV *field) const
{
    switch (field->field()){
    case of13::OFPXMT_OFB_IN_PORT:
        return json11::Json::object {
            { "in_port", (int)static_cast<of13::InPort*>(field)->value() }
        };
        break;
    case of13::OFPXMT_OFB_ETH_SRC:
        return json11::Json::object {
           { "eth_src", static_cast<of13::EthSrc*>(field)->value().to_string() }
        };
        break;
    case of13::OFPXMT_OFB_ETH_DST:
        return json11::Json::object {
            { "eth_dst", static_cast<of13::EthDst*>(field)->value().to_string() }
        };
        break;
    case of13::OFPXMT_OFB_ETH_TYPE:
        return json11::Json::object{
            { "eth_type", static_cast<of13::EthType*>(field)->value() }
        };
        break;
    case of13::OFPXMT_OFB_VLAN_VID:
        return json11::Json::object{
            { "vlan_vid", static_cast<of13::VLANVid*>(field)->value() }
        };
        break;
    case of13::OFPXMT_OFB_IPV4_SRC:
        return json11::Json::object {
            { "ip_src", AppObject::uint32_t_ip_to_string(static_cast<of13::IPv4Src*>(field)->value().getIPv4()) }
        };
        break;
    case of13::OFPXMT_OFB_IPV4_DST:
        return json11::Json::object {
            { "ip_dst", AppObject::uint32_t_ip_to_string(static_cast<of13::IPv4Dst*>(field)->value().getIPv4()) }
        };
        break;
    case of13::OFPXMT_OFB_IP_PROTO:
        return json11::Json::object {
            { "ip_proto", (int)(static_cast<of13::IPProto*>(field)->value()) }
        };
        break;
    default:
        LOG(ERROR) << " TODO : Unhandled SetField: " << (int)field->field();
    }
    return json11::Json::object{
        { "unhandled field", "error" }
    };
}

void Rule::action_list(ActionList acts, std::vector<int> &out_port,
                   json11::Json::array &sets) const
{
    json11::Json::object setact;
    for (Action *act : acts.action_list()){
        if (act->type() == of13::OFPAT_OUTPUT) {
            of13::OutputAction* output = static_cast<of13::OutputAction*>(act);
            out_port.push_back((int)output->port());
        }
        else if (act->type() == of13::OFPAT_SET_FIELD) {
            // set fields TODO : in separate function
            of13::SetFieldAction* set = static_cast<of13::SetFieldAction*>(act);
            auto field = set->field();
            sets.push_back(SetField(field));
        } // TODO : handle more action

    }
}

uint64_t Rule::id() const { return rule_id; }

json11::Json Rule::to_json() const
{
    json11::Json::object ret;
    ret["acitive"] = active ? "yes" : "no";
    ret["switch_id"] = boost::lexical_cast<std::string>(switch_id);
    // hack for correct work
    of13::FlowStats *flow1 = const_cast<of13::FlowStats *>(&flow);

    of13::Match match = flow1->match();

    if (match.in_port())
        ret["in_port"] = (int)(match.in_port()->value());
    if (match.eth_src())
        ret["eth_src"] = match.eth_src()->value().to_string();
    if (match.eth_dst())
        ret["eth_dst"] = match.eth_dst()->value().to_string();
    if (match.eth_type())
        ret["eth_type"] = match.eth_type()->value();
    if (match.ipv4_src())
        ret["ip_src"] = AppObject::uint32_t_ip_to_string(match.ipv4_src()->value().getIPv4());
    if (match.ipv4_dst())
        ret["ip_dst"] = AppObject::uint32_t_ip_to_string(match.ipv4_dst()->value().getIPv4());

    json11::Json::array set;
    std::vector<int> out_port;

    // Libfluid_msg has a bug, thus next two line must be separated
    auto instructions_tmp = flow1->instructions();
    auto instructions = instructions_tmp.instruction_set();
    for (of13::Instruction* inst : instructions){
        json11::Json::object tmp;
        switch(inst->type()){
        case (of13::OFPIT_GOTO_TABLE):
            ret["goto_table"] =
                   boost::lexical_cast<std::string>(((of13::GoToTable*)inst)->table_id());
            break;
        case (of13::OFPIT_WRITE_METADATA):
            tmp = json11::Json::object({
                {"metadata",
                   boost::lexical_cast<std::string>(((of13::WriteMetadata*)inst)->metadata()) },
                {"metadata_mask",
                   boost::lexical_cast<std::string>(((of13::WriteMetadata*)inst)->metadata_mask()) }
            });
            ret["metadata"] = tmp;
            break;
        case (of13::OFPIT_WRITE_ACTIONS):
            break;
        case (of13::OFPIT_APPLY_ACTIONS):
            action_list(((of13::ApplyActions *)inst)->actions(), out_port, set);
            break;
        case (of13::OFPIT_METER) :
            break;
        default :
            LOG(ERROR) << "Unhandled instuction type: " << inst->type();
            break;
        }
    }
    ret["out_port"] = out_port;
    ret["set_field"] = set;
    return ret;
}


void FlowManager::init(Loader *loader, const Config &rootConfig)
{
    auto config = config_cd(rootConfig, "flow-manager");
    interval = config_get(config, "interval", 30);
    ctrl = Controller::get(loader);
    sw_m = SwitchManager::get(loader);

    connect(sw_m, &SwitchManager::switchDown, this, &FlowManager::onSwitchDown);
    connect(sw_m, &SwitchManager::switchUp, this, &FlowManager::onSwitchUp);

    RestListener::get(loader)->registerRestHandler(this);

    transaction = ctrl->registerStaticTransaction(this);
    connect(transaction, &OFTransaction::response, this, &FlowManager::onResponse);

    acceptPath(Method::GET, "[0-9]+");
    acceptPath(Method::DELETE, "[0-9]+/[0-9]+");
}

void FlowManager::startUp(Loader *loader)
{
    startTimer(interval * 1000);
}

void FlowManager::addRule(Switch *sw, of13::FlowStats flow){
    if (sw) {
        Rule *rule = new Rule(sw->id(), flow);
        all_switches_rules[sw->id()].push_back(rule);
        addEvent(Event::Add, rule);
    }
}

void FlowManager::deleteRule(Switch *sw, Rule *rule)
{
    of13::FlowMod fm;
    fm.command(of13::OFPFC_DELETE);
    fm.out_group(of13::OFPG_ANY);

    if (VLOG_IS_ON(5)) {
        dumpRule(rule);
    }

    fm.match(rule->flow.match());
    fm.table_id(rule->flow.table_id());
    fm.cookie(rule->flow.cookie());
    fm.cookie_mask(0xffffffffffffffff);
    fm.out_port(of13::OFPP_ANY);
    fm.priority(rule->flow.priority());

    sw->connection()->send(fm);
}

void FlowManager::dumpRule(Rule *rule)
{
    VLOG(5) << "Deleting rule:";
    VLOG(10) << rule->to_json().dump();
}


void FlowManager::onSwitchDown(Switch *dp)
{
    cleanSwitchRules(dp);
}

void FlowManager::cleanSwitchRules(Switch *dp)
{
    Rules rules = all_switches_rules[dp->id()];
    for (Rule* rule : rules) {
        addEvent(Event::Delete, rule);
        rule->active = false;
    }
    all_switches_rules[dp->id()].clear();
}

void FlowManager::updateSwitchRules(Switch *dp,
                    std::vector<of13::FlowStats> flows)
{
    Rules& rules = all_switches_rules[dp->id()];
    auto end = std::remove_if(rules.begin(), rules.end(),
        [&flows, this](Rule *rule)->bool{
            for (auto it = flows.begin(); it != flows.end(); it++){
                if (equalflows(*it,rule->flow)){
                    flows.erase(it);
                    return false;
                }
            }
            addEvent(Event::Delete, rule);
            rule->active = false;
            return true;
        });
    rules.erase(end, rules.end());
    for (auto flow : flows){
        Rule *rule = new Rule(dp->id(), flow);
        rules.push_back(rule);
        addEvent(Event::Add, rule);
    }
}

void FlowManager::onSwitchUp(Switch* dp)
{
    sendFlowRequest(dp);
}

json11::Json FlowManager::handleGET(std::vector<std::string> params, std::string body)
{
    if (params[0] != "all") {
        uint64_t id = std::stoull(params[0]);
        auto sw = sw_m->getSwitch(id);
        if (!sw){
            return json11::Json::object{{"error" , "switch not found"}};
        }
        uint64_t dpid = sw_m->getSwitch(id)->id();
        Rules active_rules;
        for (auto rule : all_switches_rules[dpid]) {
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
        return json11::Json::object{{"error" , "switch not found"}};
    }
    auto dpid = sw->id();
    uint64_t flow_id = std::stoull(params[1]);

    if (all_switches_rules.find(dpid) == all_switches_rules.end()) {
        return json11::Json::object{{"error" , "switch has not flows"}};
    }
    Rules rules = all_switches_rules[dpid];
    for (Rule* rule : rules) {
        if (rule->id() == flow_id) {
            deleteRule(sw, rule);
            // note: corresponding elem of all_switches_rules will be deleted automatically on the next
            // internal view correction (by `updateSwitchRules` call)
            return json11::Json::object{{"flow-manager", "flow deleted"}};
        }
    }
    return json11::Json::object{{"error", "flow not found"}};
}

void FlowManager::timerEvent(QTimerEvent *)
{
    for (Switch *sw : sw_m->switches()) {
        sendFlowRequest(sw);
    }
}

void FlowManager::sendFlowRequest(Switch* dp){
    of13::MultipartRequestFlow mprf;
    mprf.table_id(of13::OFPTT_ALL);
    mprf.out_port(of13::OFPP_ANY);
    mprf.out_group(of13::OFPG_ANY);
    mprf.cookie(0x0);  // match: cookie & mask == field.cookie & mask
    mprf.cookie_mask(0x0);
    mprf.flags(0);
    transaction->request(dp->connection(), mprf);
}

void FlowManager::onResponse(SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> _reply){
    of13::MultipartReplyFlow reply = _reply->multipartReplyFlow;
    Switch *sw = sw_m->getSwitch(conn->dpid());
    updateSwitchRules(sw, reply.flow_stats());
}
