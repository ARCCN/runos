#pragma once

#include <sstream>

#include <boost/exception/exception.hpp>

#include "TraceablePacketImpl.hh"
#include "TraceTree.hh"
#include "Flow.hh"
#include "LoggableTracer.hh"

namespace runos {
namespace maple {

typedef boost::error_info< struct tag_trace, std::string >
    errinfo_trace;

template<class Decision, class Flow>
class Runtime {
    using FlowPtr = std::shared_ptr<Flow>;
    using Policy = std::function<Decision(Packet& pkt, FlowPtr flow)>;

    Backend& backend;
    std::unique_ptr<TraceTree> trace_tree;
    Policy policy;
    FlowPtr miss;

public:
    Runtime(Policy policy, Backend& backend, FlowPtr miss)
        : backend(backend)
        , trace_tree{new TraceTree{backend, miss}}
        , policy{policy}
        , miss{miss}
    { }

    FlowPtr augment(Packet& pkt, FlowPtr flow)
    {
        auto tracer = trace_tree->augment();
        LoggableTracer log_tracer {*tracer};
        TraceablePacketImpl tpkt{pkt, log_tracer};

        try {
            flow->decision(policy(tpkt, flow));
            if (not flow->disposable()) {
                tracer->finish(flow);
            } // Sometimes we don't need to create a new flow on the switch.
        } catch (boost::exception& e) {
            try {
                e << errinfo_trace(log_tracer.log());
            } catch (...) { }
            throw;
        }

        return flow;
    }

    FlowPtr operator()(Packet& pkt)
    {
        auto flow = trace_tree->lookup(pkt);
        return flow ? std::dynamic_pointer_cast<Flow>(flow) : miss;
    }

    void commit()
    {
        trace_tree->commit();
    }

    void invalidate()
    {
        trace_tree.reset(new TraceTree{backend, miss});
    }
};

}
}
