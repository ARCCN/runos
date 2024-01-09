/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include "action_parsing.hpp"
#include <runos/core/literals.hpp>
#include <runos/core/logging.hpp>
#include <fluid/of13msg.hh>
#include <fmt/format.h>

namespace runos {

namespace of13 = fluid_msg::of13;

namespace rest {
  
std::string ip_to_string(uint32_t ip) {
    return fmt::format(FMT_STRING("{:d}.{:d}.{:d}.{:d}"),
            (ip >> 0) & 0xFF,
            (ip >> 8) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 24) & 0xFF
    );
}

void parsingSetField(of13::OXMTLV* tlv, std::string prefix, ptree& pt) {
    switch(tlv->field()) {
    case of13::OFPXMT_OFB_IN_PORT: 
        pt.put(prefix + ".set_field.in_port", ((of13::InPort*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_IN_PHY_PORT:
        pt.put(prefix + ".set_field.in_phy_port", ((of13::InPhyPort*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_METADATA:
        pt.put(prefix + ".set_field.metadata.value", ((of13::Metadata*)tlv)->value());
        pt.put(prefix + ".set_field.metadata.mask", ((of13::Metadata*)tlv)->mask());
        break;
    case of13::OFPXMT_OFB_ETH_DST:
        pt.put(prefix + ".set_field.eth_dst", ((of13::EthDst*)tlv)->value().to_string());
        break;
    case of13::OFPXMT_OFB_ETH_SRC:
        pt.put(prefix + ".set_field.eth_src", ((of13::EthSrc*)tlv)->value().to_string());
        break;
    case of13::OFPXMT_OFB_ETH_TYPE:
        pt.put(prefix + ".set_field.eth_type", ((of13::EthType*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_VLAN_VID:
        pt.put(prefix + ".set_field.vlan_vid", ((of13::VLANVid*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_VLAN_PCP:
        pt.put(prefix + ".set_field.vlan_pcp", ((of13::VLANPcp*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_IP_DSCP:
        pt.put(prefix + ".set_field.ip_dscp", ((of13::IPDSCP*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_IP_PROTO:
        pt.put(prefix + ".set_field.ip_proto", ((of13::IPProto*)tlv)->value());
        break;
    case of13::OFPXMT_OFB_IPV4_SRC:
        pt.put(prefix + ".set_field.ipv4_src", 
                                        ip_to_string(((of13::IPv4Src*)tlv)->value().getIPv4()));
        break;
    case of13::OFPXMT_OFB_IPV4_DST:
        pt.put(prefix + ".set_field.ipv4_dst", 
                                        ip_to_string(((of13::IPv4Dst*)tlv)->value().getIPv4()));
        break;
    default:
        LOG(ERROR) << "Unhandled SetField: " << (int)tlv->field();
    }
}
  
void parsingAction(fluid_msg::Action* act, std::string prefix, rest::ptree& pt,
                   rest::ptree& o_coll, rest::ptree& g_coll) {
    rest::ptree tmp;
    switch (act->type()) {
    case of13::OFPAT_OUTPUT:
        tmp.put("port", ((of13::OutputAction*)act)->port());
        tmp.put("max_len", ((of13::OutputAction*)act)->max_len());
        o_coll.push_back(std::make_pair("", tmp));
        break;
    case of13::OFPAT_GROUP:
        tmp.put("group_id", ((of13::GroupAction*)act)->group_id());
        g_coll.push_back(std::make_pair("", tmp));
        break;
    case of13::OFPAT_PUSH_VLAN:
        pt.put(prefix + ".push_vlan.ethertype", ((of13::PushVLANAction*)act)->ethertype());
        break;
    case of13::OFPAT_POP_VLAN:
        pt.put(prefix + ".pop_vlan", ""); 
        break;
    case of13::OFPAT_SET_QUEUE:
        pt.put(prefix + ".set_queue.queue_id", ((of13::SetQueueAction*)act)->queue_id());
        break;
    case of13::OFPAT_SET_FIELD:
        parsingSetField(const_cast<of13::OXMTLV*>(((of13::SetFieldAction*)act)->field()), prefix, pt);
        break;
    default:
        LOG(ERROR) << "Unhandled action type: " ;//<< act->type();
        break;
    }
}

} //namespace rest
} //namespace runos
