#include "PacketParser.hh"

#include <cstring>
#include <algorithm>

#include <boost/endian/arithmetic.hpp>
#include <boost/exception/error_info.hpp>

#include <fluid/of13msg.hh>

#include "types/exception.hh"

using namespace boost::endian;

namespace runos {

typedef boost::error_info< struct tag_oxm_ns, unsigned >
    errinfo_oxm_ns;
typedef boost::error_info< struct tag_oxm_field, unsigned >
    errinfo_oxm_field;

using ofb = of::oxm::basic_match_fields;

struct ethernet_hdr {
    big_uint48_t dst;
    big_uint48_t src;
    big_uint16_t type;

    size_t header_length() const
    { return sizeof(*this); }
};
static_assert(sizeof(ethernet_hdr) == 14, "");

struct dot1q_hdr {
    big_uint48_t dst;
    big_uint48_t src;
    big_uint16_t tpid;
    union {
        big_uint16_t tci;
        struct {
            uint8_t pcp:3;
            bool dei:1;
            uint16_t vid_unordered:12;
        };
    };
    big_uint16_t type;
    size_t header_length() const
    { return sizeof(*this); }
};
static_assert(sizeof(dot1q_hdr) == 18, "");

struct ipv4_hdr {
    uint8_t ihl:4; // TODO: learn ipv4 protocol
    uint8_t version:4;
    uint8_t dscp:6;
    uint8_t ecn:2;
    big_uint16_t total_len;
    big_uint16_t identification;
    struct {
        uint16_t flags:3;
        uint16_t fragment_offset_unordered:13;
    };

    big_uint8_t ttl;
    big_uint8_t protocol;
    big_uint16_t checksum;
    big_uint32_t src;
    big_uint32_t dst;

    size_t header_length() const
    { return ihl * 4; }
};
static_assert(sizeof(ipv4_hdr) == 20, "");

struct ipv6_hdr {
    union{
        uint32_t startheader;
        // !!! TODO
        /*struct{
            uint8_t version:4;
            uint8_t Traffic_class:8;
            uint32_t FlowLabel:20;
        };*/
    };
    big_uint16_t payload_lenght;
    uint8_t protocol;
    uint8_t HopLimit;
    big_uint64_t src1;
    big_uint64_t src2;
    big_uint64_t dst1;
    big_uint64_t dst2;
    size_t header_length() const
    { return sizeof(ipv6_hdr); }
};
static_assert(sizeof(ipv6_hdr) == 40, "");

struct tcp_hdr {
    big_uint16_t src;
    big_uint16_t dst;
    big_uint32_t seq_no;
    big_uint32_t ack_no;
    uint8_t data_offset:4;
    uint8_t :3; // reserved
    bool NS:1;
    bool CWR:1;
    bool ECE:1;
    bool URG:1;
    bool ACK:1;
    bool PSH:1;
    bool RST:1;
    bool SYN:1;
    bool FIN:1;
    big_uint16_t window_size;
    big_uint16_t checksum;
    big_uint16_t urgent_pointer;
};
static_assert(sizeof(tcp_hdr) == 20, "");

struct udp_hdr {
    big_uint16_t src;
    big_uint16_t dst;
    big_uint16_t length;
    big_uint16_t checksum;
};
static_assert(sizeof(udp_hdr) == 8, "");

struct arp_hdr {
    big_uint16_t htype;
    big_uint16_t ptype;
    big_uint8_t hlen;
    big_uint8_t plen;
    big_uint16_t oper;
    big_uint48_t sha;
    big_uint32_t spa;
    big_uint48_t tha;
    big_uint32_t tpa;
};
static_assert(sizeof(arp_hdr) == 28, "");

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    big_uint16_t checksum;
};
static_assert(sizeof(icmp_hdr) == 4, "");

// TODO: make it more safe
// add size-checking of passed oxm type against field size
void PacketParser::bind(binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to bind already binded field") <<
                    errinfo_oxm_field(id));
        }
        bindings.at(id) = binding.second;
    }
}

void PacketParser::rebind(binding_list new_bindings)
{
    for (const auto& binding : new_bindings) {
        auto id = static_cast<size_t>(binding.first);
        if (!bindings.at(id)) {
            RUNOS_THROW(
                    invalid_argument() <<
                    errinfo_msg("Trying to rebind unbinded field") <<
                    errinfo_oxm_field(id));
        }
        bindings.at(id) = binding.second;
    }
}

void PacketParser::parse_l2(uint8_t* data, size_t data_len)
{
    uint16_t type = 0; // For adequate compile
    size_t len = 0; // For adequate compile
    if (sizeof(ethernet_hdr) <= data_len) {
        eth = reinterpret_cast<ethernet_hdr*>(data);
        if (eth->type == 0x8100){
            if (sizeof(dot1q_hdr) <= data_len ){
                dot1q = reinterpret_cast<dot1q_hdr*>(data);
                bind({
                    { ofb::ETH_TYPE, &dot1q->type },
                    { ofb::ETH_SRC, &dot1q->src },
                    { ofb::ETH_DST, &dot1q->dst },
                    { ofb::VLAN_VID, &dot1q->tci } //fix this
                });
                type = dot1q->type;
                len = dot1q->header_length();
            }
        } else {
            bind({
                { ofb::ETH_TYPE, &eth->type },
                { ofb::ETH_SRC, &eth->src },
                { ofb::ETH_DST, &eth->dst }
            });
            type = eth->type;
            len = eth->header_length();
        }
    parse_l3(type,
             static_cast<uint8_t*>(data) + len,
             data_len - len);
    }
}

void PacketParser::parse_l3(uint16_t eth_type, uint8_t* data, size_t data_len)
{
    switch (eth_type) {
    case 0x0800: // ipv4
        if (sizeof(ipv4_hdr) <= data_len) {
            ipv4 = reinterpret_cast<ipv4_hdr*>(data);
            bind({
                { ofb::IP_PROTO, &ipv4->protocol },
                { ofb::IPV4_SRC, &ipv4->src },
                { ofb::IPV4_DST, &ipv4->dst }
            });

            if (data_len > ipv4->header_length()) {
                parse_l4(ipv4->protocol,
                         data + ipv4->header_length(),
                         data_len - ipv4->header_length());
            }
        }
        break;
    case 0x0806: // arp
        if (sizeof(arp_hdr) <= data_len) {
            arp = reinterpret_cast<arp_hdr*>(data);
            if (arp->htype != 1 ||
                arp->ptype != 0x0800 ||
                arp->hlen != 6 ||
                arp->plen != 4)
                break;

            bind({
                { ofb::ARP_OP, &arp->oper },
                { ofb::ARP_SHA, &arp->sha },
                { ofb::ARP_THA, &arp->tha },
                { ofb::ARP_SPA, &arp->spa },
                { ofb::ARP_TPA, &arp->tpa }
            });
        }
        break;
    case 0x86dd: // ipv6
        if (sizeof(ipv6_hdr) <= data_len){
            ipv6 = reinterpret_cast<ipv6_hdr*>(data);
            bind({
                { ofb::IPV6_SRC, &ipv6->src1 },
                { ofb::IPV6_DST, &ipv6->dst1 },
                { ofb::IP_PROTO, &ipv6->protocol }
            });
        }
        if (data_len > ipv6->header_length()){
            parse_l4(ipv6->protocol,
                     data + ipv6->header_length(),
                     data_len - ipv6->header_length());
        }
        break;
    }
}

void PacketParser::parse_l4(uint8_t protocol, uint8_t* data, size_t data_len)
{
    switch (protocol) {
    case 0x06: // tcp
        if (sizeof(tcp_hdr) <= data_len) {
            tcp = reinterpret_cast<tcp_hdr*>(data);
            bind({
                { ofb::TCP_SRC, &tcp->src },
                { ofb::TCP_DST, &tcp->dst }
            });
        }
        break;
    case 0x11: // udp
        if (sizeof(udp_hdr) <= data_len) {
            udp = reinterpret_cast<udp_hdr*>(data);
            bind({
                { ofb::UDP_SRC, &udp->src },
                { ofb::UDP_DST, &udp->dst }
            });
        }
        break;
    case 0x01: // icmp
        if (sizeof(icmp_hdr) <= data_len) {
            icmp = reinterpret_cast<icmp_hdr*>(data);
            bind({
                { ofb::ICMPV4_TYPE, &icmp->type },
                { ofb::ICMPV4_CODE, &icmp->code }
            });
        }
        break;
    }
}

PacketParser::PacketParser(fluid_msg::of13::PacketIn& pi)
    : data(static_cast<uint8_t*>(pi.data()))
    , data_len(pi.data_len())
    , in_port(pi.match().in_port()->value())
{
    bindings.fill(nullptr);
    bind({
        { ofb::IN_PORT, &in_port }
    });

    if (data) {
        parse_l2(data, data_len);
    }
}

uint8_t* PacketParser::access(oxm::type t) const
{
    if (t.ns() != unsigned(of::oxm::ns::OPENFLOW_BASIC)) {
        RUNOS_THROW(
                out_of_range() <<
                errinfo_msg("Unsupported oxm namespace") <<
                errinfo_oxm_ns(t.ns()));
    }

    if (t.id() >= bindings.size() || !bindings[t.id()]) {
        RUNOS_THROW(
                out_of_range() <<
                errinfo_msg("Unsupported oxm field") <<
                errinfo_oxm_ns(t.ns()) <<
                errinfo_oxm_field(t.id()));
    }

    return (uint8_t*) bindings[t.id()];
}

oxm::field<> PacketParser::load(oxm::mask<> mask) const
{
    auto value_bits = bits<>(mask.type().nbits(), access(mask.type()));
    return oxm::value<>{ mask.type(), value_bits } & mask;
}

void PacketParser::modify(oxm::field<> patch)
{
    oxm::field<> updated =
        PacketParser::load(oxm::mask<>(patch.type())) >> patch;
    updated.value_bits().to_buffer(access(patch.type()));
}

size_t PacketParser::serialize_to(size_t buffer_size, void* buffer) const
{
    size_t copied = std::min(data_len, buffer_size);
    std::memmove(buffer, data, copied);
    return copied;
}

size_t PacketParser::total_bytes() const
{
    return data_len;
}

} // namespace runos
