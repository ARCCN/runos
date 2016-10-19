#include "TraceablePacketImpl.hh"

namespace runos {
namespace maple {

oxm::field<> TraceablePacketImpl::load(oxm::mask<> mask) const
{
    // read requested bits from packet
    oxm::field<> read = pkt.load(mask);
    // read explored before bits
    oxm::field<> explored = cache.load(mask);
    // find requested bits that hasn't explored yet
    oxm::field<> unexplored = read & ~oxm::mask<>(explored);

    // augment trace tree with unexplored data
    if (not unexplored.wildcard()) {
        tracer.load(unexplored);
        // mark them as explored (and copy its data)
        cache.modify(unexplored);
    }
    
    return read;
}

bool TraceablePacketImpl::test(oxm::field<> need) const
{
    // read requested bits from packet
    oxm::field<> read = pkt.load(oxm::mask<>(need));
    // read explored before bits
    oxm::field<> explored = cache.load(oxm::mask<>(need));
    // find requested bits that hasn't explored yet
    oxm::field<> unexplored = read & ~oxm::mask<>(explored);

    // check if result is completely determined by the previous calls
    if (not (explored & need)) {
        return false;
    }
    // no more data
    if (unexplored.wildcard()) {
        return true;
    }

    bool ret = unexplored & need;
    tracer.test(need & oxm::mask<>(unexplored), ret);

    // Assert that we know bits read only if they matched
    if (ret) {
        cache.modify(unexplored);
    }
    return ret;
}

void TraceablePacketImpl::modify(oxm::field<> patch)
{
    // May throws without touching cache
    pkt.modify(patch);
    // Shouldn't throw
    cache.modify(patch);
}

} // namespace maple
} // namespace runos
