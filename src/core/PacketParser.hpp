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

#include "api/SerializablePacket.hpp"
#include "openflow/common.hh"
#include <runos/core/safe_ptr.hpp>

#include <boost/endian/arithmetic.hpp>

#include <array>
#include <initializer_list>
#include <ostream>
#include <cstddef>
#include <cstdint>


namespace fluid_msg {
namespace of13 {
class PacketIn;
}
}

namespace runos {

class PacketParser final : public SerializablePacket {
    // buffer
    uint8_t* data;
    size_t data_len;
    boost::endian::big_uint32_t in_port;
    bool vlan_tagged;

    // bindings
    typedef std::array<void*, 40> bindings_arr;
    bindings_arr bindings;

    // replace with
    safe_ptr<struct ethernet_hdr> eth;
    safe_ptr<struct dot1q_hdr> dot1q;
    safe_ptr<struct ipv4_hdr> ipv4;
    safe_ptr<struct tcp_hdr> tcp;
    safe_ptr<struct udp_hdr> udp;
    safe_ptr<struct arp_hdr> arp;

    void parse_l2(uint8_t* data, size_t data_len);
    void parse_l3(uint16_t eth_type, uint8_t* data, size_t data_len);
    void parse_l4(uint8_t protocol, uint8_t* data, size_t data_len);

    using binding_list =
        std::initializer_list<std::pair<of::oxm::basic_match_fields, void*>>;

    void bind(binding_list bindings);
    void rebind(binding_list bindings);
    uint8_t* access(oxm::type t) const;

public:
    PacketParser(fluid_msg::of13::PacketIn& pi);

    oxm::field<> load(oxm::mask<> mask) const override;
    void modify(oxm::field<> patch) override;
    bool vlanTagged() override;

    size_t total_bytes() const override;
    size_t serialize_to(size_t buffer_size, void* buffer) const override;
};

} // namespace runos
