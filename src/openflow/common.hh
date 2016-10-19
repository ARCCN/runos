#pragma once

#include <cstdint>
#include <boost/endian/arithmetic.hpp>

namespace runos {
namespace of {

using namespace boost::endian;

struct header {
    big_uint8_t  version;
    big_uint8_t  type;
    big_uint16_t length;
    big_uint32_t xid;
};
static_assert(sizeof(header) == 8, "");

constexpr unsigned TCP_PORT = 6653;
constexpr unsigned SSL_PORT = 6653;
constexpr unsigned ETH_ALEN = 6;

namespace oxm {

/* ## -------------------------- ## */
/* ## OpenFlow Extensible Match. ## */
/* ## -------------------------- ## */

/* OXM Class IDs.
 * The high order bit differentiate reserved classes from member classes.
 * Classes 0x0000 to 0x7FFF are member classes, allocated by ONF.
 * Classes 0x8000 to 0xFFFE are reserved classes, reserved for standardisation.
 */
enum class ns : uint16_t {
    NXM_0          = 0x0000,    /* Backward compatibility with NXM */
    NXM_1          = 0x0001,    /* Backward compatibility with NXM */
    OPENFLOW_BASIC = 0x8000,    /* Basic class for OpenFlow */
    EXPERIMENTER   = 0xFFFF,    /* Experimenter class */
};

/* OXM Flow match field types for OpenFlow basic class. */
enum class basic_match_fields : uint8_t {
    IN_PORT        = 0,  /* Switch input port. */
    IN_PHY_PORT    = 1,  /* Switch physical input port. */
    METADATA       = 2,  /* Metadata passed between tables. */
    ETH_DST        = 3,  /* Ethernet destination address. */
    ETH_SRC        = 4,  /* Ethernet source address. */
    ETH_TYPE       = 5,  /* Ethernet frame type. */
    VLAN_VID       = 6,  /* VLAN id. */
    VLAN_PCP       = 7,  /* VLAN priority. */
    IP_DSCP        = 8,  /* IP DSCP (6 bits in ToS field). */
    IP_ECN         = 9,  /* IP ECN (2 bits in ToS field). */
    IP_PROTO       = 10, /* IP protocol. */
    IPV4_SRC       = 11, /* IPv4 source address. */
    IPV4_DST       = 12, /* IPv4 destination address. */
    TCP_SRC        = 13, /* TCP source port. */
    TCP_DST        = 14, /* TCP destination port. */
    UDP_SRC        = 15, /* UDP source port. */
    UDP_DST        = 16, /* UDP destination port. */
    SCTP_SRC       = 17, /* SCTP source port. */
    SCTP_DST       = 18, /* SCTP destination port. */
    ICMPV4_TYPE    = 19, /* ICMP type. */
    ICMPV4_CODE    = 20, /* ICMP code. */
    ARP_OP         = 21, /* ARP opcode. */
    ARP_SPA        = 22, /* ARP source IPv4 address. */
    ARP_TPA        = 23, /* ARP target IPv4 address. */
    ARP_SHA        = 24, /* ARP source hardware address. */
    ARP_THA        = 25, /* ARP target hardware address. */
    IPV6_SRC       = 26, /* IPv6 source address. */
    IPV6_DST       = 27, /* IPv6 destination address. */
    IPV6_FLABEL    = 28, /* IPv6 Flow Label */
    ICMPV6_TYPE    = 29, /* ICMPv6 type. */
    ICMPV6_CODE    = 30, /* ICMPv6 code. */
    IPV6_ND_TARGET = 31, /* Target address for ND. */
    IPV6_ND_SLL    = 32, /* Source link-layer for ND. */
    IPV6_ND_TLL    = 33, /* Target link-layer for ND. */
    MPLS_LABEL     = 34, /* MPLS label. */
    MPLS_TC        = 35, /* MPLS TC. */
    MPLS_BOS       = 36, /* MPLS BoS bit. */
    PBB_ISID       = 37, /* PBB I-SID. */
    TUNNEL_ID      = 38, /* Logical Port Metadata. */
    IPV6_EXTHDR    = 39, /* IPv6 Extension Header pseudo-field */
};

struct header {
    big_uint16_t ns;
    uint8_t      field:7;
    bool         hasmask:1;
    big_uint8_t  length;
};
static_assert(sizeof(header) == 4, "");

/* Bit definitions for IPv6 Extension Header pseudo-field. */
enum class ipv6exthdr_flags {      
    NONEXT = 1 << 0,     /* "No next header" encountered. */
    ESP    = 1 << 1,     /* Encrypted Sec Payload header present. */
    AUTH   = 1 << 2,     /* Authentication header present. */
    DEST   = 1 << 3,     /* 1 or 2 dest headers present. */
    FRAG   = 1 << 4,     /* Fragment header present. */
    ROUTER = 1 << 5,     /* Router header present. */
    HOP    = 1 << 6,     /* Hop-by-hop header present. */
    UNREP  = 1 << 7,     /* Unexpected repeats encountered. */
    UNSEQ  = 1 << 8,     /* Unexpected sequencing encountered. */
};

/* Header for OXM experimenter match fields.
 * The experimenter class should not use OXM_HEADER() macros for defining
 * fields due to this extra header. */
struct experimenter_header {
    header       oxm_header;        /* oxm_class = OFPXMC_EXPERIMENTER */
    big_uint32_t experimenter;      /* Experimenter ID which takes the same
                                   form as in struct ofp_experimenter_header. */
};
static_assert(sizeof(experimenter_header) == 8, "");

}

}
}
