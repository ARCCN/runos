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

struct GroupTableStats: rest::resource {
    UnsafeSwitchPtr sw;

    explicit GroupTableStats(SwitchPtr sw)
    : sw(sw.not_null()) {}
    
    rest::ptree Get() const override {
        rest::ptree ret;
        try {
            rest::ptree groups;
            auto f = sw->connection()->agent()->request_group_stats();
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Group table reply timeout!" );
            for (auto gs : f.get()) {
                rest::ptree gpt;
                gpt.put("group_id", gs.group_id());
                gpt.put("duration_sec", gs.duration_sec());
                gpt.put("duration_nsec", gs.duration_nsec());
                gpt.put("ref_count", gs.ref_count());
                gpt.put("packet_count", gs.packet_count());
                gpt.put("byte_count", gs.byte_count());
                rest::ptree buckets;
                for (auto bucket: gs.bucket_stats()) {
                    rest::ptree bpt;
                    bpt.put("byte_count", bucket.byte_count());
                    bpt.put("packet_count", bucket.packet_count());
                    buckets.push_back(std::make_pair("", std::move(bpt)));
                }
                gpt.add_child("bucket_stats", buckets);
                groups.push_back(std::make_pair("", std::move(gpt)));
            }
            ret.add_child("array", groups);
            ret.put("_size", groups.size());
        } catch (const OFAgent::request_error& e) {
            LOG(WARNING) << "[GroupTableRest] - " << e.what();
        }
        return ret;
    }
     
};


struct GroupTableDesc: rest::resource {
    UnsafeSwitchPtr sw;

     explicit GroupTableDesc(SwitchPtr sw)
    : sw(sw.not_null()) {}
    
    rest::ptree Get() const override {
        rest::ptree ret;
        try {
            rest::ptree groups;

            auto f = sw->connection()->agent()->request_group_desc();
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Group table reply timeout!" );
            for (auto& gs : f.get()) {
                rest::ptree gpt;
                gpt.put("group_id", gs.group_id());
                std::string type;
                switch (gs.type()){
                    case of13::OFPGT_ALL:
                        type = "OFPGT_ALL";
                        break;
                    case of13::OFPGT_SELECT:
                        type = "OFPGT_SELECT";
                        break;
                    case of13::OFPGT_INDIRECT:
                        type = "OFPGT_INDIRECT";
                        break;
                    case of13::OFPGT_FF:
                        type = "OFPGT_FF";
                        break;
                    default:
                        type = std::to_string(gs.type());
                        break;
                }
                gpt.put("type", type);

                rest::ptree bs;
                for (auto &bucket: gs.buckets()){
                    rest::ptree bpt;
                    bpt.put("weight", bucket.weight());
                    std::string port;
                    switch (bucket.watch_port()){
                        case of13::OFPP_MAX:
                            port = "OFPP_MAX";
                            break;
                        case of13::OFPP_IN_PORT:
                            port = "OFPP_IN_PORT";
                            break;
                        case of13::OFPP_TABLE:
                            port = "OFPP_TABLE";
                            break;
                        case of13::OFPP_NORMAL:
                            port = "OFPP_NORMAL";
                            break;
                        case of13::OFPP_FLOOD:
                            port = "OFPP_FLOOD";
                            break;
                        case of13::OFPP_ALL:
                            port = "OFPP_ALL";
                            break;
                        case of13::OFPP_CONTROLLER:
                            port = "OFPP_CONTROLLER";
                            break;
                        case of13::OFPP_LOCAL:
                            port = "OFPP_LOCAL";
                            break;
                        case of13::OFPP_ANY:
                            port = "OFPP_ANY";
                            break;
                        default:
                            port = std::to_string(bucket.watch_port());
                            break;
                    }
                    bpt.put("watch_port", port);
                    std::string group;
                    switch (bucket.watch_group()){
                        case of13::OFPG_MAX:
                            group = "OFPG_MAX";
                            break;
                        case of13::OFPG_ALL:
                            group = "OFPG_ALL";
                            break;
                        case of13::OFPG_ANY:
                            group = "OFPG_ANY";
                            break;
                        default:
                            group = std::to_string(bucket.watch_group());
                            break;
                    }
                    bpt.put("watch_group", group);
                    fluid_msg::ActionSet acts = bucket.get_actions();
                    rest::ptree poutput, pgroup;
                    for (auto act: acts.action_set()){
                        rest::parsingAction(act, "actions", bpt, poutput, pgroup);
                    }
                    if (poutput.size())
                        bpt.add_child("actions.output", poutput);
                    if (pgroup.size())
                        bpt.add_child("actions.group", pgroup);

                    bs.push_back(std::make_pair("", std::move(bpt)));
                }
                gpt.add_child("buckets", bs);
                groups.push_back(std::make_pair("", std::move(gpt)));
            }
            ret.add_child("array", groups);
            ret.put("_size", groups.size());
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[GroupTableRest] - " << e.what();
        }
        return ret;
    }
     
    
};

class GroupTableRest : public Application
{
    SIMPLE_APPLICATION(GroupTableRest, "group-table-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto rest_ = RestListener::get(loader);
        auto sw_m = SwitchManager::get(loader);

        rest_->mount(path_spec("/switches/(\\d+)/group-desc/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return GroupTableDesc { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });

       rest_->mount(path_spec("/switches/(\\d+)/group-stats/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return GroupTableStats { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });

    }
};

REGISTER_APPLICATION(GroupTableRest, {"rest-listener", "switch-manager", ""})

}
