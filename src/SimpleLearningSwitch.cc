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

#include "SimpleLearningSwitch.hh"

#include <unordered_map>

#include "api/Packet.hh"
#include "api/PacketMissHandler.hh"
#include "api/TraceablePacket.hh"
#include "oxm/openflow_basic.hh"
#include "types/ethaddr.hh"

#include "Flow.hh"
#include "Controller.hh"
#include "Maple.hh"
#include "STP.hh"

using namespace runos;

REGISTER_APPLICATION(SimpleLearningSwitch, {"controller","maple", "stp", ""})

void SimpleLearningSwitch::init(Loader *loader, const Config &)
{
    auto maple = Maple::get(loader);
    // MAC -> port mapping for EVERY switch
    std::unordered_map<uint64_t,
        std::unordered_map<ethaddr, uint32_t>> seen_port;
    const auto ofb_in_port = oxm::in_port();
    const auto ofb_eth_src = oxm::eth_src();
    const auto ofb_eth_dst = oxm::eth_dst();
    const auto switch_id = oxm::switch_id();

    maple->registerHandler("forwarding",
        [=](Packet& pkt, FlowPtr, Decision decision) mutable {
            // Learn on packet data
            uint64_t dpid = pkt.load(switch_id);
            ethaddr src_mac = pkt.load(ofb_eth_src);
            // Forward by packet destination
            ethaddr dst_mac = pkt.load(ofb_eth_dst);

            if (is_broadcast(src_mac)) {
                DLOG(WARNING) << "Broadcast source address, dropping";
                return decision.drop().return_();
            }

            seen_port[dpid][src_mac] = packet_cast<TraceablePacket>(pkt)
                                .watch(ofb_in_port);

            // forward
            auto it = seen_port[dpid].find(dst_mac);

            if (it != seen_port[dpid].end()) {
                return decision.unicast(it->second)
                               .idle_timeout(std::chrono::seconds(60))
                               .hard_timeout(std::chrono::minutes(30));
            } else {
                if (not is_broadcast(dst_mac)) {
                    LOG(INFO) << "Flooding for unknown address " << dst_mac;
                    return decision.custom(std::make_shared<STP::Decision>())
                            .idle_timeout(std::chrono::seconds::zero());
                }
                return decision.custom(std::make_shared<STP::Decision>());
            }
    });
}
