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

#include "STP.hh"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>

#include "Topology.hh"
#include "Controller.hh"
#include "SwitchConnection.hh"
REGISTER_APPLICATION(STP, {"controller", "switch-manager", "link-discovery", "topology", ""})

enum {
    FLOOD_GROUP = 0xf100d
};

void SwitchSTP::resetBroadcast()
{
    for (auto port : ports) {
        if (port.second->to_switch)
            unsetBroadcast(port.second->port_no);
    }
}

STPPorts SwitchSTP::getEnabledPorts()
{
    STPPorts result;

    for (auto port : ports) {
        if (port.second->broadcast)
            result.push_back(port.second->port_no);
    }
    return result;
}

void SwitchSTP::updateGroup()
{
    of13::GroupMod gm;
    gm.commmand(of13::OFPGC_MODIFY);
    gm.group_type(of13::OFPGT_ALL);
    gm.group_id(FLOOD_GROUP);
    STPPorts ports = getEnabledPorts();
    for (auto port : ports) {
        of13::Bucket b;
        b.watch_port(of13::OFPP_ANY);
        b.watch_group(of13::OFPG_ANY);
        //b.weight(0);
        b.add_action(new of13::OutputAction(port, 0));
        gm.add_bucket(b);
    }
    DVLOG(20) << "Update group in switch" << sw->id();
    sw->connection()->send(gm);
}

void SwitchSTP::clearGroup()
{
    if (sw){
        of13::GroupMod gm;
        gm.commmand(of13::OFPGC_DELETE);
        gm.group_type(of13::OFPGT_ALL);
        gm.group_id(FLOOD_GROUP);
        DVLOG(20) << "Clear group in switch " <<  sw->id();
        sw->connection()->send(gm);
    }
}

void SwitchSTP::installGroup()
{
    of13::GroupMod gm;
    gm.commmand(of13::OFPGC_ADD);
    gm.group_type(of13::OFPGT_ALL);
    gm.group_id(FLOOD_GROUP);
    STPPorts ports = getEnabledPorts();
    for (auto port : ports) {
        of13::Bucket b;
        b.watch_port(of13::OFPP_ANY);
        b.watch_group(of13::OFPG_ANY);
        //b.weight(0);
        b.add_action(new of13::OutputAction(port, 0));
        gm.add_bucket(b);
    }
    DVLOG(20) << "Install group in switch" << sw->id();
    sw->connection()->send(gm);
}

void SwitchSTP::setSwitchPort(uint32_t port_no, uint64_t dpid)
{
    ports.at(port_no)->to_switch = true;
    ports.at(port_no)->nextSwitch = parent->switch_list[dpid];
}

void STP::init(Loader* loader, const Config& config)
{
    QObject* ld = ILinkDiscovery::get(loader);
    auto subconfig = config_cd(config, "stp");
    poll_timeout = config_get(subconfig, "poll-timeout", 3);
    connect(ld, SIGNAL(linkDiscovered(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkDiscovered(switch_and_port, switch_and_port)));
    connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkBroken(switch_and_port, switch_and_port)));

    Controller *ctrl = Controller::get(loader);
    ctrl->registerFlood([this](uint64_t dpid){
        return new of13::GroupAction(FLOOD_GROUP);
    });
    SwitchManager* sw = SwitchManager::get(loader);
    connect(sw, &SwitchManager::switchDiscovered, this, &STP::onSwitchDiscovered);
    connect(sw, &SwitchManager::switchDown, this, &STP::onSwitchDown);
    connect(sw, &SwitchManager::switchUp, this, &STP::onSwitchUp);
    topo = Topology::get(loader);

    computed = false;
    timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(computeSpanningTree()));
    timer->start(poll_timeout * 1000);
}

STPPorts STP::getSTP(uint64_t dpid)
{
    if (switch_list.count(dpid) == 0 or not computed) {
        return {};
    }

    SwitchSTP* sw = switch_list[dpid];
    return sw->getEnabledPorts();
}

void STP::onLinkDiscovered(switch_and_port from, switch_and_port to)
{
    if (switch_list.count(from.dpid) == 0)
        return;

    if (switch_list.count(to.dpid) == 0)
        return;

    SwitchSTP* sw = switch_list[from.dpid];
    if (!sw->existsPort(from.port)) {
        Port* port = new Port(from.port);
        sw->ports[from.port] = port;
    }

    // port is disabled by default
    sw->unsetBroadcast(from.port);
    sw->updateGroup();

    sw->setSwitchPort(from.port, to.dpid);

    sw = switch_list[to.dpid];
    if (!sw->existsPort(to.port)) {
        Port* port = new Port(to.port);
        sw->ports[to.port] = port;
    }

    // port is disabled by default
    sw->unsetBroadcast(to.port);
    sw->updateGroup();

    sw->setSwitchPort(to.port, from.dpid);

    // recompute pathes for all switches
    computed = false;
}

void STP::onLinkBroken(switch_and_port from, switch_and_port to)
{
    // recompute pathes for all switches
    computed = false;
}

void STP::onSwitchDiscovered(Switch* dp)
{
    SwitchSTP* sw;
    sw = new SwitchSTP(dp, this);

    switch_list[dp->id()] = sw;

    connect(dp, &Switch::portUp, this, &STP::onPortUp);
}

void STP::onSwitchDown(Switch* dp)
{
    if (switch_list.count(dp->id()) > 0) {
        SwitchSTP* sw = switch_list[dp->id()];
        switch_list.erase(dp->id());
        delete sw;
    }
}

void STP::onSwitchUp(Switch* dp)
{
    if (switch_list.count(dp->id()) == 0) {
        SwitchSTP* sw = new SwitchSTP(dp, this);
        switch_list[dp->id()] = sw;
        connect(dp, &Switch::portUp, this, &STP::onPortUp);
    }
}

void STP::onPortUp(Switch *dp, of13::Port port)
{
    if (switch_list.count(dp->id()) > 0) {
        SwitchSTP* sw = switch_list[dp->id()];
        if ( !sw->existsPort(port.port_no()) && port.port_no() < of13::OFPP_MAX ) {
            Port* p = new Port(port.port_no());
            sw->ports[port.port_no()] = p;
        }
        sw->updateGroup();
    }
}

void STP::computeSpanningTree()
{
    static std::mutex compute_mutex;
    using namespace boost;
    using namespace topology;

    typedef graph_traits<TopologyGraph>::edge_descriptor edge_descriptor ;

    // clear spanning tree before new computing

    if (not computed){
        VLOG(20) << "Compute spanning tree";
        compute_mutex.lock();

        // all switch-switch ports are not broadcast
        for (auto sw : switch_list){
            sw.second->resetBroadcast();
        }
        spanning_tree.clear();

        auto boost_kruskal = [this](const topology::TopologyGraph& g){
            std::vector<edge_descriptor> result;
            kruskal_minimum_spanning_tree(g, std::back_inserter(result),
                    boost::weight_map( boost::get(&link_property::weight, g )) );
            for (auto& i : result){
                link_property l = g[i];
                spanning_tree[l.source.dpid].push_back(l.source.port);
                spanning_tree[l.target.dpid].push_back(l.target.port);
            }

        };
        topo->apply(boost_kruskal);

        // set ports broadcast in spanning tree
        for (auto& i : spanning_tree){
            SwitchSTP *sw = switch_list[i.first];
            for (auto& p : i.second){
                if (sw->existsPort(p)){
                    sw->setBroadcast(p);
                    VLOG(10) << i.first << ":" << p << " added in spanning tree";
                }
            }
            // update bucket groups
            sw->updateGroup();
        }

        computed = true;

        compute_mutex.unlock();
    }
}

