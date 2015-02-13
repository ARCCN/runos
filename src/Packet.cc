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

#include "Packet.hh"

#include <tins/ethernetII.h>
#include <tins/rawpdu.h>
#include <tins/arp.h>
#include <tins/ip.h>

struct PacketImpl {
    of13::Match match;

    Tins::EthernetII pkt;
    Tins::ARP*       arp;
    Tins::IP*        ip;

    PacketImpl(of13::PacketIn& pi)
        : //match(pi.match()),
          // Parse packet data
          pkt(Tins::RawPDU(static_cast<uint8_t*>(pi.data()), pi.data_len())
              .to<Tins::EthernetII>()),
          arp(dynamic_cast<Tins::ARP*>(pkt.inner_pdu())),
          ip(dynamic_cast<Tins::IP*>(pkt.inner_pdu()))
    {
        of13::Match::swap(match, pi.match());
    }
};

Packet::Packet(of13::PacketIn& pi)
    : m(new PacketImpl(pi))
{ }

Packet::~Packet()
{ delete m; }

std::vector<uint8_t> Packet::serialize() const
{
    return m->pkt.serialize();
}

inline static EthAddress convert(const Tins::HWAddress<6> &hw_addr)
{ return EthAddress(hw_addr.begin()); }

inline static IPAddress convert(const Tins::IPv4Address &ip_addr)
{ return IPAddress(ip_addr); }

// OpenFlow
uint32_t Packet::readInPort()
{ return m->match.in_port() ? m->match.in_port()->value() : of13::OFPP_ANY; }

uint32_t Packet::readInPhyPort()
{ return m->match.in_phy_port() ? m->match.in_phy_port()->value() : of13::OFPP_ANY; }

uint64_t Packet::readMetadata()
{ return m->match.metadata() ? m->match.metadata()->value() : 0; }

// Ethernet
EthAddress Packet::readEthSrc()
{ return convert(m->pkt.src_addr()); }

EthAddress Packet::readEthDst()
{ return convert(m->pkt.dst_addr()); }

uint16_t Packet::readEthType()
{ return m->pkt.payload_type(); }

// ARP
uint16_t Packet::readARPOp()
{ return m->arp ? m->arp->opcode() : 0; }

IPAddress Packet::readARPSPA()
{ return m->arp ? convert(m->arp->sender_ip_addr()) : IPAddress(); }

EthAddress Packet::readARPSHA()
{ return m->arp ? convert(m->arp->sender_hw_addr()) : EthAddress(); }

EthAddress Packet::readARPTHA()
{ return m->arp ? convert(m->arp->target_hw_addr()) : EthAddress(); }

IPAddress Packet::readARPTPA()
{ return m->arp ? convert(m->arp->target_ip_addr()) : IPAddress(); }

// IP
IPAddress Packet::readIPv4Src()
{ return m->ip ? convert(m->ip->src_addr()) : IPAddress(); }

IPAddress Packet::readIPv4Dst()
{ return m->ip ? convert(m->ip->dst_addr()) : IPAddress(); }

uint8_t Packet::readIPDSCP()
{ return m->ip ? (m->ip->tos() >> 2) : 0; }

uint8_t Packet::readIPECN()
{ return m->ip ? (m->ip->tos() & 0x3) : 0; }

uint8_t Packet::readIPProto()
{ return m->ip ? m->ip->protocol() : 0; }

void Packet::read(of13::OXMTLV& tlv)
{
    switch (tlv.field()) {
    case of13::OFPXMT_OFB_IN_PORT:
        tlv = of13::InPort(readInPort()); break;
    case of13::OFPXMT_OFB_IN_PHY_PORT:
        tlv = of13::InPhyPort(readInPhyPort()); break;
    case of13::OFPXMT_OFB_ETH_SRC:
        tlv = of13::EthSrc(readEthSrc()); break;
    case of13::OFPXMT_OFB_ETH_DST:
        tlv = of13::EthDst(readEthDst()); break;
    case of13::OFPXMT_OFB_ETH_TYPE:
        tlv = of13::EthType(readEthType()); break;
    case of13::OFPXMT_OFB_ARP_OP:
        tlv = of13::ARPOp(readARPOp()); break;
    case of13::OFPXMT_OFB_ARP_SHA:
        tlv = of13::ARPSHA(readARPSHA()); break;
    case of13::OFPXMT_OFB_ARP_SPA:
        tlv = of13::ARPSPA(readARPSPA()); break;
    case of13::OFPXMT_OFB_ARP_THA:
        tlv = of13::ARPTHA(readARPTHA()); break;
    case of13::OFPXMT_OFB_ARP_TPA:
        tlv = of13::ARPTPA(readARPTPA()); break;
    case of13::OFPXMT_OFB_METADATA:
        tlv = of13::Metadata(readMetadata()); break;
    case of13::OFPXMT_OFB_VLAN_VID:break;
    case of13::OFPXMT_OFB_VLAN_PCP:break;
    case of13::OFPXMT_OFB_IP_DSCP:
        tlv = of13::IPDSCP(readIPDSCP()); break;
    case of13::OFPXMT_OFB_IP_ECN:
        tlv = of13::IPECN(readIPECN()); break;
    case of13::OFPXMT_OFB_IP_PROTO:
        tlv = of13::IPProto(readIPProto()); break;
    case of13::OFPXMT_OFB_IPV4_SRC:
        tlv = of13::IPv4Src(readIPv4Src()); break;
    case of13::OFPXMT_OFB_IPV4_DST:
        tlv = of13::IPv4Dst(readIPv4Dst()); break;
    case of13::OFPXMT_OFB_TCP_SRC:break;
    case of13::OFPXMT_OFB_TCP_DST:break;
    case of13::OFPXMT_OFB_UDP_SRC:break;
    case of13::OFPXMT_OFB_UDP_DST:break;
    case of13::OFPXMT_OFB_SCTP_SRC:break;
    case of13::OFPXMT_OFB_SCTP_DST:break;
    case of13::OFPXMT_OFB_ICMPV4_TYPE:break;
    case of13::OFPXMT_OFB_ICMPV4_CODE:break;
    case of13::OFPXMT_OFB_IPV6_SRC:break;
    case of13::OFPXMT_OFB_IPV6_DST:break;
    case of13::OFPXMT_OFB_IPV6_FLABEL:break;
    case of13::OFPXMT_OFB_ICMPV6_TYPE:break;
    case of13::OFPXMT_OFB_ICMPV6_CODE:break;
    case of13::OFPXMT_OFB_IPV6_ND_TARGET:break;
    case of13::OFPXMT_OFB_IPV6_ND_SLL:break;
    case of13::OFPXMT_OFB_IPV6_ND_TLL:break;
    case of13::OFPXMT_OFB_MPLS_LABEL:break;
    case of13::OFPXMT_OFB_MPLS_TC:break;
    case of13::OFPXMT_OFB_MPLS_BOS:break;
    case of13::OFPXMT_OFB_PBB_ISID:break;
    case of13::OFPXMT_OFB_TUNNEL_ID:break;
    case of13::OFPXMT_OFB_IPV6_EXTHDR:break;
    default:
        LOG(ERROR) << "Unsupported OXM TLV field: " << tlv.field();
    }
}

void Packet::read(OXMTLVUnion &tlv)
{
    read(*tlv.base());
}

of13::OXMTLV *Packet::read(uint8_t field)
{
    of13::OXMTLV* ret = of13::Match::make_oxm_tlv(field);
    read(*ret);
    return ret;
}
