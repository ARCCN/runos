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

using namespace runos;

REGISTER_APPLICATION(SimpleLearningSwitch, {"controller", ""})

void SimpleLearningSwitch::init(Loader *loader, const Config &)
{
    Controller* ctrl = Controller::get(loader);
    ctrl->registerHandler("forwarding", [](SwitchConnectionPtr) {
        // MAC -> port mapping for EVERY switch
        std::unordered_map<ethaddr, uint32_t> seen_port;
        const auto ofb_in_port = oxm::in_port();
        const auto ofb_eth_src = oxm::eth_src();
        const auto ofb_eth_dst = oxm::eth_dst();

        return [=](Packet& pkt, FlowPtr, Decision decision) mutable {
            // Learn on packet data
            ethaddr src_mac = pkt.load(ofb_eth_src);
            // Forward by packet destination
            ethaddr dst_mac = pkt.load(ofb_eth_dst);

            if (is_broadcast(src_mac)) {
                DLOG(WARNING) << "Broadcast source address, dropping";
                return decision.drop().return_();
            }

            seen_port[src_mac] = packet_cast<TraceablePacket>(pkt)
                                .watch(ofb_in_port);

            // forward
            auto it = seen_port.find(dst_mac);

            if (it != seen_port.end()) {
                return decision.unicast(it->second)
                               .idle_timeout(std::chrono::seconds(60))
                               .hard_timeout(std::chrono::minutes(30));
            } else {
                auto ret = decision.broadcast();
                if (not is_broadcast(dst_mac)) {
                    LOG(INFO) << "Flooding for unknown address " << dst_mac;
                    ret.hard_timeout(std::chrono::seconds::zero());
                }
                return ret;
            }
        };
    });
}
