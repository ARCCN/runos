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

#include <tins/arp.h>
#include <tins/ethernetII.h>
#include "Controller.hh"
#include "HostManager.hh"

constexpr auto ARP_ETH_TYPE = 0x0806;
constexpr auto ARP_REQUEST = 1;
constexpr auto ARP_REPLY = 2;

REGISTER_APPLICATION(ArpHandler, {"controller","switch-manager", "host-manager", ""})

void ArpHandler::init(Loader *loader, const Config& config)
{
        Controller* ctrl = Controller::get(loader);
        host_manager = HostManager::get(loader);
        ctrl->registerHandler(this);
}

OFMessageHandler::Action ArpHandler::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{     
    if (flow->pkt()->readEthType() == ARP_ETH_TYPE && flow->pkt()->readARPOp() == ARP_REQUEST) {
        IPAddress ip = flow->pkt()->readARPTPA();
        Host* target = app->host_manager->getHost(ip);
        if (target) {
            flow->setFlags(Flow::Disposable);

            Tins::EthernetII arp;
            arp = Tins::ARP::make_arp_reply(Tins::IPv4Address(flow->pkt()->readARPSPA().getIPv4()), \
                Tins::IPv4Address(ip.getIPv4()), flow->pkt()->readARPSHA().to_string(), target->mac());

            Tins::PDU::serialization_type ser = arp.serialize();
            uint8_t* data = &(ser[0]);
            of13::PacketOut out;
            out.buffer_id(OFP_NO_BUFFER);
            out.data(data, arp.size());
            of13::OutputAction action(flow->pkt()->readInPort(), 0);
            out.add_action(action);

            uint8_t* buffer = out.pack();
            ofconn->send(buffer, out.length());
            OFMsg::free_buffer(buffer);

            DVLOG(10) << "We said " << flow->pkt()->readARPSHA().to_string()
                         << " (" << Tins::IPv4Address(flow->pkt()->readARPSPA().getIPv4()).to_string()
                         << ") that IP = "
                         << Tins::IPv4Address(ip.getIPv4()).to_string() << " has " << target->mac();

            return Stop;

        }
    }

    return Continue;
}
