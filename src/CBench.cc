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

#include "CBench.hh"
#include "Controller.hh"

#include "api/Packet.hh"
#include "api/PacketMissHandler.hh"
#include "types/ethaddr.hh"
#include "oxm/openflow_basic.hh"
#include "Flow.hh"

using namespace runos;

REGISTER_APPLICATION(CBench, {"controller", ""})

void CBench::init(Loader* loader, const Config&)
{
    Controller::get(loader)->registerHandler("cbench",
    [=](SwitchConnectionPtr) {
        const auto ofb_eth_dst = oxm::eth_dst();

        return [=](Packet& pkt, FlowPtr, Decision decision) {
            uint32_t out_port = ethaddr(pkt.load(ofb_eth_dst))
                               .to_octets()[5] % 3;
            return decision.unicast(out_port)
                           .return_();
        };
    });
}
