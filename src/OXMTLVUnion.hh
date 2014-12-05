#pragma once

#include "Common.hh"

/**
 * @brief Wraps all OXMTLV types in a single-sized class.
 * This allows to allocate dynamic-typed OXMTLV on stack.
 */
struct OXMTLVUnion {

    union {
        // OpenFlow
        of13::InPort inPort;
        of13::InPhyPort inPhyPort;
        of13::Metadata metadata;
        of13::TUNNELId tunnelId;
        // L2
        of13::EthDst ethDst;
        of13::EthSrc ethSrc;
        of13::EthType ethType;
        of13::VLANVid vlanVid;
        of13::VLANPcp vlanPcp;
        of13::MPLSBOS mplsBos;
        of13::MPLSLabel mplsLabel;
        of13::MPLSTC mplsTc;
        of13::PBBIsid pbbIsid;
        // L3
        of13::IPDSCP ipDscp;
        of13::IPECN ipEcn;
        of13::IPProto ipProto;
        of13::IPv4Src ipv4Src;
        of13::IPv4Dst ipv4Dst;
        of13::IPv6Src ipv6Src;
        of13::IPv6Dst ipv6Dst;
        of13::IPV6Flabel ipv6Flabel;
        of13::IPv6Exthdr ipv6Exthdr;
        of13::IPv6NDSLL ipv6NDSLL;
        of13::IPv6NDTarget ipv6NDTarget;
        of13::IPv6NDTLL ipv6NDTLL;
        // L4
        of13::TCPSrc tcpSrc;
        of13::TCPDst tcpDst;
        of13::UDPSrc udpSrc;
        of13::UDPDst udpDst;
        of13::SCTPSrc sctpSrc;
        of13::SCTPDst sctpDst;
        of13::ICMPv4Code icmpv4Code;
        of13::ICMPv4Type icmpv4Type;
        of13::ICMPv6Code icmpv6Code;
        of13::ICMPv6Type icmpv6Type;
        of13::ARPSHA arpSha;
        of13::ARPTHA arpTha;
        of13::ARPSPA arpSpa;
        of13::ARPTPA arpTpa;
        of13::ARPOp arpOp;
    };

    OXMTLVUnion();
    OXMTLVUnion(OXMTLVUnion&& other);
    ~OXMTLVUnion();

    OXMTLVUnion(of13::OXMTLV& tlv);
    of13::OXMTLV* init(uint8_t field);
    explicit OXMTLVUnion(uint8_t field);

    bool equals(const OXMTLVUnion& other);
    bool equals(const of13::OXMTLV& other);
    bool operator==(const OXMTLVUnion& other) const;
    bool operator==(const of13::OXMTLV& other) const;
    bool operator!=(const OXMTLVUnion& other) const;
    OXMTLVUnion& operator=(const OXMTLVUnion& other);
    OXMTLVUnion& operator=(const of13::OXMTLV& tlv);

    of13::OXMTLV* base() { return m_base; }
private:
    of13::OXMTLV* m_base;
};
