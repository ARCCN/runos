#include "Decision.hh"

#include <algorithm>
#include <boost/variant/get.hpp>
#include <boost/variant/polymorphic_get.hpp>

namespace runos {

Decision::Base& Decision::base()
{ return boost::polymorphic_get<Base>(m_data); }

const Decision::Base& Decision::base() const
{ return boost::polymorphic_get<Base>(m_data); }

Decision Decision::return_() const
{
    Decision ret{*this};
    ret.base().return_ = true;
    return ret;
}

Decision::duration Decision::idle_timeout() const
{
    return base().idle_timeout;
}

Decision Decision::idle_timeout(duration seconds) const
{
    Decision ret{*this};
    ret.base().idle_timeout = std::min(seconds, ret.idle_timeout());
    return ret;
}

Decision::duration Decision::hard_timeout() const
{
    return base().hard_timeout;
}

Decision Decision::hard_timeout(duration seconds) const
{
    Decision ret{*this};
    ret.base().hard_timeout = std::min(seconds, ret.hard_timeout());
    ret.base().idle_timeout = std::min(seconds, ret.idle_timeout());
    return ret;
}

Decision Decision::unicast(uint32_t port) const
{
    if (boost::get<Undefined>(&m_data) == nullptr) {
        const Unicast* u = boost::get<Unicast>(&m_data);
        if (not u || u->port != port) {
            RUNOS_THROW(decision_conflict());
        }
    }
    return Decision{Unicast{base(), port}};
}

Decision Decision::multicast(Multicast::PortSet ps) const
{
    if (boost::get<Undefined>(&m_data) == nullptr) {
        const Multicast* m = boost::get<Multicast>(&m_data);
        if (not m || m->ports != ps) {
            RUNOS_THROW(decision_conflict());
        }
    }
    return Decision{Multicast{base(), std::move(ps) }};
}

Decision Decision::broadcast() const
{
    if (boost::get<Undefined>(&m_data) == nullptr) {
        if (not boost::get<Broadcast>(&m_data))
            RUNOS_THROW(decision_conflict());
    }
    return Decision{Broadcast{base()}};
}

Decision Decision::inspect(uint16_t bytes) const
{
    if (boost::get<Undefined>(&m_data) == nullptr) {
        if (not boost::get<Inspect>(&m_data))
            RUNOS_THROW(decision_conflict());
    }

    const Inspect* i = boost::get<Inspect>(&m_data);
    if (i) {
        return Decision{Inspect{base(), std::max(bytes, i->send_bytes_len)}};
    } else {
        return Decision{Inspect{base(), bytes}};
    }
}

Decision Decision::drop() const
{
    return Decision{Drop{base()}};
}

Decision Decision::clear() const
{
    return Decision{Undefined{}};
}

} // namespace runos
