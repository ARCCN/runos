#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <initializer_list>
#include <ostream>

#include <boost/endian/arithmetic.hpp>

#include "api/SerializablePacket.hh"
#include "types/checked_ptr.hh"
#include "openflow/common.hh"

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
    boost::endian::big_uint64_t switch_id;

    //ofb_bindings
    typedef std::array<void*, 40> ofb_bindings_arr;
    ofb_bindings_arr ofb_bindings;

    //non openflow bindings
    typedef std::array<void*, 20> nonof_bindings_arr;
    nonof_bindings_arr nonof_bindings;

    // replace with
    checked_ptr<struct ethernet_hdr> eth;
    checked_ptr<struct ipv4_hdr> ipv4;
    checked_ptr<struct ipv6_hdr> ipv6;
    checked_ptr<struct tcp_hdr> tcp;
    checked_ptr<struct udp_hdr> udp;
    checked_ptr<struct arp_hdr> arp;
    checked_ptr<struct dot1q_hdr> dot1q;
    checked_ptr<struct icmp_hdr> icmp;

    void parse_l2(uint8_t* data, size_t data_len);
    void parse_l3(uint16_t eth_type, uint8_t* data, size_t data_len);
    void parse_l4(uint8_t protocol, uint8_t* data, size_t data_len);

    using ofb_binding_list =
        std::initializer_list<std::pair<of::oxm::basic_match_fields, void*>>;

    using nonof_binding_list =
        std::initializer_list<std::pair<of::oxm::non_openflow_fields, void*>>;


    // openflow basic bindings
    void bind(ofb_binding_list ofb_bindings);
    void rebind(ofb_binding_list ofb_bindings);

    // non openflow bindings
    void bind(nonof_binding_list nonof_bindings);
    void rebind(nonof_binding_list nonof_bindings);

    uint8_t* access(oxm::type t) const;

public:
    PacketParser(fluid_msg::of13::PacketIn& pi, uint64_t from_dpid);

    oxm::field<> load(oxm::mask<> mask) const override;
    void modify(oxm::field<> patch) override;

    size_t total_bytes() const override;
    size_t serialize_to(size_t buffer_size, void* buffer) const override;
};

} // namespace runos
