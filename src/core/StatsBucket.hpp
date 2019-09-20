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
 
#pragma once

#include "api/Statistics.hpp"
#include "lib/kwargs.hpp"
#include "Application.hpp"
#include "Loader.hpp"
#include <runos/core/safe_ptr.hpp>

#include <fluid/of13/of13match.hh>

#include <chrono>
#include <string>
#include <utility>
#include <memory>

namespace runos {

namespace flow_selector {
    constexpr kwarg<struct dpid_tag, std::vector<uint64_t> > dpid;
    constexpr kwarg<struct table_tag, uint8_t> table;
    constexpr kwarg<struct out_port_tag, std::vector<uint32_t> > out_port;
    constexpr kwarg<struct out_group_tag, std::vector<uint32_t> > out_group;
    constexpr kwarg<struct cookie_tag, uint64_t> cookie;
    constexpr kwarg<struct cookie_mask_tag, uint64_t> cookie_mask;
    constexpr kwarg<struct match_tag, fluid_msg::of13::Match> match;
};

class FlowStatsBucket : public QObject
{
    Q_OBJECT
public:
    virtual int id() const = 0; // primary key
    virtual std::string name() const = 0; // key

    virtual Statistics<FlowMeasurement> stats() const = 0;
    virtual ~FlowStatsBucket() { }
signals:
    void updated();
};

using FlowStatsBucketPtr = safe::shared_ptr<FlowStatsBucket>;
using UnsafeFlowStatsBucketPtr = std::shared_ptr<FlowStatsBucket>;
using FlowStatsBucketWeakPtr = safe::weak_ptr<FlowStatsBucket>;

class StatsBucketManager : public Application
{
    Q_OBJECT
    SIMPLE_APPLICATION(StatsBucketManager, "stats-bucket-manager")
public:
    StatsBucketManager();
    ~StatsBucketManager() noexcept;
    
    void init(Loader* loader, const Config& config) override;
    //void startUp(Loader* loader) override;
    
    FlowStatsBucketPtr bucket(int id) const;
    FlowStatsBucketPtr bucket(std::string const& name) const;

    using duration = std::chrono::milliseconds;

    template<class ...Args>
    FlowStatsBucketPtr aggregateFlows(duration poll_interval,
                                      std::string name,
                                      Args&& ...args)
    {
        return aggregateFlows(
            poll_interval,
            std::move(name),
            { std::forward<Args>(args)... }
        );
    }

    using FlowSelector = decltype(kwargs(
        flow_selector::dpid,
        flow_selector::table,
        flow_selector::out_port,
        flow_selector::out_group,
        flow_selector::cookie,
        flow_selector::cookie_mask,
        flow_selector::match
    ));

    FlowStatsBucketPtr aggregateFlows(duration poll_interval,
                                      std::string name,
                                      FlowSelector selector);

private:
    struct implementation;
    std::unique_ptr<implementation> impl;
};

} // namespace runos
