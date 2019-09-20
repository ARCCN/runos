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
#include "lib/switch_and_port.hpp"
#include <json.hpp>

#include <ostream>
#include <set>
#include <chrono>

namespace runos {

struct DiscoveredLink {
    typedef std::chrono::time_point<std::chrono::steady_clock>
            valid_through_t;

    DiscoveredLink(switch_and_port source, switch_and_port target, valid_through_t through)
        : source(source)
        , target(target)
        , valid_through(through) {}

    switch_and_port source;
    switch_and_port target;
    valid_through_t valid_through;

    nlohmann::json to_json() const {
        return {
            {"source_dpid", source.dpid},
            {"source_port", source.port},
            {"target_dpid", target.dpid},
            {"target_port", target.port}
        };
    }
};

/**
  Discovers links between switches and monitors it for failures.
  The interface assumes that all links bidirectional and 
  ensures (from <= to) in all signals when applicable.
*/
class ILinkDiscovery {
    INTERFACE(ILinkDiscovery, "link-discovery")
signals:
    /**
     * This signal emitted when new link discovered or broken link recovered.
     */
    virtual void linkDiscovered(switch_and_port from, switch_and_port to) = 0;

    /**
     * This signal emitted when link discovered before was unusable.
     * That can occur but not limited to from:
     *     - Switch down events
     *     - Port down events
     *     - Link down events
     */
    virtual void linkBroken(switch_and_port from, switch_and_port to) = 0;

    // return by value because we don't want to use mutex (faster for REST calls?)
    virtual const std::set<DiscoveredLink> links() const = 0;
    virtual switch_and_port other(switch_and_port) const = 0;
};

} // namespace runos

Q_DECLARE_INTERFACE(runos::ILinkDiscovery, "ru.arccn.link-discovery/0.2")
Q_DECLARE_METATYPE(runos::switch_and_port)

