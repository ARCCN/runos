#pragma once

#include "api/Packet.hh"
#include "oxm/field.hh"

namespace runos {

class TraceablePacket {
public:
    // get field without tracing
    virtual oxm::field<> watch(oxm::mask<> mask) const = 0;
};

class TraceableProxy final : public TraceablePacket,
                             public PacketProxy
{
public:
    oxm::field<> watch(oxm::mask<> mask) const override
    {
        return tpkt ? tpkt->watch(mask) : pkt.load(mask);
    }
    
    template<class Type>
    oxm::value<Type> watch(Type type) const
    {
        auto field = watch(oxm::mask<Type>{type});
        BOOST_ASSERT(field.exact());
        return oxm::value<Type>(field);
    }

    template<class Type>
    oxm::field<Type> watch(oxm::mask<Type> mask) const
    {
        auto generic_mask = static_cast<oxm::mask<>>(mask);
        auto generic_ret = watch(generic_mask);
        return static_cast<oxm::field<Type>>(generic_ret);
    }

    explicit TraceableProxy(Packet& pkt) noexcept
        : PacketProxy(pkt)
        , tpkt(packet_cast<TraceablePacket*>(pkt))
    { }

protected:
    TraceablePacket* const tpkt;
};

// guaranteed downcast
template<class T>
typename std::enable_if<std::is_same<T, TraceablePacket>::value, TraceableProxy>::type
packet_cast(Packet& pkt)
{
    return TraceableProxy(pkt);
}

} // namespace runos
