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

#include <runos/core/literals.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/throw.hpp>

#include <boost/thread/future.hpp>
#include <boost/chrono.hpp>
#include <boost/lexical_cast.hpp>

namespace runos {

namespace of13 = fluid_msg::of13;


struct MeterTableCollection : rest::resource {
    UnsafeSwitchPtr sw;

    explicit MeterTableCollection(SwitchPtr sw)
    : sw(sw.not_null()) {}
    
    rest::ptree Get() const override {
        rest::ptree ret;
        try {
            rest::ptree meters;

            auto f = sw->connection()->agent()->request_meter_stats();
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Meter table reply timeout!" );
            for (auto ms : f.get()) {
                rest::ptree mpt;
                mpt.put("meter_id", ms.meter_id());
                mpt.put("duration_sec", ms.duration_sec());
                mpt.put("duration_nsec", ms.duration_nsec());
                mpt.put("packet_count", ms.byte_in_count());
                mpt.put("byte_count", ms.packet_in_count());
                meters.push_back(std::make_pair("", std::move(mpt)));
            }
            ret.add_child("array", meters);
            ret.put("_size", meters.size());
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[MeterTableRest] - " << e.what();
        }
        return ret;
    }
};

struct MeterTableConfig : rest::resource {
    UnsafeSwitchPtr sw;

    explicit MeterTableConfig(SwitchPtr sw)
    : sw(sw.not_null()) {}
    
    rest::ptree Get() const override {
        rest::ptree ret;
        try {
            rest::ptree meters;

            auto f = sw->connection()->agent()->request_meter_config();
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Meter table reply timeout!" );
            for (auto mc : f.get()) {
                rest::ptree mpt;
                std::string flags = "";
                auto num_flags = mc.flags();
                if (num_flags & of13::OFPMF_KBPS)
                    flags += "OFPMF_KBPS ";
                if (num_flags & of13::OFPMF_PKTPS)
                    flags += "OFPMF_PKTPS ";
                if (num_flags & of13::OFPMF_BURST)
                    flags += "OFPMF_BURST ";
                if (num_flags & of13::OFPMF_KBPS)
                    flags += "OFPMF_STATS ";
                mpt.put("flags", flags);
                mpt.put("num_flags", mc.flags());
                mpt.put("meter_id", mc.meter_id());
                rest::ptree bands;
                for (auto &band: mc.bands().meter_bands()) {
                    rest::ptree bpt;
                    bpt.put("type", band->type());
                    bpt.put("len", band->len());
                    bpt.put("rate", band->rate());
                    bpt.put("burst_size", band->burst_size());
                    bands.push_back(std::make_pair("", std::move(bpt)));
                }
                mpt.add_child("bands", bands);
                meters.push_back(std::make_pair("", std::move(mpt)));
            }
            ret.add_child("array", meters);
            ret.put("_size", meters.size());
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[MeterTableRest] - " << e.what();
        }
        return ret;
    }
};

struct MeterTableFeatures : rest::resource {
    UnsafeSwitchPtr sw;

    explicit MeterTableFeatures(SwitchPtr sw)
    : sw(sw.not_null()) {}
    
    rest::ptree Get() const override {
        rest::ptree ret;

        try {
            auto f = sw->connection()->agent()->request_meter_features();
            auto status = f.wait_for(boost::chrono::seconds(5));
            THROW_IF(status == boost::future_status::timeout,
                        rest::http_error(504), "Meter table reply timeout!" );
            rest::ptree mpt;
            auto mf = f.get();
            mpt.put("max_meter", mf.max_meter());
            mpt.put("band_types", mf.band_types());
            mpt.put("capabilities", mf.capabilities());
            mpt.put("max_bands",mf.max_bands());
            mpt.put("max_color", mf.max_color());
            ret.add_child("features", mpt);
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[MeterTableRest] - " << e.what();
        }
        return ret;
    }
};


class MeterTableRest : public Application
{
    SIMPLE_APPLICATION(MeterTableRest, "meter-table-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto rest_ = RestListener::get(loader);
        auto sw_m = SwitchManager::get(loader); 
 
        rest_->mount(path_spec("/switches/(\\d+)/meter-stats/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return MeterTableCollection { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/meter-config/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return MeterTableConfig { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });
       
        rest_->mount(path_spec("/switches/(\\d+)/meter-features/"), [=](const path_match& m) {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return MeterTableFeatures { sw_m->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404) );
            }
        });
    }
};

REGISTER_APPLICATION(MeterTableRest, {"rest-listener", "switch-manager", ""})

}
