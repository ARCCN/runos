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

/* The main purpose of this module is to have stats from all discovered switches.
 * It is done by a few things:
 *  - every n seconds we get current set of switches and send stats requests for each of them.
 *    Then we receive and save to our internal representation their answers.
 *  - also, we correct out internal representation when SwitchManager discovers a new switch.
 * Collected stats are sent as responses for REST API requests.
 * */

#pragma once

#include <QTimer>
#include <unordered_map>
#include <vector>

#include <string>

#include "Common.hh"
#include "Switch.hh"
#include "Application.hh"
#include "Loader.hh"
#include "Rest.hh"
#include "AppObject.hh"
#include "json11.hpp"

// represents stats for a port
struct port_packets_bytes : public AppObject {
    // from-switch stats
    of13::PortStats stats;

    port_packets_bytes(of13::PortStats stats);
    port_packets_bytes();

    uint64_t id() const override;
    json11::Json to_json() const override;
};

class SwitchPortStats {
private:
    // switch id
    Switch* sw;
    // stats for each port
    std::unordered_map<int, port_packets_bytes> port_stats;

public:
    // setters
    void insertElem(std::pair<int, port_packets_bytes> elem) { port_stats.insert(elem); }

    // getters
    port_packets_bytes& getElem(int key) { return port_stats.at(key); }
    std::vector<port_packets_bytes> to_vector();

    SwitchPortStats(Switch* _sw);
    SwitchPortStats();

    friend class SwitchStats;
    friend class SwitchStatsRest;
};

/**
* An application which gathers port statistics from all known switches every n seconds
*/
class SwitchStats: public Application, RestHandler {
    Q_OBJECT
    SIMPLE_APPLICATION(SwitchStats, "switch-stats")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* provider) override;

    bool eventable() override {return false;}
    AppType type() override { return AppType::Service; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;

public slots:
    /**
    * Called when a switch has answered with MulipartReplyPortStats message
    */ 
    void portStatsArrived(SwitchConnectionPtr, std::shared_ptr<OFMsgUnion>);
    // called when a new switch is discovered
    void newSwitch(Switch* sw);

private slots:
    // sends stats request to each switch, registered on the controller.
    // The method is called every n (set in config) seconds -- timeout.
    // And stats getting process continues in `portStatsArrived` method.
    void pollTimeout();

private:
    unsigned c_poll_interval;
    QTimer* m_timer;
    SwitchManager* m_switch_manager;
    // port stats for each switch: {dpid: {port_id: stat}}
    std::unordered_map<uint64_t, SwitchPortStats> all_switches_stats;
    OFTransaction* pdescr;
};
