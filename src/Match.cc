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

#include "Match.hh"

bool oxm_match(const of13::OXMTLV *tlv, const of13::OXMTLV *value)
{
    auto field = tlv->field();
    if (field != value->field())
        return false;

    #define HANDLE_OXM_MATCH(oxm_type) \
        return oxm_match(*SMART_CAST<const oxm_type*>(tlv), SMART_CAST<const oxm_type*>(value)->value())

    // Bridge between static and dynamic type selection
    switch (field) {
    case of13::OFPXMT_OFB_IN_PORT:
        HANDLE_OXM_MATCH(of13::InPort);
    case of13::OFPXMT_OFB_IN_PHY_PORT:
        HANDLE_OXM_MATCH(of13::InPhyPort);
    case of13::OFPXMT_OFB_ETH_SRC:
        HANDLE_OXM_MATCH(of13::EthSrc);
    case of13::OFPXMT_OFB_ETH_DST:
        HANDLE_OXM_MATCH(of13::EthDst);
    case of13::OFPXMT_OFB_ETH_TYPE:
        HANDLE_OXM_MATCH(of13::EthType);
    case of13::OFPXMT_OFB_ARP_OP:
        HANDLE_OXM_MATCH(of13::ARPOp);
    case of13::OFPXMT_OFB_ARP_SHA:
        HANDLE_OXM_MATCH(of13::ARPSHA);
    case of13::OFPXMT_OFB_ARP_SPA:
        HANDLE_OXM_MATCH(of13::ARPSPA);
    case of13::OFPXMT_OFB_ARP_THA:
        HANDLE_OXM_MATCH(of13::ARPTHA);
    case of13::OFPXMT_OFB_ARP_TPA:
        HANDLE_OXM_MATCH(of13::ARPTPA);
    case of13::OFPXMT_OFB_METADATA:
        HANDLE_OXM_MATCH(of13::Metadata);
    case of13::OFPXMT_OFB_VLAN_VID:
        HANDLE_OXM_MATCH(of13::VLANVid);
    case of13::OFPXMT_OFB_VLAN_PCP:
        HANDLE_OXM_MATCH(of13::VLANPcp);
    case of13::OFPXMT_OFB_IP_DSCP:
        HANDLE_OXM_MATCH(of13::IPDSCP);
    case of13::OFPXMT_OFB_IP_ECN:
        HANDLE_OXM_MATCH(of13::IPECN);
    case of13::OFPXMT_OFB_IP_PROTO:
        HANDLE_OXM_MATCH(of13::IPProto);
    case of13::OFPXMT_OFB_IPV4_SRC:
        HANDLE_OXM_MATCH(of13::IPv4Src);
    case of13::OFPXMT_OFB_IPV4_DST:
        HANDLE_OXM_MATCH(of13::IPv4Dst);
    case of13::OFPXMT_OFB_TCP_SRC:
        HANDLE_OXM_MATCH(of13::TCPSrc);
    case of13::OFPXMT_OFB_TCP_DST:
        HANDLE_OXM_MATCH(of13::TCPDst);
    case of13::OFPXMT_OFB_UDP_SRC:
        HANDLE_OXM_MATCH(of13::UDPSrc);
    case of13::OFPXMT_OFB_UDP_DST:
        HANDLE_OXM_MATCH(of13::UDPDst);
    case of13::OFPXMT_OFB_SCTP_SRC:
        HANDLE_OXM_MATCH(of13::SCTPSrc);
    case of13::OFPXMT_OFB_SCTP_DST:
        HANDLE_OXM_MATCH(of13::SCTPDst);
    case of13::OFPXMT_OFB_ICMPV4_TYPE:
        HANDLE_OXM_MATCH(of13::ICMPv4Type);
    case of13::OFPXMT_OFB_ICMPV4_CODE:
        HANDLE_OXM_MATCH(of13::ICMPv4Code);
    case of13::OFPXMT_OFB_IPV6_SRC:
        HANDLE_OXM_MATCH(of13::IPv6Src);
    case of13::OFPXMT_OFB_IPV6_DST:
        HANDLE_OXM_MATCH(of13::IPv6Dst);
    case of13::OFPXMT_OFB_IPV6_FLABEL:
        HANDLE_OXM_MATCH(of13::IPV6Flabel);
    case of13::OFPXMT_OFB_ICMPV6_TYPE:
        HANDLE_OXM_MATCH(of13::ICMPv6Type);
    case of13::OFPXMT_OFB_ICMPV6_CODE:
        HANDLE_OXM_MATCH(of13::ICMPv6Code);
    case of13::OFPXMT_OFB_IPV6_ND_TARGET:
        HANDLE_OXM_MATCH(of13::IPv6NDTarget);
    case of13::OFPXMT_OFB_IPV6_ND_SLL:
        HANDLE_OXM_MATCH(of13::IPv6NDSLL);
    case of13::OFPXMT_OFB_IPV6_ND_TLL:
        HANDLE_OXM_MATCH(of13::IPv6NDTLL);
    case of13::OFPXMT_OFB_MPLS_LABEL:
        HANDLE_OXM_MATCH(of13::MPLSLabel);
    case of13::OFPXMT_OFB_MPLS_TC:
        HANDLE_OXM_MATCH(of13::MPLSTC);
    case of13::OFPXMT_OFB_MPLS_BOS:
        HANDLE_OXM_MATCH(of13::MPLSBOS);
    case of13::OFPXMT_OFB_PBB_ISID:
        HANDLE_OXM_MATCH(of13::PBBIsid);
    case of13::OFPXMT_OFB_TUNNEL_ID:
        HANDLE_OXM_MATCH(of13::TUNNELId);
    case of13::OFPXMT_OFB_IPV6_EXTHDR:
        HANDLE_OXM_MATCH(of13::IPv6Exthdr);
    default:
        LOG(ERROR) << "Unsupported OXM TLV field: " << field;
        return false;
    }
    #undef HANDLE_OXM_MATCH
}

EthAddress operator&(const EthAddress &a, const EthAddress &b)
{
    /* ugly code, need refactoring */
    const uint8_t *d1 = const_cast<EthAddress &>(a).get_data();
    const uint8_t *d2 = const_cast<EthAddress &>(b).get_data();
    uint8_t data[6] = {0};

    for(int i = 0; i < 6; ++i)
        data[i] = d1[i] & d2[i];

    return fluid_msg::EthAddress(data);
}

IPAddress operator&(const IPAddress &a, const IPAddress &b)
{
    // Only IPV4 version. IPv6 is broken
    return fluid_msg::IPAddress(const_cast<IPAddress &>(a).getIPv4() & const_cast<IPAddress &>(b).getIPv4());
}
