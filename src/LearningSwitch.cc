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

#include <mutex>
#include <unordered_map>
#include <boost/optional.hpp>
#include <boost/thread.hpp>

#include "api/Packet.hh"
#include "api/PacketMissHandler.hh"
#include "api/TraceablePacket.hh"
#include "types/ethaddr.hh"
#include "oxm/openflow_basic.hh"

#include "Controller.hh"
#include "Topology.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"

REGISTER_APPLICATION(LearningSwitch, {"controller", "topology", ""})

using namespace runos;

class HostsDatabase {
    boost::shared_mutex mutex;
    std::unordered_map<ethaddr, switch_and_port> db;

public:
    void learn(uint64_t dpid, uint32_t in_port, ethaddr mac)
    {
        if (is_broadcast(mac)) { // should we test here??
            DLOG(WARNING) << "Broadcast source address detected";
            return;
        }

        VLOG(5) << mac << " seen at " << dpid << ':' << in_port;
        {
            boost::unique_lock< boost::shared_mutex > lock(mutex);
            db.emplace(mac, switch_and_port{dpid, in_port});
        }
    }

    boost::optional<switch_and_port> query(ethaddr mac)
    {
        boost::shared_lock< boost::shared_mutex > lock(mutex);

        auto it = db.find(mac);
        if (it != db.end())
            return it->second;
        else
            return boost::none;
    }
};

void LearningSwitch::init(Loader *loader, const Config &)
{
    Controller* ctrl = Controller::get(loader);
    auto topology = Topology::get(loader);
    auto db = std::make_shared<HostsDatabase>();

    ctrl->registerHandler("forwarding",
    [=](SwitchConnectionPtr connection) {
        const auto ofb_in_port = oxm::in_port();
        const auto ofb_eth_src = oxm::eth_src();
        const auto ofb_eth_dst = oxm::eth_dst();

        return [=](Packet& pkt, FlowPtr, Decision decision) {
            // Get required fields
            ethaddr dst_mac = pkt.load(ofb_eth_dst);

            db->learn(connection->dpid(),
                      pkt.load(ofb_in_port),
                      packet_cast<TraceablePacket>(pkt).watch(ofb_eth_src));

            auto target = db->query(dst_mac);

            // Forward
            if (target) {
                auto route = topology
                             ->computeRoute(connection->dpid(), target->dpid);

                if (route.size() > 0) {
                    DVLOG(10) << "Forwarding packet from "
                              << connection->dpid()
                              << ':' << uint32_t(pkt.load(ofb_in_port))
                              << " via port " << route[0].port
                              << " to " << ethaddr(pkt.load(ofb_eth_dst))
                              << " seen at " << target->dpid;
                    // unicast via core
                    return decision.unicast(route[0].port)
                                   .idle_timeout(std::chrono::seconds(60))
                                   .hard_timeout(std::chrono::minutes(30));
                } else if (connection->dpid() == target->dpid) {
                    DVLOG(10) << "Forwarding packet from "
                              << connection->dpid()
                              << ':' << uint32_t(pkt.load(ofb_in_port))
                              << " via port " << target->port;
                    // send through core border
                    return decision.unicast(target->port)
                                   .idle_timeout(std::chrono::seconds(60))
                                   .hard_timeout(std::chrono::minutes(30));
                } else {
                    LOG(WARNING)
                        << "Path from " << connection->dpid()
                        << " to " << target->dpid << " not found";
                    return decision.drop()
                                   .idle_timeout(std::chrono::seconds(30))
                                   .hard_timeout(std::chrono::seconds(60))
                                   .return_();
                }
            } else {
                if (not is_broadcast(dst_mac)) {
                    VLOG(5) << "Flooding for unknown address " << dst_mac;
                    return decision.broadcast()
                                   .idle_timeout(std::chrono::seconds::zero());
                }
                return decision.broadcast();
            }
        };
    });
}
