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

#include "RestFlowMod.hh"
#include "Controller.hh"
#include "FlowManager.hh"
#include "RestListener.hh"
#include "SwitchConnection.hh"
#include "RestStringProcessing.hh"

#include <boost/lexical_cast.hpp>

REGISTER_APPLICATION(RestFlowMod, {"controller", "switch-manager", "rest-listener", ""})

template<typename T>
class FlaggedVal
{
    T val_;
    bool is_filled_;
public:
    FlaggedVal() : val_(T{}), is_filled_(false) {}
    // getters
    T val() const
    {
        return val_;
    }
    bool isFilled() const {
        return is_filled_;
    }
    // setters
    void val(const T &val)
    {
        val_ = val;
        is_filled_ = true;
    }
};

/// converts json value to uint and adds it to the FlowMod object
#define HANDLE_UINT(field, type) \
    try { \
        auto value = json_cast<type>(matches.at(#field)); \
        fm.add_oxm_field(new field{value}); \
        added = true; \
    } catch (std::overflow_error) { \
        throw; \
    } catch (...) {}

/// use for types that have constructor from std::string. Calls the constructor and adds the result to the FlowMod object
/// note: EthAddress: sets "00:00:...:00" in case of incorrect input instead of throwing an exception
#define HANDLE_STRING(field) \
    try { \
        fm.add_oxm_field(new field{matches.at(#field).string_value()}); \
        added = true; \
    } catch (...) {}

#define SET(field, dst, src) dst.field(json_cast<decltype(dst.field())>(src.at(#field)))

#define SET_ACT(field) \
    try { \
        SET(field, act, action); \
    } catch (...) {}


/// ip-encapsulated protocols
namespace inet_types
{
    constexpr uint16_t tcp = 6;
    constexpr uint16_t udp = 17;
}  // end of namespace inet_types

/// Ethernet-encapsulated protocols
namespace eth_types
{
    constexpr uint16_t ipv4 = 0x0800;
    constexpr uint16_t ipv6 = 0x86DD;
    constexpr uint16_t arp = 0x0806;
    constexpr uint16_t vlan = 0x8100;
}  // end of namespace eth_types


void RestFlowMod::init(Loader* loader, const Config& rootConfig)
{
    ctrl_ = Controller::get(loader);
    sw_m_ = SwitchManager::get(loader);
    tableNo_ = ctrl_->reserveTable();

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::POST, "flowentry");
    acceptPath(Method::POST, "flowentry/delete");
    acceptPath(Method::POST, "flowentry/delete_strict");
    acceptPath(Method::DELETE, "flowentry/clear/" DPID_);
}

void RestFlowMod::processInfoAdd(of13::FlowMod &fm,
                                 const json11::Json::object &req)
{
    auto chVal = [&] (std::string field, uint64_t alternative) {
        return chooseValue<uint64_t>(req, field, uint64_t(alternative));
    };

    fm.table_id(json_cast<uint8_t>(req.at("table_id")));
    fm.cookie(chVal("cookie", 0));
    fm.cookie_mask(chVal("cookie_mask", 0));
    fm.idle_timeout(chVal("idle_timeout", 10));
    fm.hard_timeout(chVal("hard_timeout", 10));
    fm.priority(chVal("priority", 0));
    fm.buffer_id(chVal("buffer_id", 0xffffffff));
    fm.flags(chVal("flags", 0));
}

void RestFlowMod::processInfoDelete(of13::FlowMod &fm,
                                 const json11::Json::object &req)
{
    auto chVal = [&] (std::string field, uint64_t alternative) {
        return chooseValue<uint64_t>(req, field, uint64_t(alternative));
    };

    fm.cookie(chVal("cookie", 0));
    fm.cookie_mask(chVal("cookie_mask", 0));
    fm.table_id(chVal("table_id", 0));
    fm.idle_timeout(chVal("idle_timeout", 0));
    fm.hard_timeout(chVal("hard_timeout", 0));
    fm.priority(chVal("priority", 0));
    fm.buffer_id(chVal("buffer_id", OFP_NO_BUFFER));
    fm.out_port(chVal("out_port", of13::OFPP_ANY));
    fm.out_group(chVal("out_group", of13::OFPG_ANY));
    fm.flags(chVal("flags", 0));
}

void RestFlowMod::processMatches(of13::FlowMod &fm, const json11::Json::object &matches)
{
    using namespace fluid_fix::matches;
    bool added = false;
    FlaggedVal<uint16_t> ether_type;
    FlaggedVal<uint8_t> inet_type;

    // process L4
    HANDLE_UINT(tcp_src, uint16_t)
    HANDLE_UINT(tcp_dst, uint16_t)
    if (added) {
        fm.add_oxm_field(new of13::EthType{eth_types::ipv4});
        fm.add_oxm_field(new of13::IPProto{inet_types::tcp});
        ether_type.val(eth_types::ipv4);
        inet_type.val(inet_types::tcp);
        added = false;
    }
    HANDLE_UINT(udp_src, uint16_t)
    HANDLE_UINT(udp_dst, uint16_t)
    if (added) {
        fm.add_oxm_field(new of13::EthType{eth_types::ipv4});
        fm.add_oxm_field(new of13::IPProto{inet_types::udp});
        ether_type.val(eth_types::ipv4);
        inet_type.val(inet_types::udp);
        added = false;
    }

    // process L3
    HANDLE_UINT(ip_proto, uint8_t)
    if (!ether_type.isFilled() || ether_type.val() == eth_types::ipv4) {
        HANDLE_STRING(ipv4_src)
        HANDLE_STRING(ipv4_dst)
        if (added) {
            fm.add_oxm_field(new of13::EthType{eth_types::ipv4});
            ether_type.val(eth_types::ipv4);
            added = false;
        }
    }
    if (!ether_type.isFilled()) {
        HANDLE_UINT(arp_op, uint16_t)
        HANDLE_STRING(arp_spa)
        HANDLE_STRING(arp_tpa)
        HANDLE_STRING(arp_sha)
        HANDLE_STRING(arp_tha)
        if (added) {
            fm.add_oxm_field(new of13::EthType{eth_types::arp});
            ether_type.val(eth_types::arp);
            added = false;
        }
    }

    // process L2, L1
    HANDLE_UINT(vlan_vid, uint16_t)
    HANDLE_UINT(vlan_pcp, uint8_t)
    HANDLE_UINT(in_port, uint32_t)
    HANDLE_UINT(in_phy_port, uint32_t)
    HANDLE_UINT(metadata, uint64_t)
    HANDLE_STRING(eth_dst)
    HANDLE_STRING(eth_src)
    HANDLE_UINT(eth_type, uint16_t)
}

/**
 * note: by default, unexpected fields are ignored. In some cases you will be informed about it, in other -- not.
 * In case of incorrect request or if an expected field can not be found, the processing will be stoped.
 */
void RestFlowMod::processActions(of13::FlowMod &fm, const json11::Json::array &actions)
{
    of13::ApplyActions applyActions;
    for (const auto &action : actions) {
        processAction(action.object_items(), applyActions);
    }
    fm.add_instruction(applyActions);
}

/**
 * Method tries to convert received values to corresponding types.
 * In general, if a field can't be converted it will be discarded.
 * But in some cases (like if an overflow occurred) the processing will be stoped and you will recieve an error massage.
 */
json11::Json RestFlowMod::handlePOST(std::vector<std::string> params, std::string body)
{
    if (params[0] != "flowentry") {
        return json11::Json::object{
                {"RestFlowMod", "Unsupported parameter"}
        };
    }
    try {
        auto req = parse(body);
        of13::FlowMod fm;
        if (params.size() == 1) {  // add new flow entry
            validateRequest(req, "full", ctrl_, sw_m_);

            const auto &matches = req.at("match").object_items();
            const auto &actions = req.at("actions").array_items();

            fm.command(of13::OFPFC_ADD);
            processInfoAdd(fm, req);
            processMatches(fm, matches);
            processActions(fm, actions);

            auto dpid = json_cast<uint64_t>(req.at("dpid"));
            auto sw = sw_m_->getSwitch(dpid);
            if (!sw) {
                throw "switch not found";
            }
            sw->connection()->send(fm);

            auto msg = "new flow added" + response_.str();
            response_.str("");
            return json11::Json::object{
                    {"RestFlowMod", msg.c_str()}
            };
        } else if (params[1] == "delete_strict" || params[1] == "delete") {
            // delete existing flow entries
            // Note: In contrast to "flowentry/clear", you should control redirection flows (redirects to next tables and to_controller in Maple's table) on your own.

            validateRequest(req, "lite", ctrl_, sw_m_);
            if (params[1] == "delete_strict") {
                fm.command(of13::OFPFC_DELETE_STRICT);
            } else {
                fm.command(of13::OFPFC_DELETE);
            }

            processInfoDelete(fm, req);

            if (req.find("match") != req.end()) {
                const auto &matches = req.at("match").object_items();
                processMatches(fm, matches);
            }
            // note: OvS does not support finding flows by actions. It will ignore that section.
            if (req.find("actions") != req.end()) {
                const auto &actions = req.at("actions").array_items();
                processActions(fm, actions);
            }

            auto dpid = json_cast<uint64_t>(req.at("dpid"));
            auto sw = sw_m_->getSwitch(dpid);
            if (!sw) {
                throw "switch not found";
            }
            sw->connection()->send(fm);

            auto msg = "have sent flow_mod with DELETE command" + response_.str();
            response_.str("");
            return json11::Json::object{
                    {"RestFlowMod", msg.c_str()}
            };
        }
    } catch (const std::string &errMsg) {
        return json11::Json::object{
                {"RestFlowMod", errMsg.c_str()}
        };
    } catch (...) {
        return json11::Json::object{
                {"RestFlowMod", "Some error on request handling"}
        };
    }
    return json11::Json::object{};
}

void RestFlowMod::processAction(const json11::Json::object &action, of13::ApplyActions &applyActions)
{
    using namespace fluid_fix::actions;
    try {
        auto type = action.at("type").string_value();
        if (type == "SET_FIELD") {
             of13::OXMTLV *field = doSwitch(action.at("field").string_value(),
                                           action.at("value").string_value());
            auto act = new of13::SetFieldAction{field};
            applyActions.add_action(act);
            return;
        }
        if (type == "OUTPUT") {
            auto &act = *(new OUTPUT);
            SET_ACT(port)
            SET_ACT(max_len)
            applyActions.add_action(&act);
            return;
        }
        if (type == "COPY_TTL_OUT") {
            auto &act = *(new COPY_TTL_OUT);
            applyActions.add_action(&act);
            return;
        }
        if (type == "COPY_TTL_IN") {
            auto &act = *(new COPY_TTL_IN);
            applyActions.add_action(&act);
            return;
        }
        if (type == "SET_MPLS_TTL") {
            auto &act = *(new SET_MPLS_TTL);
            SET_ACT(mpls_ttl)
            applyActions.add_action(&act);
            return;
        }
        if (type == "DEC_MPLS_TTL") {
            auto &act = *(new DEC_MPLS_TTL);
            applyActions.add_action(&act);
            return;
        }
        if (type == "PUSH_VLAN") {
            auto &act = *(new PUSH_VLAN);
            SET_ACT(ethertype)
            applyActions.add_action(&act);
            return;
        }
        if (type == "POP_VLAN") {
            auto &act = *(new POP_VLAN);
            applyActions.add_action(&act);
            return;
        }
        if (type == "PUSH_MPLS") {
            auto &act = *(new PUSH_MPLS);
            SET_ACT(ethertype)
            applyActions.add_action(&act);
            return;
        }
        if (type == "POP_MPLS") {
            auto &act = *(new POP_MPLS);
            SET_ACT(ethertype)
            applyActions.add_action(&act);
            return;
        }
        if (type == "SET_QUEUE") {
            auto &act = *(new SET_QUEUE);
            SET_ACT(queue_id)
            applyActions.add_action(&act);
            return;
        }
        if (type == "GROUP") {
            auto &act = *(new GROUP);
            SET_ACT(group_id)
            applyActions.add_action(&act);
            return;
        }
        if (type == "SET_NW_TTL") {
            auto &act = *(new SET_NW_TTL);
            SET_ACT(nw_ttl)
            applyActions.add_action(&act);
            return;
        }
        if (type == "DEC_NW_TTL") {
            auto &act = *(new DEC_NW_TTL);
            applyActions.add_action(&act);
            return;
        }
        if (type == "PUSH_PBB") {
            auto &act = *(new PUSH_PBB);
            SET_ACT(ethertype);
            applyActions.add_action(&act);
            return;
        }
        if (type == "POP_PBB") {
            auto &act = *(new POP_PBB);
            applyActions.add_action(&act);
            return;
        }
        if (type == "EXPERIMENTER") {
            auto &act = *(new EXPERIMENTER);
            SET_ACT(experimenter);
            applyActions.add_action(&act);
            return;
        }
    } catch (...) {
        response_ << " Incorrect input in an action " << action.at("type").string_value() << ". This action is ignored.";
    }
}

of13::OXMTLV *doSwitch(const std::string &field,
                      const std::string &value)
{
    using namespace fluid_fix::matches;
    if (field == "metadata") {
        return new metadata{string_cast<uint64_t>(value)};
    }
    if (field == "eth_src") {
        return new eth_src{value};
    }
    if (field == "eth_dst") {
        return new eth_dst{value};
    }
    if (field == "eth_type") {
        return new eth_type{string_cast<uint16_t>(value)};
    }
    if (field == "vlan_vid") {
        return new vlan_vid{string_cast<uint16_t>(value)};
    }
    if (field == "ip_dscp") {
        return new ip_dscp{string_cast<uint8_t>(value)};
    }
    if (field == "ip_ecn") {
        return new ip_ecn{string_cast<uint8_t>(value)};
    }
    if (field == "ip_proto") {
        return new ip_proto{string_cast<uint8_t>(value)};
    }
    if (field == "ipv4_src") {
        return new ipv4_src{value};
    }
    if (field == "ipv4_dst") {
        return new ipv4_dst{value};
    }
    if (field == "tcp_src") {
        return new tcp_src{string_cast<uint16_t>(value)};
    }
    if (field == "tcp_dst") {
        return new tcp_dst{string_cast<uint16_t>(value)};
    }
    if (field == "udp_src") {
        return new udp_src{string_cast<uint16_t>(value)};
    }
    if (field == "udp_dst") {
        return new udp_dst{string_cast<uint16_t>(value)};
    }

    if (field == "arp_spa") {
        return new arp_spa{value};
    }
    if (field == "arp_tpa") {
        return new arp_tpa{value};
    }
    if (field == "arp_sha") {
        return new arp_sha{value};
    }
    if (field == "arp_tha") {
        return new arp_tha{value};
    }
    if (field == "arp_op") {
        return new arp_op{string_cast<uint16_t>(value)};
    }
    throw "unsupported field name";
}


json11::Json RestFlowMod::handleDELETE(std::vector<std::string> params, std::string body)
{
    constexpr uint8_t FLOWENTRY = 0, CLEAR = 1, DPID = 2;
    if (params.at(FLOWENTRY) != "flowentry" || params.at(CLEAR) != "clear") {
        return json11::Json::object{
                {"RestFlowMod", "Unsupported parameter"}
        };
    }
    try {
        auto dpid = string_cast<uint64_t>(params.at(DPID));
        auto sw = sw_m_->getSwitch(dpid);
        if (!sw) {
            throw "switch not found";
        }

        clearTables(dpid);
        // let switch work
        for (uint8_t i = 0; i < ctrl_->handler_table(); i++){
            installGoto(dpid, i);
        }
        installTableMissRule(dpid, ctrl_->handler_table());

        auto msg = "all tables have been initialized" + response_.str();
        response_.str("");
        return json11::Json::object{
                {"RestFlowMod", msg.c_str()}
        };
    } catch (const std::string &errMsg) {
        return json11::Json::object{
                {"RestFlowMod", errMsg.c_str()}
        };
    } catch (...) {
        return json11::Json::object{
                {"RestFlowMod", "Some error on request handling"}
        };
    }
    return json11::Json::object{};
}

void RestFlowMod::clearTables(uint64_t dpid)
{
    of13::FlowMod fm;
    fm.command(of13::OFPFC_DELETE);
    fm.table_id(of13::OFPTT_ALL);
    fm.cookie(0x0);
    fm.cookie_mask(0x0);
    fm.out_port(of13::OFPP_ANY);
    fm.out_group(of13::OFPG_ANY);
    auto tmp = sw_m_->getSwitch(dpid);
    tmp->connection()->send(fm);
}

void RestFlowMod::installTableMissRule(uint64_t dpid, uint8_t table_id)
{
    of13::FlowMod fm;
    fm.command(of13::OFPFC_ADD);
    fm.priority(0);
    fm.cookie(0);
    fm.idle_timeout(0);
    fm.hard_timeout(0);
    fm.table_id(table_id);
    fm.flags( of13::OFPFF_CHECK_OVERLAP |
              of13::OFPFF_SEND_FLOW_REM );
    of13::ApplyActions act;
    of13::OutputAction out(of13::OFPP_CONTROLLER, 0);
    act.add_action(out);
    fm.add_instruction(act);
    sw_m_->getSwitch(dpid)->connection()->send(fm);
}

void RestFlowMod::installGoto(uint64_t dpid, uint8_t table_id)
{
    of13::FlowMod fm;
    fm.command(of13::OFPFC_ADD);
    fm.priority(0);
    fm.cookie(0);
    fm.idle_timeout(0);
    fm.hard_timeout(0);
    fm.table_id(table_id);
    fm.flags( of13::OFPFF_CHECK_OVERLAP |
              of13::OFPFF_SEND_FLOW_REM );
    of13::ApplyActions act;
    of13::GoToTable go_to_table(table_id + 1);
    fm.add_instruction(go_to_table);
    sw_m_->getSwitch(dpid)->connection()->send(fm);
}