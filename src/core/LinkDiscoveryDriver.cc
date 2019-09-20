/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include "LinkDiscoveryDriver.hpp"

#include <runos/core/logging.hpp>
 
namespace runos {

void onSwitchUp::handle(drivers::DefaultDriver& driver) const { 
    of13::FlowMod fm;
    fm.table_id(sw->tables.admission);
    fm.add_oxm_field(new of13::EthType(LLDP_ETH_TYPE));
    of13::ApplyActions actions;
    actions.add_action(new of13::OutputAction(of13::OFPP_CONTROLLER,
                                              of13::OFPCML_NO_BUFFER));
    fm.add_instruction(actions);
    fm.flags(0);
    fm.idle_timeout(0);
    fm.hard_timeout(0);
    fm.command(of13::OFPFC_ADD);
    fm.priority(fm_prio);
    fm.cookie((1 << 16) + 0x11D0);

    sw->connection()->send(fm);
}

lldp_packet sendLLDP::cookPacket() const {
    lldp_packet lldp;

    lldp.src_mac = port->hw_addr().to_number();
    // lower 48 bits of datapath id represents switch MAC address
    lldp.chassis_id_sub_mac = sw->dpid();
    lldp.port_id_sub_component = port->number();
    lldp.ttl_seconds = app.pollInterval();
    lldp.dpid_data   = sw->dpid();

    return lldp;
}

void sendLLDP::handle(drivers::DefaultDriver& driver) const { 

    if (sw->property("local_port", of13::OFPP_LOCAL) == port->number() ||
        port->link_down() || port->number() > of13::OFPP_MAX) {
        return;
    }

    of13::OutputAction ofoutput(port->number(), of13::OFPCML_NO_BUFFER);
    of13::PacketOut po;

    lldp_packet lldp = cookPacket();
    po.data(&lldp, sizeof lldp);

    po.xid(xid);
    po.in_port(of13::OFPP_CONTROLLER);

    if (app.outputQueueId() >= 0) {
        of13::SetQueueAction queue(app.outputQueueId());
        po.add_action(queue);
    }
    po.add_action(ofoutput);

    VLOG(5) << "Sending LLDP packet to " << port->name();
    // Send packet 3 times to prevent drops
    sw->connection()->send(po);
    sw->connection()->send(po);
    sw->connection()->send(po);
}

void sendLLDP::sendLLDPtoPorts() {
    for (auto &pr: sw->ports()) {
        if (pr->link_down()) {
            continue;
        }
        port = pr;
        sw->handle(*this);
        
    }
}



} //runos
