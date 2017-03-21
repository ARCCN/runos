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

#include <sstream>

#include "RestStringProcessing.hh"
#include "Controller.hh"
#include "Switch.hh"

#define SHOW(field, obj) {#field, toString_cast(obj.field(), false)}

#define EMPLACE_IF_EXIST(field, match, isHex) \
    if (match.field()) { \
        ans.emplace(std::pair<std::string, json11::Json>(#field, toString_cast(match.field()->value(), isHex))); \
    }


template<>
std::string toString_cast<std::string>(const std::string &value, bool toHex)
{
    return value;
}

template<>
std::string toString_cast<char>(const char &value, bool toHex)
{
    std::stringstream ss;
    ss << static_cast<int>(value);
    return ss.str();
}

template<>
std::string toString_cast<unsigned char>(const unsigned char &value, bool toHex)
{
    std::stringstream ss;
    ss << static_cast<unsigned int>(value);
    return ss.str();
}

json11::Json toJson(std::vector<of13::FlowStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(length, stat),
                SHOW(table_id, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat),
                SHOW(priority, stat),
                SHOW(idle_timeout, stat),
                SHOW(hard_timeout, stat),
                {"flags", toString_cast(stat.get_flags(), false)},
                SHOW(cookie, stat),
                SHOW(packet_count, stat),
                SHOW(byte_count, stat),
                {"match:", json11::Json::object {processMatch(stat.match())}},
                {"actions:", json11::Json::object {processInstructions(stat.instructions())}}
        });
    }
    return ans;
}

std::map<std::string, json11::Json> processMatch(of13::Match match)
{
    std::map<std::string, json11::Json> ans;
    EMPLACE_IF_EXIST(in_port, match, false)
    EMPLACE_IF_EXIST(in_phy_port, match, false)
    EMPLACE_IF_EXIST(eth_type, match, true)
    EMPLACE_IF_EXIST(ip_proto, match, true)
    EMPLACE_IF_EXIST(tcp_dst, match, false)
    EMPLACE_IF_EXIST(tcp_src, match, false)
    EMPLACE_IF_EXIST(udp_dst, match, false)
    EMPLACE_IF_EXIST(udp_src, match, false)
    EMPLACE_IF_EXIST(metadata, match, false)
    if (match.ipv4_src()) {
        ans.emplace(std::pair<std::string, json11::Json>("ipv4_src", AppObject::uint32_t_ip_to_string(match.ipv4_src()->value().getIPv4())));
    }
    if (match.ipv4_dst()) {
        ans.emplace(std::pair<std::string, json11::Json>("ipv4_dst", AppObject::uint32_t_ip_to_string(match.ipv4_dst()->value().getIPv4())));
    }
    if (match.eth_src()) {
        ans.emplace(std::pair<std::string, json11::Json>("eth_src", match.eth_src()->value().to_string()));
    }
    if (match.eth_dst()) {
        ans.emplace(std::pair<std::string, json11::Json>("eth_dst", match.eth_dst()->value().to_string()));
    }

    // arp
    EMPLACE_IF_EXIST(arp_op, match, true)
    if (match.arp_spa()) {
        ans.emplace(std::pair<std::string, json11::Json>("arp_spa", AppObject::uint32_t_ip_to_string(match.arp_spa()->value().getIPv4())));
    }
    if (match.arp_tpa()) {
        ans.emplace(std::pair<std::string, json11::Json>("arp_tpa", AppObject::uint32_t_ip_to_string(match.arp_tpa()->value().getIPv4())));
    }
    if (match.arp_sha()) {
        ans.emplace(std::pair<std::string, json11::Json>("arp_sha", match.arp_sha()->value().to_string()));
    }
    if (match.arp_tha()) {
        ans.emplace(std::pair<std::string, json11::Json>("arp_tha", match.arp_tha()->value().to_string()));
    }

    // vlan
    EMPLACE_IF_EXIST(vlan_vid, match, false)
    EMPLACE_IF_EXIST(vlan_pcp, match, false)

    return ans;
}

std::map<std::string, json11::Json> processInstructions(of13::InstructionSet instructionSet)
{
    std::map<std::string, json11::Json> ans;
    auto instructions = instructionSet.instruction_set();
    for (of13::Instruction* inst : instructions) {
        switch(inst->type()){
        case of13::OFPIT_GOTO_TABLE:
            ans["goto_table"] = toString_cast(((of13::GoToTable*)inst)->table_id());
            break;
        case of13::OFPIT_WRITE_METADATA:
            ans["metadata"] = json11::Json::object({
                {"metadata",
                   toString_cast(((of13::WriteMetadata*)inst)->metadata()) },
                {"metadata_mask",
                   toString_cast(((of13::WriteMetadata*)inst)->metadata_mask()) }
            });
            break;
        case of13::OFPIT_WRITE_ACTIONS:
            ans["write_actions"] = processActions(((of13::ApplyActions *) inst)->actions());
            break;
        case of13::OFPIT_APPLY_ACTIONS:
            ans["apply_actions"] = processActions(((of13::ApplyActions *) inst)->actions());
            break;
        case of13::OFPIT_CLEAR_ACTIONS:
            ans["clear_actions"] = "clear_actions";
            break;
        case of13::OFPIT_METER:
            ans["meter"] = toString_cast(((of13::Meter *)inst)->meter_id());
            break;
        case of13::OFPAT_EXPERIMENTER:
            ans["experimenter"] = "experimenter";
            break;
        default :
            LOG(ERROR) << "Unhandled instuction type: " << inst->type();
            break;
        }
    }
    return ans;
}

std::vector<json11::Json> processActions(ActionList actions)
{
    std::vector<json11::Json> ans;

    for (auto *action : actions.action_list()) {
        std::map<std::string, json11::Json> elem;
        switch (action->type()) {
        case of13::OFPAT_OUTPUT:
        {
            auto act = static_cast<of13::OutputAction *>(action);
            elem["type"] = "OUTPUT";
            elem["port"] = toString_cast(act->port(), true);
            break;
        }
        case of13::OFPAT_COPY_TTL_OUT:
        {
            elem["type"] = "COPY_TTL_OUT";
            break;
        }
        case of13::OFPAT_COPY_TTL_IN:
        {
            elem["type"] = "COPY_TTL_IN";
            break;
        }
        case of13::OFPAT_SET_MPLS_TTL:
        {
            auto *act = static_cast<of13::SetMPLSTTLAction *>(action);
            elem["type"] = "SET_MPLS_TTL";
            elem["mpls_ttl"] = toString_cast(act->mpls_ttl());
            break;
        }
        case of13::OFPAT_DEC_MPLS_TTL:
        {
            elem["type"] = "DEC_MPLS_TTL";
            break;
        }
        case of13::OFPAT_PUSH_VLAN:
        {
            auto *act = static_cast<of13::PushVLANAction *>(action);
            elem["type"] = "PUSH_VLAN";
            elem["ethertype"] = toString_cast(act->ethertype());
            break;
        }
        case of13::OFPAT_POP_VLAN:
        {
            elem["type"] = "POP_VLAN";
            break;
        }
        case of13::OFPAT_PUSH_MPLS:
        {
            auto *act = static_cast<of13::PushMPLSAction *>(action);
            elem["type"] = "PUSH_MPLS";
            elem["ethertype"] = toString_cast(act->ethertype());
            break;
        }
        case of13::OFPAT_POP_MPLS:
        {
            auto *act = static_cast<of13::PopMPLSAction *>(action);
            elem["type"] = "POP_MPLS";
            elem["ethertype"] = toString_cast(act->ethertype());
            break;
        }
        case of13::OFPAT_SET_QUEUE:
        {
            auto *act = static_cast<of13::SetQueueAction *>(action);
            elem["type"] = "SET_QUEUE";
            elem["queue_id"] = toString_cast(act->queue_id());
            break;
        }
        case of13::OFPAT_GROUP:
        {
            auto *act = static_cast<of13::GroupAction *>(action);
            elem["type"] = "GROUP";
            elem["group_id"] = toString_cast(act->group_id());
            break;
        }
        case of13::OFPAT_SET_NW_TTL:
        {
            auto *act = static_cast<of13::SetNWTTLAction *>(action);
            elem["type"] = "SET_NW_TTL";
            elem["nw_ttl"] = toString_cast(act->nw_ttl());
            break;
        }
        case of13::OFPAT_DEC_NW_TTL:
        {
            elem["type"] = "DEC_NW_TTL";
            break;
        }
        case of13::OFPAT_SET_FIELD:
        {
            auto *act = static_cast<of13::SetFieldAction *>(action);
            elem = processSetField(act->field());
            elem["type"] = "SET_FIELD";
            break;
        }
        case of13::OFPAT_PUSH_PBB:
        {
            auto *act = static_cast<of13::PushPBBAction *>(action);
            elem["type"] = "PUSH_PBB";
            elem["ethertype"] = toString_cast(act->ethertype());
            break;
        }
        case of13::OFPAT_POP_PBB:
        {
            elem["type"] = "POP_PBB";
            break;
        }
        case of13::OFPAT_EXPERIMENTER:
        {
            elem["type"] = "EXPERIMENTER";
            break;
        }
        default:
            elem["type"] = "Unknown action type";
            break;
        }
        ans.emplace_back(elem);
    }
    return ans;
}

json11::Json::object processSetField(of13::OXMTLV *field)
{
    switch (field->field()){
    case of13::OFPXMT_OFB_IN_PORT:
        return json11::Json::object {
            { "field", "in_port"},
            { "value", toString_cast(static_cast<of13::InPort*>(field)->value()) }
        };
    case of13::OFPXMT_OFB_ETH_SRC:
        return json11::Json::object {
           { "field", "eth_src"},
           { "value", static_cast<of13::EthSrc*>(field)->value().to_string() }
        };
    case of13::OFPXMT_OFB_ETH_DST:
        return json11::Json::object {
            { "field", "eth_dst"},
            { "value", static_cast<of13::EthDst*>(field)->value().to_string() }
        };
    case of13::OFPXMT_OFB_ETH_TYPE:
        return json11::Json::object{
            { "field", "eth_type"},
            { "value", toString_cast(static_cast<of13::EthType*>(field)->value()) }
        };
    case of13::OFPXMT_OFB_VLAN_VID:
        return json11::Json::object{
            { "field", "vlan_vid"},
            { "value", toString_cast(static_cast<of13::VLANVid*>(field)->value()) }
        };
    case of13::OFPXMT_OFB_IPV4_SRC:
        return json11::Json::object {
            { "field", "ip_src"},
            { "value", AppObject::uint32_t_ip_to_string(static_cast<of13::IPv4Src*>(field)->value().getIPv4()) }
        };
    case of13::OFPXMT_OFB_IPV4_DST:
        return json11::Json::object {
            { "field", "ip_dst"},
            { "value", AppObject::uint32_t_ip_to_string(static_cast<of13::IPv4Dst*>(field)->value().getIPv4()) }
        };
    case of13::OFPXMT_OFB_IP_PROTO:
        return json11::Json::object {
            { "field", "ip_proto"},
            { "value", toString_cast(static_cast<of13::IPProto*>(field)->value()) }
        };
    case of13::OFPXMT_OFB_ARP_SPA:
        return json11::Json::object {
            { "field", "arp_spa"},
            { "value", AppObject::uint32_t_ip_to_string(static_cast<of13::ARPSPA*>(field)->value().getIPv4()) }
        };
    case of13::OFPXMT_OFB_ARP_TPA:
        return json11::Json::object {
            { "field", "arp_tpa"},
            { "value", AppObject::uint32_t_ip_to_string(static_cast<of13::ARPTPA*>(field)->value().getIPv4()) }
        };
    case of13::OFPXMT_OFB_ARP_SHA:
        return json11::Json::object {
            { "field", "arp_sha"},
            { "value", static_cast<of13::ARPSHA*>(field)->value().to_string() }
        };
    case of13::OFPXMT_OFB_ARP_THA:
        return json11::Json::object {
            { "field", "arp_tha"},
            { "value", static_cast<of13::ARPTHA*>(field)->value().to_string() }
        };
    case of13::OFPXMT_OFB_ARP_OP:
        return json11::Json::object {
            { "field", "arp_op"},
            { "value", toString_cast(static_cast<of13::ARPOp*>(field)->value()) }
        };
    default:
        LOG(INFO) << " TODO : Unhandled SetField: " << (int)field->field();
    }
    return json11::Json::object{
        { "unhandled field", "error" }
    };
}

json11::Json toJson(std::vector<of13::PortStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                SHOW(rx_packets, stat),
                SHOW(tx_packets, stat),
                SHOW(rx_bytes, stat),
                SHOW(tx_bytes, stat),
                SHOW(rx_dropped, stat),
                SHOW(tx_dropped, stat),
                SHOW(rx_errors, stat),
                SHOW(tx_errors, stat),
                SHOW(rx_frame_err, stat),
                SHOW(rx_over_err, stat),
                SHOW(rx_crc_err, stat),
                SHOW(collisions, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat)
        });
    }
    return ans;
}

json11::Json toJson(fluid_msg::SwitchDesc &stat)
{
    return json11::Json::object {
            SHOW(mfr_desc, stat),
            SHOW(hw_desc, stat),
            SHOW(sw_desc, stat),
            SHOW(serial_num, stat),
            SHOW(dp_desc, stat)
    };
}

json11::Json toJson(of13::MultipartReplyAggregate &stat)
{
    return json11::Json::object {
            SHOW(packet_count, stat),
            SHOW(byte_count, stat),
            SHOW(flow_count, stat),
    };
}

json11::Json toJson(std::vector<of13::TableStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(active_count, stat),
                SHOW(table_id, stat),
                SHOW(lookup_count, stat),
                SHOW(matched_count, stat)
        });
    }
    return ans;
}

json11::Json toJson(std::vector<of13::Port> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                {"hw_addr", stat.hw_addr().to_string()},
                SHOW(name, stat),
                SHOW(config, stat),
                SHOW(state, stat),
                SHOW(curr, stat),
                SHOW(advertised, stat),
                SHOW(supported, stat),
                SHOW(peer, stat),
                SHOW(curr_speed, stat),
                SHOW(max_speed, stat)
        });
    }
    return ans;
}

json11::Json toJson(std::vector<of13::QueueStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                SHOW(queue_id, stat),
                SHOW(tx_bytes, stat),
                SHOW(tx_packets, stat),
                SHOW(tx_errors, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat),
        });
    }
    return ans;
}

json11::Json::object parse(const std::string &body)
{
    std::string forErr;
    json11::Json::object req = json11::Json::parse(body, forErr).object_items();
    if (!forErr.empty()) {
        std::ostringstream errMsg;
        errMsg << "Can't parse input request : " << forErr;
        throw errMsg.str();
    }
    return req;
}

void validateRequest(const json11::Json::object &req,
                     const std::string &mode,
                     Controller *ctrl,
                     SwitchManager *sw_m)
{
    try {
        std::stringstream errMsg;
        errMsg << "Incorrect request.";
        auto dpid = json_cast<uint64_t>(req.at("dpid"));
        if (mode == "full") {
            auto table_id = json_cast<uint8_t>(req.at("table_id"));
            if (table_id >= ctrl->handler_table()) {
                errMsg << " RestFlowMod can work only with tables in range of 0.." << ctrl->handler_table() <<
                       ". Look at RestFlowMod class definition for more details.";
                throw errMsg.str();
            }
            req.at("match");
            req.at("actions");
        }
        if (!sw_m->getSwitch(dpid)) {
            errMsg << " Switch hasn't been found";
            throw errMsg.str();
        }
    } catch (const std::string &) {
        throw;
    } catch (const std::overflow_error &errMsg) {
        throw errMsg.what();
    } catch (...) {
        throw std::string{"Incorrect request."};
    }
}