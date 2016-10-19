#pragma once

#include <cstdint>
#include <unordered_set>
#include <chrono>
#include <utility>
#include <boost/variant/variant.hpp>

#include "types/exception.hh"

namespace runos {

struct decision_conflict : virtual logic_error { };

class Decision {
public:
    typedef std::chrono::duration<uint32_t> duration;

    struct Base {
        bool return_ { false };
        duration idle_timeout { duration::max() };
        duration hard_timeout { duration::max() };
        Base() noexcept = default;
    };

    struct Undefined : Base { };

    struct Drop : Base {
    protected:
        friend class Decision;

        explicit Drop(Base base)
            : Base(base)
        { }
    };

    struct Unicast : Base {
        uint32_t port;
    protected:
        friend class Decision;

        explicit Unicast(Base base, uint32_t port)
            : Base(base), port(port)
        { }
    };

    struct Multicast : Base {
        typedef std::unordered_set<uint32_t> PortSet;
        PortSet ports;

    protected:
        friend class Decision;

        explicit Multicast(Base base, PortSet ports)
            : Base(base), ports(std::move(ports))
        { }
    };

    struct Broadcast : Base {
    protected:
        friend class Decision;

        explicit Broadcast(Base base)
            : Base(base)
        { }
    };

    struct Inspect : Base {
        uint16_t send_bytes_len;
    protected:
        friend class Decision;

        explicit Inspect(Base base, uint16_t send_bytes_len)
            : Base(base), send_bytes_len(send_bytes_len)
        { }
    };

    using DecisionData =
        boost::variant<
            Undefined,
            Drop,
            Unicast,
            Multicast,
            Broadcast,
            Inspect
        >;

    const DecisionData& data() const
    { return m_data; }

    Decision return_() const;

    // defaults to infinity
    duration idle_timeout() const;
    Decision idle_timeout(duration seconds) const;

    duration hard_timeout() const;
    Decision hard_timeout(duration seconds) const;

    // overwrites: unicast, broadcast
    Decision unicast(uint32_t port) const;

    // overwrites: multicast
    Decision multicast(Multicast::PortSet ports) const;
    //
    Decision broadcast() const;
    //
    Decision inspect(uint16_t bytes /* = MAX */) const;
    //
    Decision drop() const;
    //
    Decision clear() const;

protected:
    Decision() = default;

    Decision(DecisionData data)
        : m_data(std::move(data))
    { }

    Base& base();
    const Base& base() const;

    DecisionData m_data { Undefined{} };
};

} // namespace runos
