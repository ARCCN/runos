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

#include "OXMTLVUnion.hh"

OXMTLVUnion::OXMTLVUnion()
    : m_base(nullptr)
{
}

OXMTLVUnion::~OXMTLVUnion()
{
    if (m_base) m_base->~OXMTLV();
}

of13::OXMTLV *OXMTLVUnion::init(uint8_t field)
{
    if (m_base != nullptr) {
        m_base->~OXMTLV();
        m_base = nullptr;
    }

    switch (field) {
    case of13::OFPXMT_OFB_IN_PORT:
        m_base = new (&inPort) of13::InPort(); break;
    case of13::OFPXMT_OFB_IN_PHY_PORT:
        m_base = new (&inPhyPort) of13::InPhyPort(); break;
    case of13::OFPXMT_OFB_ETH_SRC:
        m_base = new (&ethSrc) of13::EthSrc(); break;
    case of13::OFPXMT_OFB_ETH_DST:
        m_base = new (&ethDst) of13::EthDst(); break;
    case of13::OFPXMT_OFB_ETH_TYPE:
        m_base = new (&ethType) of13::EthType(); break;
    case of13::OFPXMT_OFB_ARP_OP:
        m_base = new (&arpOp) of13::ARPOp(); break;
    case of13::OFPXMT_OFB_ARP_SHA:
        m_base = new (&arpSha) of13::ARPSHA(); break;
    case of13::OFPXMT_OFB_ARP_SPA:
        m_base = new (&arpSpa) of13::ARPSPA(); break;
    case of13::OFPXMT_OFB_ARP_THA:
        m_base = new (&arpTha) of13::ARPTHA(); break;
    case of13::OFPXMT_OFB_ARP_TPA:
        m_base = new (&arpTpa) of13::ARPTPA(); break;
    case of13::OFPXMT_OFB_METADATA:break;
        m_base = new (&metadata) of13::Metadata(); break;
    //case of13::OFPXMT_OFB_VLAN_VID:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_VLAN_PCP:break;
    //    m_base = new (&) of13::(); break;
    case of13::OFPXMT_OFB_IP_DSCP:
        m_base = new (&ipDscp) of13::IPDSCP(); break;
    case of13::OFPXMT_OFB_IP_ECN:
        m_base = new (&ipEcn) of13::IPECN(); break;
    case of13::OFPXMT_OFB_IP_PROTO:
        m_base = new (&ipProto) of13::IPProto(); break;
    case of13::OFPXMT_OFB_IPV4_SRC:
        m_base = new (&ipv4Src) of13::IPv4Src(); break;
    case of13::OFPXMT_OFB_IPV4_DST:
        m_base = new (&ipv4Dst) of13::IPv4Dst(); break;
    case of13::OFPXMT_OFB_TCP_SRC:break;
        m_base = new (&tcpSrc) of13::TCPSrc(); break;
    case of13::OFPXMT_OFB_TCP_DST:break;
        m_base = new (&tcpDst) of13::TCPDst(); break;
    case of13::OFPXMT_OFB_UDP_SRC:break;
        m_base = new (&udpSrc) of13::UDPSrc(); break;
    case of13::OFPXMT_OFB_UDP_DST:break;
        m_base = new (&udpDst) of13::UDPDst(); break;
    //case of13::OFPXMT_OFB_SCTP_SRC:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_SCTP_DST:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_ICMPV4_TYPE:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_ICMPV4_CODE:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_SRC:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_DST:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_FLABEL:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_ICMPV6_TYPE:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_ICMPV6_CODE:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_ND_TARGET:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_ND_SLL:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_ND_TLL:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_MPLS_LABEL:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_MPLS_TC:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_MPLS_BOS:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_PBB_ISID:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_TUNNEL_ID:break;
    //    m_base = new (&) of13::(); break;
    //case of13::OFPXMT_OFB_IPV6_EXTHDR:break;
    //    m_base = new (&) of13::(); break;
    default:
        LOG(ERROR) << "Unsupported OXM TLV field: " << field;
    }

    return m_base;
}

OXMTLVUnion::OXMTLVUnion(uint8_t field)
    : m_base(nullptr)
{
    init(field);
}
