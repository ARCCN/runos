#pragma once

#include "Application.hh"
#include "Loader.hh"

struct switch_and_port {
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
inline bool operator==(const switch_and_port & a, const switch_and_port & b)
{ return std::tie(a.dpid, a.port) == std::tie(b.dpid, b.port); }

inline bool operator<(const switch_and_port & a, const switch_and_port & b)
{ return std::tie(a.dpid, a.port) < std::tie(b.dpid, b.port); }

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
