#pragma once

#include <cstdint>
#include <unordered_set>
#include <chrono>
#include <utility>
#include <boost/variant/variant.hpp>

#include "types/exception.hh"
#include "Common.hh"
#include "Flow.hh"
#include "api/Packet.hh"

namespace runos {

struct decision_conflict : virtual logic_error { };

class Decision {
public:
    typedef std::chrono::duration<uint32_t> duration;

    struct CustomDecision {
        virtual void apply(ActionList& ret, uint64_t dpid) = 0;
    };

    typedef std::shared_ptr<CustomDecision> CustomDecisionPtr;

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

    //Handler return true, if is not needed continue handle
    struct Inspect : Base {
        uint16_t send_bytes_len;
        typedef std::function<bool(Packet&, FlowPtr)> Handler;
        Handler handler;
    protected:
        friend class Decision;

        explicit Inspect(Base base,
                uint16_t send_bytes_len,
                Handler handler)
            : Base(base), send_bytes_len(send_bytes_len), handler(handler)
        { }
    };

    struct Custom : Base {
        CustomDecisionPtr body;
    protected:
        friend class Decision;
        explicit Custom(Base base, CustomDecisionPtr body)
            : Base(base), body(body)
        { }
    };


    using DecisionData =
        boost::variant<
            Undefined,
            Drop,
            Unicast,
            Multicast,
            Broadcast,
            Inspect,
            Custom
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
    Decision inspect(uint16_t bytes /* = MAX */, Inspect::Handler h) const;
    //
    Decision drop() const;
    //
    Decision clear() const;
    //
    Decision custom(CustomDecisionPtr body) const;

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
