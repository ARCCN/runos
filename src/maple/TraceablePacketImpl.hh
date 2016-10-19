#pragma once

#include "api/Packet.hh"
#include "api/TraceablePacket.hh"
#include "oxm/field_set.hh"
#include "Tracer.hh"

namespace runos {
namespace maple {

// Provides trace tree compression
class TraceablePacketImpl final : public TraceablePacket
                                , public PacketProxy
{
    Tracer& tracer;
    mutable oxm::field_set cache; // traces + modifications

public:
    TraceablePacketImpl(Packet& pkt, Tracer& tracer)
        : PacketProxy(pkt), tracer(tracer)
    { }

    ~TraceablePacketImpl() = default;

    oxm::field<> load(oxm::mask<> mask) const override;
    bool test(oxm::field<> need) const override;
    void modify(oxm::field<> patch) override;

    oxm::field<> watch(oxm::mask<> mask) const override
    { return pkt.load(mask); }
};

} // namespace maple
} // namespace runos
