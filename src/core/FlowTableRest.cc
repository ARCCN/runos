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

#include "SwitchManager.hpp"
#include "RestListener.hpp"
#include "api/OFAgent.hpp"
#include "lib/action_parsing.hpp"

#include <runos/core/literals.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/throw.hpp>

#include <boost/thread/future.hpp>
#include <boost/chrono.hpp>
#include <boost/lexical_cast.hpp>

namespace runos {

namespace of13 = fluid_msg::of13;

struct FlowTableCollection : rest::resource {
    UnsafeSwitchPtr sw;

    explicit FlowTableCollection(SwitchPtr sw)
    : sw(sw.not_null()) {}

    static void parsingMatch(of13::Match match, rest::ptree& pt) {
        pt.put("match", "");
        // OF
        if (match.in_port())
            pt.put("match.in_port", match.in_port()->value());
        if (match.metadata()) {
            pt.put("match.metadata.value", match.metadata()->value());
            pt.put("match.metadata.mask", match.metadata()->mask());
        }

        // Eth / VLAN
        if (match.vlan_vid())
            pt.put("match.vlan_vid", match.vlan_vid()->value());
        if (match.vlan_pcp())
            pt.put("match.vlan_pcp", match.vlan_pcp()->value());
        if (match.eth_type())
            pt.put("match.eth_type", match.eth_type()->value());
        if (match.eth_src()) {
            pt.put("match.eth_src.value", match.eth_src()->value().to_string());
            pt.put("match.eth_src.mask", match.eth_src()->mask().to_string());
        }
        if (match.eth_dst()) {
            pt.put("match.eth_dst.value", match.eth_dst()->value().to_string());
            pt.put("match.eth_dst.mask", match.eth_dst()->mask().to_string());
        }

        // IP
        if (match.ipv4_src())
            pt.put("match.ipv4_src",
                    rest::ip_to_string(match.ipv4_src()->value().getIPv4()));
        if (match.ipv4_dst())
            pt.put("match.ipv4_dst",
                    rest::ip_to_string(match.ipv4_dst()->value().getIPv4()));
        if (match.ip_proto())
            pt.put("match.ip_proto", match.ip_proto()->value());
        if (match.ip_dscp())
            pt.put("match.ip_dscp", match.ip_dscp()->value());

        // ARP
        if (match.arp_op())
            pt.put("match.arp_op", match.arp_op()->value());
        if (match.arp_spa())
            pt.put("match.arp_spa",
                    rest::ip_to_string(match.arp_spa()->value().getIPv4()));
        if (match.arp_tpa())
            pt.put("match.arp_tpa",
                    rest::ip_to_string(match.arp_tpa()->value().getIPv4()));
        if (match.arp_sha())
            pt.put("match.arp_sha", match.arp_sha()->value().to_string());
        if (match.arp_tha())
            pt.put("match.arp_tha", match.arp_tha()->value().to_string());
    }

    rest::ptree Get() const override {
        rest::ptree ret;
        rest::ptree flows;

        auto agent = sw->connection()->agent();
        ofp::flow_stats_request req;
        try {
            auto f = agent->request_flow_stats(req);
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Flow table reply timeout!" );

            for (auto fs : f.get()) {
                rest::ptree fpt;
                fpt.put("table_id", fs.table_id());
                fpt.put("duration_sec", fs.duration_sec());
                fpt.put("priority", fs.priority());
                fpt.put("idle_timeout", fs.idle_timeout());
                fpt.put("hard_timeout", fs.hard_timeout());
                fpt.put("cookie", fs.cookie());
                fpt.put("packet_count", fs.packet_count());
                fpt.put("byte_count", fs.byte_count());

                parsingMatch(fs.match(), fpt);

                auto instrcts = fs.instructions();
                auto instr_set = instrcts.instruction_set();
                for (auto it = instr_set.begin(); it != instr_set.end(); it++) {
                    of13::Instruction* instr = *it;
                    fluid_msg::ActionList lst;
                    fluid_msg::ActionSet acts;
                    std::list<fluid_msg::Action*> alist;
                    std::set<fluid_msg::Action*, fluid_msg::comp_action_set_order> aset;
                    rest::ptree output_collector, group_collector;
                    rest::ptree woutput_collector, wgroup_collector;

                    switch (instr->type()) {
                    case (of13::OFPIT_GOTO_TABLE):
                        fpt.put("instructions.goto_table.table_id",
                                ((of13::GoToTable*)instr)->table_id());
                        break;
                    case (of13::OFPIT_WRITE_METADATA):
                        fpt.put("instructions.write_metadata.metadata",
                                ((of13::WriteMetadata*)instr)->metadata());
                        fpt.put("instructions.write_metadata.metadata_mask",
                                ((of13::WriteMetadata*)instr)->metadata_mask());
                        break;
                    case (of13::OFPIT_WRITE_ACTIONS):
                        acts = ((of13::WriteActions*)instr)->actions();
                        aset = acts.action_set();
                        for (auto it = aset.begin(); it != aset.end(); it++) {
                            rest::parsingAction(*it, "instructions.write_actions",
                                         fpt, woutput_collector, wgroup_collector);
                        }
                        if (woutput_collector.size()) {
                            fpt.add_child("instructions.write_actions.output",
                                                                woutput_collector);
                        }
                        if (wgroup_collector.size()) {
                            fpt.add_child("instructions.write_actions.group",
                                                                 wgroup_collector);
                        }
                        break;
                    case (of13::OFPIT_APPLY_ACTIONS):
                        lst = ((of13::ApplyActions*)instr)->actions();
                        alist = lst.action_list();
                        for (auto it = alist.begin(); it != alist.end(); it++) {
                            rest::parsingAction(*it, "instructions.apply_actions",
                                           fpt, output_collector, group_collector);
                        }
                        if (output_collector.size()) {
                            fpt.add_child("instructions.apply_actions.output",
                                                                 output_collector);
                        }
                        if (group_collector.size()) {
                            fpt.add_child("instructions.apply_actions.group",
                                                                  group_collector);
                        }
                        break;
                    case (of13::OFPIT_CLEAR_ACTIONS):
                        fpt.put("instructions.clear_actions", "");
                        break;
                    case (of13::OFPIT_METER):
                        fpt.put("instructions.meter.meter_id",
                                ((of13::Meter*)instr)->meter_id());
                        break;
                    default:
                        LOG(ERROR) << "Unhandled instruction type: "
                                   << instr->type();
                        break;
                    }
                }

                flows.push_back(std::make_pair("", std::move(fpt)));
            }
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[FlowTableCollection] - " << e.what();
        }
        ret.add_child("array", flows);
        ret.put("_size", flows.size());
        return ret;
    }
};

class FlowTableRest : public Application
{
    SIMPLE_APPLICATION(FlowTableRest, "flow-table-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto rest_ = RestListener::get(loader);
        auto sw_m = SwitchManager::get(loader);

        rest_->mount(path_spec("/switches/(\\d+)/flow-tables/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return FlowTableCollection { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });
    }
 
};

REGISTER_APPLICATION(FlowTableRest, {"rest-listener", "switch-manager", ""})

}
