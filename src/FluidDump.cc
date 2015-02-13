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

#include "FluidDump.hh"
#include <sstream>
#include <list>

typedef
    std::list< std::pair<std::string, std::string> >
    Properties;

static std::ostringstream& dumpDict(std::ostringstream& ss, std::string class_name, Properties d)
{
    ss << class_name << "(";

    bool first = true;
    for (auto& prop : d) {
        ss << (first ? "" : ",\n") << prop.first << "\t\t=\t\t" << prop.second;
    }
    ss << ")";

    return ss;
}

static std::string dumpDict(std::string class_name, Properties d)
{
    std::ostringstream ss;
    return dumpDict(ss, class_name, d).str();
}

template<class T>
std::string decimal(T number)
{
    return std::to_string(number);
}

template<>
std::string decimal(uint8_t number)
{
    return std::to_string((int) number);
}

template<class T>
std::string hex(T number)
{
    std::ostringstream ss;
    ss << std::hex << "0x" << number;
    return ss.str();
}

std::string dump(OFMsg *msg)
{
    Properties d {
            {"version", decimal(msg->version()) },
            {"xid", hex(msg->xid()) },
            {"length", decimal(msg->length()) },
            {"type", decimal(msg->type()) }
    };

    std::ostringstream ss;
    dumpDict(ss, "OFMsg", d);

    switch (msg->type()) {
    case of13::OFPT_FLOW_MOD:
        ss << std::endl << dump(dynamic_cast<of13::FlowMod*>(msg));
        break;
    }

    return ss.str();
}

std::string dump(of13::ofp_flow_mod_command cmd)
{
    switch (cmd) {
    case of13::OFPFC_ADD: return "add";
    case of13::OFPFC_MODIFY: return "modify";
    case of13::OFPFC_MODIFY_STRICT: return "modify-strict";
    case of13::OFPFC_DELETE: return "delete";
    case of13::OFPFC_DELETE_STRICT: return "delete-strict";
    default: return "invalid";
    }
}

std::string dump(of13::ofp_flow_mod_flags flags)
{
    std::string ret;
    if (flags & of13::OFPFF_SEND_FLOW_REM)
        ret += " send-flow-rem";
    if (flags & of13::OFPFF_CHECK_OVERLAP)
        ret += " check-overlap";
    if (flags & of13::OFPFF_RESET_COUNTS)
        ret += " reset-counts";
    if (flags & of13::OFPFF_NO_PKT_COUNTS)
        ret += " no-pkt-counts";
    if (flags & of13::OFPFF_NO_BYT_COUNTS)
        ret += " no-byt-counts";
    return ret;
}

std::string dump(of13::Match)
{
    return "";
}

std::string dump(of13::InstructionSet instructions)
{
    bool first = true;
    std::string ret;


    return ret;
}

std::string dump(of13::FlowMod *fm)
{
    Properties d{
            {"cookie", hex(fm->cookie())},
            {"cookie_mask", hex(fm->cookie_mask())},
            {"table_id", decimal(fm->table_id())},
            {"command", dump((of13::ofp_flow_mod_command) fm->command())},
            {"idle_timeout", decimal(fm->idle_timeout())},
            {"hard_timeout", decimal(fm->hard_timeout())},
            {"priority", decimal(fm->priority())},
            {"buffer_id", decimal(fm->buffer_id())},
            {"out_port", decimal(fm->out_port())},
            {"out_group", decimal(fm->out_group())},
            {"flags", dump((of13::ofp_flow_mod_flags) fm->flags())},
            {"match", dump(fm->match())},
            {"instructions", dump(fm->instructions())}
    };
    return dumpDict("of13::FlowMod", d);
}

std::string dump(EthAddress addr)
{
    return addr.to_string();
}

std::string dump(of13::OXMTLV* tlv)
{
    switch ((of13::oxm_ofb_match_fields) tlv->field()) {
    case of13::OFPXMT_OFB_IN_PORT: {
        of13::InPort* in_port = dynamic_cast<of13::InPort*>(tlv);
        return "of13::InPort("  + decimal(in_port->value()) + ")";
    }
    case of13::OFPXMT_OFB_IN_PHY_PORT: {
        of13::InPhyPort* in_phy_port = dynamic_cast<of13::InPhyPort*>(tlv);
        return "of13::InPhyPort("  + decimal(in_phy_port->value()) + ")";
    }
    case of13::OFPXMT_OFB_ETH_SRC: {
        of13::EthSrc* eth_src = dynamic_cast<of13::EthSrc*>(tlv);
        if (!eth_src->has_mask())
            return "of13::EthSrc(" + dump(eth_src->value()) + ")";
        else
            return "of13::EthSrc(" + dump(eth_src->value()) + ", " + dump(eth_src->mask())  + ")";
    }
    case of13::OFPXMT_OFB_ETH_DST: {
        of13::EthDst* eth_dst = dynamic_cast<of13::EthDst*>(tlv);
        if (!eth_dst->has_mask())
            return "of13::EthDst(" + dump(eth_dst->value()) + ")";
        else
            return "of13::EthDst(" + dump(eth_dst->value()) + ", " + dump(eth_dst->mask())  + ")";
    }
    case of13::OFPXMT_OFB_ETH_TYPE: {
        of13::EthType* eth_type = dynamic_cast<of13::EthType*>(tlv);
        return "of13::EthType("  + hex(eth_type->value()) + ")";
    }
    case of13::OFPXMT_OFB_ARP_OP:
    case of13::OFPXMT_OFB_ARP_SHA:
    case of13::OFPXMT_OFB_ARP_SPA:
    case of13::OFPXMT_OFB_ARP_THA:
    case of13::OFPXMT_OFB_ARP_TPA:
    case of13::OFPXMT_OFB_METADATA:
    case of13::OFPXMT_OFB_VLAN_VID:
    case of13::OFPXMT_OFB_VLAN_PCP:
    case of13::OFPXMT_OFB_IP_DSCP:
    case of13::OFPXMT_OFB_IP_ECN:
    case of13::OFPXMT_OFB_IP_PROTO:
    case of13::OFPXMT_OFB_IPV4_SRC:
    case of13::OFPXMT_OFB_IPV4_DST:
    case of13::OFPXMT_OFB_TCP_SRC:
    case of13::OFPXMT_OFB_TCP_DST:
    case of13::OFPXMT_OFB_UDP_SRC:
    case of13::OFPXMT_OFB_UDP_DST:
    case of13::OFPXMT_OFB_SCTP_SRC:
    case of13::OFPXMT_OFB_SCTP_DST:
    case of13::OFPXMT_OFB_ICMPV4_TYPE:
    case of13::OFPXMT_OFB_ICMPV4_CODE:
    case of13::OFPXMT_OFB_IPV6_SRC:
    case of13::OFPXMT_OFB_IPV6_DST:
    case of13::OFPXMT_OFB_IPV6_FLABEL:
    case of13::OFPXMT_OFB_ICMPV6_TYPE:
    case of13::OFPXMT_OFB_ICMPV6_CODE:
    case of13::OFPXMT_OFB_IPV6_ND_TARGET:
    case of13::OFPXMT_OFB_IPV6_ND_SLL:
    case of13::OFPXMT_OFB_IPV6_ND_TLL:
    case of13::OFPXMT_OFB_MPLS_LABEL:
    case of13::OFPXMT_OFB_MPLS_TC:
    case of13::OFPXMT_OFB_MPLS_BOS:
    case of13::OFPXMT_OFB_PBB_ISID:
    case of13::OFPXMT_OFB_TUNNEL_ID:
    case of13::OFPXMT_OFB_IPV6_EXTHDR:
    default:
        return "of13::OXMTLV()";
    }
}
