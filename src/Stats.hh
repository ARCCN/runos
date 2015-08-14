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

struct port_packets_bytes : public AppObject {
    uint32_t port_no;
    uint64_t tx_packets;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;

    uint64_t tx_byte_speed;
    uint64_t rx_byte_speed;

    port_packets_bytes(uint32_t pn, uint64_t tp, uint64_t rp, uint64_t tb, uint64_t rb);
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
    void insertElem(std::pair<int, port_packets_bytes> elem) { port_stats.insert(elem); }
    port_packets_bytes& getElem(int key) { return port_stats.at(key); }
    std::vector<port_packets_bytes> getPPB_vec();

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

    std::string restName() override {return "stats";}
    bool eventable() override {return false;}
    AppType type() override { return AppType::Service; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;

public slots:
    /**
    * Called when a switch has answered with MulipartReplyPortStats message
    */ 
    void portStatsArrived(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> reply);
    void newSwitch(Switch* sw);

private slots:
    void pollTimeout();

private:
    unsigned c_poll_interval;
    QTimer* m_timer;
    SwitchManager* m_switch_manager;
    // port stats for each switch
    std::unordered_map<uint64_t, SwitchPortStats> switch_stats;
    OFTransaction* pdescr;
};

