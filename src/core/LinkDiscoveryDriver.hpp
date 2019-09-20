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

#pragma once

#include "LLDP.hpp"
#include "api/OFDriver.hpp"
#include "api/Switch.hpp"
#include "LinkDiscovery.hpp"

#include <fluid/of13/of13match.hh>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

namespace runos {

using namespace boost::endian;
namespace of13 = fluid_msg::of13;
static const uint16_t fm_prio = 50000;
static constexpr uint32_t xid = 24500;

struct tagged_lldp_packet {
    big_uint16_t lldp_tlv_header(big_uint16_t type, big_uint16_t length) {
        return (type << big_uint16_t(9)) | length;
    }

    // WARN: Some NIC can drop packet with following dst mac address
    big_uint48_t dst_mac {0x0180c200000eULL};
    big_uint48_t src_mac;
    big_uint32_t vlan_field;
    big_uint16_t eth_type;

    big_uint16_t chassis_id_header {lldp_tlv_header(LLDP_CHASSIS_ID_TLV, 7)};
    big_uint8_t chassis_id_sub {LLDP_CHASSIS_ID_SUB_MAC};
    big_uint48_t chassis_id_sub_mac;

    big_uint16_t port_id_header {lldp_tlv_header(LLDP_PORT_ID_TLV, 5)};
    big_uint8_t port_id_sub {LLDP_PORT_ID_SUB_COMPONENT};
    big_uint32_t port_id_sub_component;

    big_uint16_t ttl_header {lldp_tlv_header(LLDP_TTL_TLV, 2)};
    big_uint16_t ttl_seconds;

    big_uint16_t dpid_header {lldp_tlv_header(127, 12)};
    big_uint24_t dpid_oui {0x0026e1}; // OpenFlow
    big_uint8_t  dpid_sub {0};
    big_uint64_t dpid_data;

    big_uint16_t end {0};
};
static_assert(sizeof(tagged_lldp_packet) == 54, "Unexpected alignment");

struct lldp_packet {
    big_uint16_t lldp_tlv_header(big_uint16_t type, big_uint16_t length) {
        return (type << big_uint16_t(9)) | length;
    }

    // WARN: Some NIC can drop packet with following dst mac address
    big_uint48_t dst_mac {0x0180c200000eULL};
    big_uint48_t src_mac;
    big_uint16_t eth_type {LLDP_ETH_TYPE};

    big_uint16_t chassis_id_header {lldp_tlv_header(LLDP_CHASSIS_ID_TLV, 7)};
    big_uint8_t chassis_id_sub {LLDP_CHASSIS_ID_SUB_MAC};
    big_uint48_t chassis_id_sub_mac;

    big_uint16_t port_id_header {lldp_tlv_header(LLDP_PORT_ID_TLV, 5)};
    big_uint8_t port_id_sub {LLDP_PORT_ID_SUB_COMPONENT};
    big_uint32_t port_id_sub_component;

    big_uint16_t ttl_header {lldp_tlv_header(LLDP_TTL_TLV, 2)};
    big_uint16_t ttl_seconds;

    big_uint16_t dpid_header {lldp_tlv_header(127, 12)};
    big_uint24_t dpid_oui {0x0026e1}; // OpenFlow
    big_uint8_t  dpid_sub {0};
    big_uint64_t dpid_data;

    big_uint16_t end {0};
};
static_assert(sizeof(lldp_packet) == 50, "Unexpected alignment");


class onSwitchUp : public drivers::Handler {
public:

    SwitchPtr sw;
    onSwitchUp(SwitchPtr _sw): sw(_sw)
    { }

    void handle(drivers::DefaultDriver& driver) const;
};

class sendLLDP : public drivers::Handler {
public:
    const LinkDiscovery& app;
    SwitchPtr sw;
    PortPtr port;

    sendLLDP(const LinkDiscovery& _app, SwitchPtr _sw): app(_app), sw(_sw)
    { }

    sendLLDP(const LinkDiscovery& _app, PortPtr _port): app(_app), port(_port)
    {  sw = port->switch_(); }


    lldp_packet cookPacket() const;

    void handle(drivers::DefaultDriver& driver) const;

    void sendLLDPtoPorts();
};

}//runos
