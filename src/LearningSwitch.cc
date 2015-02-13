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

#include "LearningSwitch.hh"
#include "Controller.hh"
#include "Topology.hh"
#include "Switch.hh"

REGISTER_APPLICATION(LearningSwitch, {"controller", "switch-manager", "topology", ""})

void LearningSwitch::init(Loader *loader, const Config &config)
{
    Controller* ctrl = Controller::get(loader);
    topology = Topology::get(loader);
    switch_manager = SwitchManager::get(loader);

    ctrl->registerHandler(this);
}

std::pair<bool, switch_and_port>
LearningSwitch::findAndLearn(const switch_and_port& where,
                             const EthAddress& eth_src,
                             const EthAddress& eth_dst)
{
    std::pair<bool, switch_and_port> ret;
    ret.first = false;

    auto it_src = db.find(eth_src);
    auto it_dst = db.find(eth_dst);

    if (it_dst != db.end()) {
        ret.first = true;
        ret.second = it_dst->second;
    }

    // Learn new MAC or update old entry
    // TODO: implement migrations
    if (it_src == db.end()) {
        db_lock.lock();
        db[eth_src] = where;
        db_lock.unlock();

        LOG(INFO) << eth_src.to_string() << " seen at "
            << FORMAT_DPID << where.dpid << ':' << where.port;
    }

    return ret;
}

OFMessageHandler::Action LearningSwitch::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    static EthAddress broadcast("ff:ff:ff:ff:ff:ff");

    // Get required fields
    EthAddress eth_src = flow->pkt()->readEthSrc();
    uint32_t   in_port = flow->loadInPort();
    EthAddress eth_dst = flow->loadEthDst();
    uint32_t out_port = 0;

    if (eth_src == broadcast) {
        DLOG(WARNING) << "Broadcast source address, dropping";
        return Stop;
    }

    // Observe our point
    Switch* sw = app->switch_manager->getSwitch(ofconn);
    if (sw) {
        switch_and_port where;
        where.dpid = sw->id();
        where.port = in_port;

        // Learn
        bool target_found;
        switch_and_port target;
        std::tie(target_found, target)
                = app->findAndLearn(where, eth_src, eth_dst);

        // Find path
        if (target_found) {
            if (where.dpid != target.dpid) {
                data_link_route route = app->topology->computeRoute(where.dpid, target.dpid);
                if (route.size() > 0) {
                    out_port = route[0].port;
                } else {
                    LOG(WARNING) << "Path between " << FORMAT_DPID << where.dpid 
                        << " and " << FORMAT_DPID << target.dpid << " not found";
                }
            } else {
                out_port = target.port;
            }
        }
    }

    // Forward
    if (out_port) {
        flow->idleTimeout(60);
        flow->timeToLive(5*60);
        flow->add_action(new of13::OutputAction(out_port, 0));
        return Continue;
    } else {
        DVLOG(5) << "Flooding for address " << eth_dst.to_string();

        if (eth_dst == broadcast) {
            flow->idleTimeout(0);
            flow->timeToLive(0);
        } else {
            flow->setFlags(Flow::Disposable);
        }

        // Should be replaced with STP ports
        flow->add_action(new of13::OutputAction(of13::OFPP_ALL, 128));
        return Continue;
    }
}

bool LearningSwitch::isPrereq(const std::string &name) const
{
    return name == "link-discovery";
}
