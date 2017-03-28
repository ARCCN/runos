#include "TraceablePacketImpl.hh"

#include <tuple>

#include <tuple>

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

std::pair<oxm::field<>, oxm::field<>>
TraceablePacketImpl::vload(oxm::mask<> by, oxm::mask<> what) const
{
    // TODO : make virtual load cache

    // read by field
    oxm::field<> by_read = pkt.load(oxm::mask<>(by));
    // read explored before bits
    oxm::field<> by_explored = cache.load(oxm::mask<>(by));
    // find requested bits that hasn't explored yet
    oxm::field<> by_unexplored = by_read & ~oxm::mask<>(by_explored);

    if (by_unexplored.wildcard()) {
        return { by_read, load(what) };
    }

    // read what field
    oxm::field<> what_read = pkt.load(oxm::mask<>(what));
    // read explored before bits
    oxm::field<> what_explored = cache.load(oxm::mask<>(what));
    // find requested bits that hasn't explored yet
    oxm::field<> what_unexplored = what_read & ~oxm::mask<>(what_explored);

    if (what_unexplored.wildcard()) {
        return { load(by), what_read };
    }

    tracer.vload( by_unexplored, what_unexplored );

    return { by_read, what_read };
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
