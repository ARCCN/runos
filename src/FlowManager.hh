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

/** @file */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Common.hh"
#include "Loader.hh"
#include "Application.hh"
#include "Rest.hh"
#include "Switch.hh"
#include "AppObject.hh"
#include "json11.hpp"
#include "OFTransaction.hh"
#include "SwitchConnection.hh"

#include "oxm/field_set.hh"

typedef of13::OXMTLV* ModifyElem;
typedef std::vector<ModifyElem> ModifyList;

class Rule : public AppObject {
public:
    Rule(uint64_t _switch_id, of13::FlowStats flow);

    // rest
    uint64_t id() const override;
    json11::Json to_json() const override;
private:
    uint64_t rule_id;
    uint64_t switch_id;
    uint64_t cookie;

    std::vector<int> out_port;

    ModifyList modify;

    of13::FlowStats flow;
    bool active;
    void action_list(ActionList acts, std::vector<int> &out_port,
                   json11::Json::array &sets) const;
    json11::Json::object SetField(of13::OXMTLV *) const;
    friend class FlowManager;
};

// a vector of pointers to all rules, that are set in a switch
typedef std::vector<Rule*> Rules;

/**
 *
 * Application, that allow you manage flows on switchs table by Rest API.
 *
 * You may know which flows are installed on switch by GET request : GET /api/flow-manager/<switch_id>
 * And FlowManager will reply a json, which contains following information about flows :
 *
 *  Matches : input port, ethernet source/destination address, ethernet type, VLAN id, IP source/destenation address, IP protocol
 *  Actions : output port, goto table, metadata and set fields.
 *  Flow identificator.
 *
 * You may delete flow by its identifictator, for this you need send DELETE request : DELETE /api/flow-manager/<switch_id>/<flow_id>
 *
 *  This application support event model, and manage Rule objects.
 */
class FlowManager : public Application, RestHandler {
    Q_OBJECT
    SIMPLE_APPLICATION(FlowManager, "flow-manager")
public:
    void init(Loader* loader, const Config& rootConfig) override;
    void startUp(Loader *) override;

    // rest
    bool eventable() override {return true;}
    AppType type() override { return AppType::Service; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;
    json11::Json handleDELETE(std::vector<std::string> params, std::string body) override;

protected slots:
    void onSwitchDown(Switch* dp);
    void onResponse(SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> reply);
    void onSwitchUp(Switch* dp);
protected:
    void deleteRule(Switch *sw, Rule* rule);
    void dumpRule(Rule* rule);
    void timerEvent(QTimerEvent*) override;
    void addRule(Switch *sw, of13::FlowStats flow);

private:
    unsigned int interval;
    void sendFlowRequest(Switch *dp);
    class Controller* ctrl;
    class SwitchManager* sw_m;

    // a map of all switches (by dpid) and all rules for each of their
    std::unordered_map<uint64_t, Rules> all_switches_rules;
    //std::unordered_map<Flow*, Rule*> all_flows_rules;

    void cleanSwitchRules(Switch  *dp);
    void updateSwitchRules(Switch *dp, std::vector<of13::FlowStats> flows);
    OFTransaction* transaction;
};
