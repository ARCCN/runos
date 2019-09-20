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

#include "Application.hpp"
#include "RestListener.hpp"
#include "SwitchManager.hpp"
#include "StatsRulesManager.hpp"

#include "PortImpl.hpp"
#include "SwitchImpl.hpp"

#include "boost/lexical_cast.hpp"

namespace runos {

struct AppInterface {
    virtual FlowStatsBucketPtr bucket(const std::string& name) const = 0;
    virtual void addBuckets(BucketsMap buckets) = 0;
    virtual void removeBuckets(const BucketsNames& names) = 0;
};

class EndpointTrafficStats : public rest::resource
{
public:
    using ptree = rest::ptree;

    explicit EndpointTrafficStats(AppInterface* app,
                                  StatsRulesManager* srm,
                                  const PortPtr& port,
                                  uint16_t stag,
                                  ForwardingType ftype)
        : app_(app)
        , rules_mgr_(srm)
        , port_(port.not_null())
        , stag_(stag)
        , ftype_(ftype)
    { }

    ptree Get() const override
    {
        if (ftype_ != (+ForwardingType::None)) {
            return trafficStats(ftype_);
        }

        rest::ptree ret;

        auto unicast = trafficStats(ForwardingType::Unicast);
        auto broadcast = trafficStats(ForwardingType::Broadcast);
        auto multicast = trafficStats(ForwardingType::Multicast);

        ret.add_child("unicast", unicast);
        ret.add_child("broadcast", broadcast);
        ret.add_child("multicast", multicast);

        return ret;
    }

    ptree Put(ptree const& pt) override
    {
        if ((+ForwardingType::None)!= ftype_) {
            throw std::invalid_argument(
                "Stats rules cannot be installed for specific traffic type");
        }

        auto buckets = rules_mgr_->installEndpointRules(port_, stag_);
        app_->addBuckets(buckets);

        return ptree();
    }

    ptree Delete() override
    {
        if ((+ForwardingType::None) == ftype_) {
            throw std::invalid_argument(
                "Stats rules cannot be deleted for specific traffic type");
        }

        auto names = rules_mgr_->deleteEndpointRules(port_, stag_);
        app_->removeBuckets(names);

        return ptree();
    }

private:
    AppInterface* app_;
    StatsRulesManager* rules_mgr_;
    PortPtr port_;
    uint16_t stag_;
    ForwardingType ftype_;

    rest::ptree trafficStats(ForwardingType type) const
    {
        rest::ptree ret;
        auto bucket = app_->bucket(bucket_name());

        auto stats = bucket->stats();
        auto& speed = stats.current_speed;
        auto& max = stats.max_speed;
        auto& integral = stats.integral;

        ret.put("current-speed.rx-packets", speed.packets());
        ret.put("current-speed.rx-bytes", speed.bytes());

        ret.put("integral.rx-packets", integral.packets());
        ret.put("integral.rx-bytes", integral.bytes());

        ret.put("max.rx-packets", max.packets());
        ret.put("max.rx-bytes", max.bytes());

        return ret;
    }

    std::string bucket_name() const
    {
        return std::string("traffic") + "-" +
               ftype_._to_string() + "_" +
               std::to_string(port_->switch_().not_null()->dpid()) + "_" +
               std::to_string(port_->number()) + "_" +
               std::to_string(stag_);
    }
};

class StatsRulesManagerRest : public Application, public AppInterface
{
SIMPLE_APPLICATION(StatsRulesManagerRest, "stats-rules-manager-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using namespace rest;

        auto sw_mgr = SwitchManager::get(loader);
        auto rest_ = RestListener::get(loader);
        auto rules_mgr = StatsRulesManager::get(loader);

        rest_->mount(path_spec("/switches/(\\d+)/ports/(\\d+)/"
                               "vlan/(\\d+)/traffic_stats/"
                               "(?:(multicast|broadcast|unicast)/)?"),
            [=](const path_match& m)
            {
                try {
                    auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                    auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                    auto vlan = boost::lexical_cast<uint16_t>(m[3].str());

                    auto ftype_str = m[4].str();
                    ftype_str = ftype_str.empty() ? "none" : ftype_str;
                    ftype_str[0] = std::toupper(ftype_str[0]);
                    auto ftype = ForwardingType::_from_string(ftype_str.c_str());

                    return EndpointTrafficStats{
                        this,
                        rules_mgr,
                        sw_mgr->switch_(dpid).not_null()->port(port_no),
                        vlan,
                        ftype};
                } catch (const boost::bad_lexical_cast& e) {
                    THROW(http_error(400), "Bad request: {}", e.what());
                } catch (const bad_pointer_access& e) {
                    THROW(http_error(404), "Port not found");
                } catch (const std::invalid_argument& e) {
                    THROW(http_error(400), "Bad request: {}", e.what());
                } catch (const std::out_of_range& e) {
                    THROW(http_error(404), "Vlan not found");
                }
            });
    }

    FlowStatsBucketPtr bucket(const std::string& name) const override
    {
        std::lock_guard<std::mutex> lock(mut_);
        return buckets_.at(name);
    }

    void addBuckets(BucketsMap buckets) override
    {
        std::lock_guard<std::mutex> lock(mut_);
        for (auto& bucket: buckets) {
            buckets_.emplace(bucket.first, std::move(bucket.second));
        }
    }

    void removeBuckets(const BucketsNames& names) override
    {
        std::lock_guard<std::mutex> lock(mut_);
        for (const auto& name: names) {
            buckets_.erase(name);
        }
    }

private:
    std::unordered_map<std::string, FlowStatsBucketPtr> buckets_;
    mutable std::mutex mut_;
};

REGISTER_APPLICATION(StatsRulesManagerRest, {"rest-listener", "switch-manager",
                                             "stats-rules-manager", ""})

} // namespace runos

