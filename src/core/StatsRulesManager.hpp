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

#include "Application.hpp"
#include "Loader.hpp"
#include "api/Port.hpp"
#include "api/Switch.hpp"
#include "StatsBucket.hpp"

#include <fluid/of13msg.hh>

#include <vector>
#include <memory>

namespace runos {

namespace of13 = fluid_msg::of13;

using FlowModPtr = std::shared_ptr<of13::FlowMod>;
using FlowModPtrSequence = std::vector<FlowModPtr>;
using BucketsMap = std::unordered_map<std::string, FlowStatsBucketPtr>;
using BucketsNames = std::vector<std::string>;

class StatsRulesManager : public Application
{
    SIMPLE_APPLICATION(StatsRulesManager, "stats-rules-manager")
public:
    void init(Loader* loader, const Config&) override;

    BucketsMap installEndpointRules(PortPtr port, uint16_t stag);
    BucketsNames deleteEndpointRules(PortPtr port, uint16_t stag);

public slots:
    void installRules(PortPtr new_port);
    void deleteRules(PortPtr down_port);

public:
    void clearStatsTable(SwitchPtr sw);

private:
    StatsBucketManager* bucket_mgr_;
};

} // namespace runos
