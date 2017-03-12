#pragma once

#include <functional>

#include "types/exception.hh"
#include "oxm/field_fwd.hh"
#include "Flow.hh"

namespace runos {
namespace maple {

typedef std::function<void()> Installer;

struct inconsistent_trace : runtime_error { };

struct Tracer {
    virtual void load(oxm::field<> unexplored) = 0;
    virtual void test(oxm::field<> pred, bool ret) = 0;
    virtual Installer finish(FlowPtr flow) = 0;
    virtual ~Tracer() = default;
};

} // namespace maple
} // namespace runos
