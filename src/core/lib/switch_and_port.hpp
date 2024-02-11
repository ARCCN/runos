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

#include <tuple>
#include <fmt/core.h>

namespace runos {

struct switch_and_port {
    uint64_t dpid;
    uint32_t port;

    switch_and_port() = default;
    switch_and_port(uint64_t dpid, uint32_t port): dpid(dpid), port(port) {}
    friend std::ostream& operator<<(std::ostream& out, switch_and_port const& loc)
    { return out << loc.dpid << ':' << loc.port; }
};

inline bool operator==(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) == std::tie(b.dpid, b.port); }
inline bool operator!=(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) != std::tie(b.dpid, b.port); }

inline bool operator<(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) < std::tie(b.dpid, b.port); }
inline bool operator>(switch_and_port a, switch_and_port b)
{ return std::tie(a.dpid, a.port) > std::tie(b.dpid, b.port); }

} // namespace runos

namespace std {
    template<>
    struct hash<runos::switch_and_port>
    {
        size_t operator()(const runos::switch_and_port & ap) const {
            return hash<uint64_t>()(ap.dpid) ^ hash<uint32_t>()(ap.port);
        }
    };
}

template<>
struct fmt::formatter<runos::switch_and_port> : fmt::formatter<std::string>
{
    auto format(runos::switch_and_port sw, format_context &ctx) const -> decltype(ctx.out())
    {
        return format_to( ctx.out(), "{}:{}", sw.dpid, sw.port );
    }
};