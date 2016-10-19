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

    //bindings
    typedef std::array<void*, 40> bindings_arr;
    bindings_arr bindings;

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

    using binding_list =
        std::initializer_list<std::pair<of::oxm::basic_match_fields, void*>>;

    void bind(binding_list bindings);
    void rebind(binding_list bindings);
    uint8_t* access(oxm::type t) const;

public:
    PacketParser(fluid_msg::of13::PacketIn& pi);

    oxm::field<> load(oxm::mask<> mask) const override;
    void modify(oxm::field<> patch) override;

    size_t total_bytes() const override;
    size_t serialize_to(size_t buffer_size, void* buffer) const override;
};

} // namespace runos
