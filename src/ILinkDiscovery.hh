/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include "Application.hh"
#include "Loader.hh"

struct switch_and_port
{
    uint64_t dpid;
    uint32_t port;
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
};

//////
inline bool operator==(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) == std::tie(b.dpid, b.port); }
inline bool operator!=(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) != std::tie(b.dpid, b.port); }

inline bool operator<(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) < std::tie(b.dpid, b.port); }
inline bool operator>(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) > std::tie(b.dpid, b.port); }

namespace std {
    template<>
    struct hash<switch_and_port>
    {
        size_t operator()(const switch_and_port & ap) const {
            return hash<uint64_t>()(ap.dpid) ^ hash<uint32_t>()(ap.port);
        }
    };
}
/////

Q_DECLARE_INTERFACE(ILinkDiscovery, "ru.arccn.link-discovery/0.2")
Q_DECLARE_METATYPE(switch_and_port)
