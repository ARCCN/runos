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

#include <QTimer>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "ILinkDiscovery.hh"
#include "Switch.hh"
#include "OFTransaction.hh"

typedef std::vector<uint32_t> STPPorts;

struct Port {
    uint32_t port_no;
    // this port is broadcast or not
    bool broadcast;
    // it means that port is between two switches
    bool to_switch;
    class SwitchSTP* nextSwitch;

    Port(uint32_t _port_no, bool _b = true):
            port_no(_port_no), broadcast(_b), to_switch(false), nextSwitch(nullptr) {}
};

class STP;

class SwitchSTP : public QObject {
    Q_OBJECT
public:
    Switch* sw;
    STP* parent;
    std::unordered_map<uint32_t, Port*> ports;

    SwitchSTP() = delete;
    SwitchSTP(Switch* _sw, STP* stp):
        sw(_sw),
        parent(stp)
        {
            clearGroup();
            installGroup();
        }

    bool existsPort(uint32_t port_no) { return (ports.count(port_no) > 0 ? true : false); }
    void unsetBroadcast(uint32_t port_no) { ports.at(port_no)->broadcast = false; }
    void setBroadcast(uint32_t port_no) { ports.at(port_no)->broadcast = true; }
    void setSwitchPort(uint32_t port_no, uint64_t dpid);
    void resetBroadcast();
    void updateGroup();

    STPPorts getEnabledPorts();
private:
    void clearGroup();
    void installGroup();
    friend STP;
};

/**
 * @brief Implementation of Spanning Tree
 *
 * This application register function which implement flood action, and prevents
 * loops and storm of broadcast packets, by disabling some ports
 *
 * This application computes mininal spanning tree by kruskal alghorithm
 */
class STP : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(STP, "stp")
public:
    void init(Loader* loader, const Config& config) override;
/**
 * get enabling for flooding ports
 *
 * @param dpid id of switch
 * @return Ports
 */
    STPPorts getSTP(uint64_t dpid);

protected slots:
    void onLinkDiscovered(switch_and_port from, switch_and_port to);
    void onLinkBroken(switch_and_port from, switch_and_port to);
    void onSwitchDiscovered(Switch* dp);
    void onSwitchDown(Switch* dp);
    void onSwitchUp(Switch* dp);
    void onPortUp(Switch* dp, of13::Port port);
    void computeSpanningTree();

private:
    unsigned int poll_timeout;

    std::unordered_map<uint64_t, SwitchSTP*> switch_list;
    class Topology* topo;

    friend class SwitchSTP;

    std::unordered_map<uint64_t, STPPorts> spanning_tree;

    bool computed;
    QTimer *timer;
};
