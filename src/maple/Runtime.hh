#pragma once

#include <sstream>
#include <functional>
#include <tuple>

#include <boost/exception/exception.hpp>

#include "TraceablePacketImpl.hh"
#include "TraceTree.hh"
#include "Flow.hh"
#include "LoggableTracer.hh"

namespace runos {
namespace maple {

typedef boost::error_info< struct tag_trace, std::string >
    errinfo_trace;

typedef std::function<void()>  Installer;

template<class Decision, class Flow>
class Runtime {
    using FlowPtr = std::shared_ptr<Flow>;
    using Policy = std::function<Decision(Packet& pkt, FlowPtr flow)>;

    Backend& backend;
    std::unique_ptr<TraceTree> trace_tree;
    Policy policy;

public:
    Runtime(Policy policy, Backend& backend)
        : backend(backend)
        , trace_tree{new TraceTree{backend}}
        , policy{policy}
    { }

    std::pair<FlowPtr, Installer> augment(Packet& pkt, FlowPtr flow)
    {
        auto tracer = trace_tree->augment();
        LoggableTracer log_tracer {*tracer};
        TraceablePacketImpl tpkt{pkt, log_tracer};
        Installer installer;

        try {
            flow->decision(policy(tpkt, flow));
            installer = tracer->finish(flow);
        } catch (boost::exception& e) {
            try {
                e << errinfo_trace(log_tracer.log());
            } catch (...) { }
            throw;
        }

        return {flow, installer};
    }

    FlowPtr operator()(Packet& pkt)
    {
        auto flow = trace_tree->lookup(pkt);
        return flow ? std::dynamic_pointer_cast<Flow>(flow) : nullptr;
    }

    void commit()
    {
        trace_tree->commit();
    }

    void update()
    {
        trace_tree->update();
    }

    void invalidate()
    {
        trace_tree.reset(new TraceTree{backend});
    }
};

}
}
