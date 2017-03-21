/*
 * Copyright 2016 Applied Research Center for Computer Networks
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

/** @file */

#pragma once

#include <boost/lexical_cast.hpp>
#include "Common.hh"
#include "json11.hpp"
#include "Controller.hh"
#include "Switch.hh"

// This module contains string-oriented stuff used in RestMultipart and ReatFlowMod

//******************************PART 1: FUNCTIONS******************************
/// use instead of lexical_cast when need to interpret (u)int8_t as num, not a char (symbol code)
template<typename T>
inline std::string toString_cast(const T &value, bool toHex=false)
{
    std::stringstream ss;
    if (toHex) {
        ss << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex;
    }
    ss << value;
    return ss.str();
}

/// works on "0xffffffff" and interpret char as int instead of boost one
template<typename T>
inline T string_cast(const std::string &val)
{
    using LargestType = uint64_t;
    std::stringstream ss;
    if (val[0] == '0' && val[1] == 'x') {
        ss << std::hex;
    }
    ss << val;
    LargestType no_overflow;
    ss >> no_overflow;
    auto ans = static_cast<T>(no_overflow);
    // overflow check
    if (ans != no_overflow) {
        throw std::overflow_error{"Overflow. Check maximal supported values."};
    }
    return ans;
}


template<typename T>
inline T json_cast(const json11::Json &value)
{
    return string_cast<T>(value.string_value());
}

// toJson implementations -- per one for each of above-mentioned variable
// note: methods get non-const parameters because of json11 library issue
json11::Json toJson(std::vector<of13::FlowStats> &stats);
json11::Json toJson(std::vector<of13::PortStats> &stats);
json11::Json toJson(fluid_msg::SwitchDesc &stat);
json11::Json toJson(of13::MultipartReplyAggregate &stat);
json11::Json toJson(std::vector<of13::TableStats> &stats);
json11::Json toJson(std::vector<of13::Port> &stats);
json11::Json toJson(std::vector<of13::QueueStats> &stats);

std::map<std::string, json11::Json> processMatch(of13::Match match);
std::map<std::string, json11::Json> processInstructions(of13::InstructionSet instructionSet);
std::vector<json11::Json> processActions(ActionList actions);
json11::Json::object processSetField(of13::OXMTLV *field);

of13::OXMTLV *doSwitch(const std::string &field,
                       const std::string &value);

template<typename T>
T chooseValue(json11::Json::object filter,
              const std::string &field,
              T alternative)
{
    auto res = filter.find(field);
    if (res != filter.end()) {
        return json_cast<T>(res->second);
    } else {
        DVLOG(20) << "Using alternative for field " << field;
        return alternative;
    }
}

/// getting body of the request in Json format
json11::Json::object parse(const std::string &body);

/// checks request for fields that can not be omitted and it's values. If check fails, it throws std::string with description.
void validateRequest(const json11::Json::object &req,
                     const std::string &mode,
                     Controller *ctrl,
                     SwitchManager *sw_m);

//******************************PART 2: NAMESPACES******************************
/// By careful in use: using can override some symbols
/// note: implemented by typedefs instead of defines, because can't specify order of macro substitution
namespace fluid_fix
{
    namespace matches
    {
        typedef of13::InPort in_port;
        typedef of13::InPhyPort in_phy_port;
        typedef of13::Metadata metadata;
        typedef of13::EthDst eth_dst;
        typedef of13::EthSrc eth_src;
        typedef of13::EthType eth_type;
        typedef of13::VLANVid vlan_vid;
        typedef of13::VLANPcp vlan_pcp;
        typedef of13::IPDSCP ip_dscp;
        typedef of13::IPECN ip_ecn;
        typedef of13::IPProto ip_proto;
        typedef of13::IPv4Src ipv4_src;
        typedef of13::IPv4Dst ipv4_dst;
        typedef of13::TCPSrc tcp_src;
        typedef of13::TCPDst tcp_dst;
        typedef of13::UDPSrc udp_src;
        typedef of13::UDPDst udp_dst;
        typedef of13::SCTPSrc sctp_src;
        typedef of13::SCTPDst sctp_dst;
        typedef of13::ICMPv4Type icmpv4_type;
        typedef of13::ICMPv4Code icmpv4_code;
        typedef of13::ARPOp arp_op;
        typedef of13::ARPSPA arp_spa;
        typedef of13::ARPTPA arp_tpa;
        typedef of13::ARPSHA arp_sha;
        typedef of13::ARPTHA arp_tha;
        typedef of13::IPv6Src ipv6_src;
        typedef of13::IPv6Dst ipv6_dst;
        typedef of13::IPV6Flabel ipv6_flabel;
        typedef of13::ICMPv6Type icmpv6_type;
        typedef of13::ICMPv6Code icmpv6_code;
        typedef of13::MPLSTC mpls_tc;
        typedef of13::MPLSBOS mpls_bos;
        typedef of13::PBBIsid pbb_isid;
        typedef of13::TUNNELId tunnel_id;
        typedef of13::IPv6Exthdr ipv6_exthdr;
        typedef of13::MPLSLabel mpls_label;
    }  // end of nested namespace matches

    namespace actions
    {
        typedef of13::OutputAction OUTPUT;
        typedef of13::CopyTTLOutAction COPY_TTL_OUT;
        typedef of13::CopyTTLInAction COPY_TTL_IN;
        typedef of13::SetMPLSTTLAction SET_MPLS_TTL;
        typedef of13::DecMPLSTTLAction DEC_MPLS_TTL;
        typedef of13::PushVLANAction PUSH_VLAN;
        typedef of13::PopVLANAction POP_VLAN;
        typedef of13::PushMPLSAction PUSH_MPLS;
        typedef of13::PopMPLSAction POP_MPLS;
        typedef of13::SetQueueAction SET_QUEUE;
        typedef of13::GroupAction GROUP;
        typedef of13::SetNWTTLAction SET_NW_TTL;
        typedef of13::DecNWTTLAction DEC_NW_TTL;
        typedef of13::SetFieldAction SET_FIELD;
        typedef of13::PushPBBAction PUSH_PBB;
        typedef of13::PopPBBAction POP_PBB;
        typedef of13::ExperimenterAction EXPERIMENTER;
        // todo: not implemented:
//            COPY_FIELD
//    METER
//
//    GOTO_TABLE
//            WRITE_METADATA
//    METER
//            WRITE_ACTIONS
//    CLEAR_ACTIONS
    }  // end of namespace actions
}  // end of namespace fluid_fix

// ************************************PART 3: MACROS*********************************************
// macros for path construction
#define DPID_ "[0-9]+"
#define DPID_ALL_ "(all|[0-9]+)"
#define PORTNUMBER_ "[0-9]+"
#define PORTNUMBER_ALL_ "(all|[0-9]+)"
#define QUEUEID_ALL_ "(all|[0-9]+)"