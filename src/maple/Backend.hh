#pragma once

#include "Flow.hh"
#include "oxm/field_set_fwd.hh"

namespace runos {
namespace maple {

struct Backend {
    virtual void install(unsigned priority,
                         oxm::field_set const& match,
                         FlowPtr flow) = 0;

    virtual void remove(FlowPtr flow) = 0;
    virtual void remove(unsigned priority,
                        oxm::field_set const& match) = 0;
    virtual void remove(oxm::field_set const& match) = 0;
    virtual void barrier() { }
};

} // namespace maple
} // namespace runos
