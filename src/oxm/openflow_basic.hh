#pragma once

#include <cstdint>

#include "openflow/common.hh"
#include "types/ethaddr.hh"
#include "types/IPv4Addr.hh"
#include "types/IPv6Addr.hh"
#include "type.hh"

namespace runos {
namespace oxm {

// TODO: unmaskable<type>

template< class Final,
          of::oxm::basic_match_fields ID,
          size_t NBITS,
          class ValueType,
          class MaskType = ValueType,
          bool HASMASK = false >
using define_ofb_type =
    define_type< Final
               , uint16_t(of::oxm::ns::OPENFLOW_BASIC)
               , uint8_t(ID)
               , NBITS, ValueType, MaskType, HASMASK >;

struct in_port : define_ofb_type
     < in_port, of::oxm::basic_match_fields::IN_PORT, 32, uint32_t >
{ };

struct eth_type : define_ofb_type
    < eth_type, of::oxm::basic_match_fields::ETH_TYPE, 16, uint16_t >
{ };
struct eth_src : define_ofb_type
    < eth_src, of::oxm::basic_match_fields::ETH_SRC, 48, ethaddr, ethaddr, true >
{ };
struct eth_dst : define_ofb_type
    < eth_dst, of::oxm::basic_match_fields::ETH_DST, 48, ethaddr, ethaddr, true >
{ };

struct ip_proto : define_ofb_type
    < ip_proto, of::oxm::basic_match_fields::IP_PROTO, 8, uint8_t >
{ };
// TODO: replace with ipaddr type
struct ipv4_src : define_ofb_type
    < ipv4_src, of::oxm::basic_match_fields::IPV4_SRC, 32, IPv4Addr, IPv4Addr, true >
{ };
struct ipv4_dst : define_ofb_type
    < ipv4_dst, of::oxm::basic_match_fields::IPV4_DST, 32, IPv4Addr, IPv4Addr, true >
{ };

struct tcp_src : define_ofb_type
    < tcp_src, of::oxm::basic_match_fields::TCP_SRC, 16, uint16_t >
{ };
struct tcp_dst : define_ofb_type
    < tcp_dst, of::oxm::basic_match_fields::TCP_DST, 16, uint16_t >
{ };

struct udp_src : define_ofb_type
    < udp_src, of::oxm::basic_match_fields::UDP_SRC, 16, uint16_t >
{ };
struct udp_dst : define_ofb_type
    < udp_dst, of::oxm::basic_match_fields::UDP_DST, 16, uint16_t >
{ };

// TODO: replace with ipaddr type
struct arp_spa : define_ofb_type
    < arp_spa, of::oxm::basic_match_fields::ARP_SPA, 32, IPv4Addr, IPv4Addr, true >
{ };
struct arp_tpa : define_ofb_type
    < arp_tpa, of::oxm::basic_match_fields::ARP_TPA, 32, IPv4Addr, IPv4Addr, true >
{ };
struct arp_sha : define_ofb_type
    < arp_sha, of::oxm::basic_match_fields::ARP_SHA, 48, ethaddr, ethaddr, true >
{ };
struct arp_tha : define_ofb_type
    < arp_tha, of::oxm::basic_match_fields::ARP_THA, 48, ethaddr, ethaddr, true >
{ };
struct arp_op : define_ofb_type
    < arp_op, of::oxm::basic_match_fields::ARP_OP, 16, uint16_t >
{ };
struct vlan_vid : define_ofb_type
    < vlan_vid, of::oxm::basic_match_fields::VLAN_VID, 12, uint16_t >
{ };
struct ipv6_src : define_ofb_type
    < ipv6_src, of::oxm::basic_match_fields::IPV6_SRC, 128, IPv6Addr, IPv6Addr, true >
{ };
struct ipv6_dst : define_ofb_type
    < ipv6_dst, of::oxm::basic_match_fields::IPV6_DST, 128, IPv6Addr, IPv6Addr, true >
{ };
struct icmp_type : define_ofb_type
    < icmp_type, of::oxm::basic_match_fields::ICMPV4_TYPE, 8, uint8_t >
{ };
struct icmp_code : define_ofb_type
    < icmp_code, of::oxm::basic_match_fields::ICMPV4_CODE, 8, uint8_t >
{ };
}
}
