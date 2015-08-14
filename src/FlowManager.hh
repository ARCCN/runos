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

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Common.hh"
#include "Loader.hh"
#include "Application.hh"
#include "OFMessageHandler.hh"
#include "Rest.hh"
#include "Switch.hh"
#include "AppObject.hh"
#include "json11.hpp"

class Rule : public AppObject {
public:
    enum Type {
        TraceTree,
        Static
    };

    Rule(Flow* _flow, uint64_t _switch_id);
    bool active;
    Flow* flow;

    uint64_t id() const override;
    json11::Json to_json() const;
private:
    Type type;
    uint64_t rule_id;
    uint64_t switch_id;

    uint32_t in_port;
    EthAddress eth_src;
    EthAddress eth_dst;
    uint16_t eth_type;
    uint32_t ip_src;
    uint32_t ip_dst;
    std::vector<int> out_port;

    friend class FlowManager;
};

typedef std::vector<Rule*> Rules;

class FlowManager : public Application, OFMessageHandlerFactory, RestHandler {
    Q_OBJECT
    SIMPLE_APPLICATION(FlowManager, "flow-manager")
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "flow-detector"; }
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override { return std::unique_ptr<OFMessageHandler>(new Handler(this)); }
    bool isPrereq(const std::string &name) const;

    std::string restName() override {return "flow";}
    bool eventable() override {return true;}
    AppType type() override { return AppType::Service; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;
    json11::Json handleDELETE(std::vector<std::string> params, std::string body) override;

    void addToFlowManager(Flow* flow, uint64_t dpid);

protected slots:
    void onStateChanged(Flow::FlowState new_state, Flow::FlowState old_state);
    void onSwitchDown(Switch* dp);
protected:
    void addRule(Switch *sw, Flow *flow, Rule::Type type);
    void deleteRule(Switch *sw, Rule* rule);
    void dumpRule(Rule* rule);
private:
    class Controller* ctrl;
    class SwitchManager* sw_m;
    std::unordered_map<int, Rules> switch_rules;
    std::unordered_map<Flow*, Rule*> flow_rule;

    class Handler: public OFMessageHandler {
    public:
        Handler(FlowManager* app_) : app(app_) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        FlowManager* app;
    };
};
