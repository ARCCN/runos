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

#include "ArpHandler.hh"

#include "api/Packet.hh"
#include "api/PacketMissHandler.hh"
#include "api/TraceablePacket.hh"

#include "types/ethaddr.hh"
#include "types/IPv4Addr.hh"

#include "SwitchConnection.hh"
#include "Controller.hh"
#include "HostManager.hh"

using namespace boost::endian;

struct Reply{
    big_uint48_t dst;
    big_uint48_t src;
    big_uint16_t type = 0x0806;
    struct Arp
    {
        big_uint16_t htype = 0x0001;
        big_uint16_t ptype = 0x0800;
        big_uint8_t hlen = 0x6;
        big_uint8_t plen = 0x4;
        big_uint16_t oper = 0x0002;
        big_uint48_t sha;
        big_uint32_t spa;
        big_uint48_t tha;
        big_uint32_t tpa;
    } arp;
};

constexpr auto ARP_ETH_TYPE = 0x0806;
constexpr auto ARP_REQUEST = 1;
constexpr auto ARP_REPLY = 2;

REGISTER_APPLICATION(ArpHandler, {"controller","switch-manager", "host-manager", ""})

//TODO : deal with it application
void ArpHandler::init(Loader *loader, const Config& config)
{
        Controller* ctrl = Controller::get(loader);
        host_manager = HostManager::get(loader);
        ctrl->registerHandler("arp-handler",
            [=](SwitchConnectionPtr connection) {
                const auto ofb_in_port = oxm::in_port();
                const auto ofb_eth_type = oxm::eth_type();
                const auto ofb_arp_op = oxm::arp_op();
                const auto ofb_arp_tpa = oxm::arp_tpa();
                const auto ofb_arp_spa = oxm::arp_spa();
                const auto ofb_arp_sha = oxm::arp_sha();
                return [=](Packet& pkt, FlowPtr, Decision decision){
                    auto tpkt = packet_cast<TraceablePacket>(pkt);
                    if (pkt.test(ofb_eth_type == ARP_ETH_TYPE) && tpkt.test(ofb_arp_op == ARP_REQUEST)) {
                        IPv4Addr arp_tpa = tpkt.watch(ofb_arp_tpa);
                        Host *target = host_manager->getHost(arp_tpa);
                        if (target) {
                            // arp_tha is declared above
                            ethaddr arp_tha = target->mac();
                            ethaddr arp_sha = tpkt.watch(ofb_arp_sha);
                            IPv4Addr arp_spa = tpkt.watch(ofb_arp_spa);
                            Reply reply;
                            reply.dst = arp_sha.to_number();
                            reply.src = arp_tha.to_number();
                            reply.arp.sha = arp_tha.to_number();
                            reply.arp.spa = arp_tpa.to_number();
                            reply.arp.tha = arp_sha.to_number();
                            reply.arp.tpa = arp_spa.to_number();

                            uint8_t* data = (uint8_t *)&(reply);
                            of13::PacketOut out;
                            out.buffer_id(OFP_NO_BUFFER);
                            out.data(data, sizeof(reply));
                            of13::OutputAction action(uint32_t(tpkt.watch(ofb_in_port)), 0);
                            out.add_action(action);

                            connection->send(out);
                            VLOG(10) << "We said " << arp_sha
                                         << " (" << arp_spa
                                         << ") that IP = "
                                         << arp_tpa << " has " << arp_tha;
                            return decision
                            .idle_timeout(std::chrono::seconds::zero())
                            .drop() // To controller with arpheader lenght
                            .return_();
                        }
                        return decision.idle_timeout(std::chrono::seconds::zero());
                    }
                    return decision;
                };
            });
}
