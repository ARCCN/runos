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
#include <unordered_map>

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFTransaction.hh"
#include "Switch.hh"
#include "Rest.hh"
#include "json11.hpp"

class FlowDesc {
    struct FlowDescImpl* m;
public:
    FlowDesc();

    uint32_t in_port();
    uint32_t out_port();
    uint16_t eth_type();
    EthAddress eth_src();
    EthAddress eth_dst();
    IPAddress ip_src();
    IPAddress ip_dst();

    int idle();
    int hard();
    uint16_t priority();

    void in_port(uint32_t port);
    void out_port(uint32_t port);
    void eth_type(uint16_t type);
    void eth_src(std::string mac);
    void eth_dst(std::string mac);
    void ip_src(std::string ip);
    void ip_dst(std::string ip);

    void idle(int time);
    void hard(int time);
    void priority(int prio);


};

class StaticFlowPusher : public Application, RestHandler {
    SIMPLE_APPLICATION(StaticFlowPusher, "static-flow-pusher")
public:
    void init(Loader* loader, const Config& rootConfig) override;
    void sendToSwitch(Switch* dp, FlowDesc* fd);

    std::string restName() override { return "static-flow-pusher"; }
    bool eventable() override { return false; }
    AppType type() override { return AppType::Service; }

    json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
private:
    SwitchManager* sw_m;
    class FlowManager* flow_m;

    std::string def_act;
    std::unordered_map<std::string, json11::Json> flows_map;
    OFTransaction* new_flow;
    uint32_t start_prio;

    of13::FlowMod formFlowMod(FlowDesc* fd, Switch *sw);
    FlowDesc readFlowFromConfig(Config config);
    void cleanFlowTable(OFConnection* ofconn);
    void sendDefault(Switch* sw);

private slots:
    void onSwitchDiscovered(Switch* dp);
    void onSwitchDown(Switch* dp);
};
