#pragma once

#include "oxm/field.hh"
#include "oxm/field_set.hh"

#include <vector>
#include <tuple>
#include <memory>

namespace runos {
namespace maple {

class Flow {
public:
    virtual const Flow& operator=(const Flow&) {return *this; }
    virtual ~Flow(){ }
    virtual std::vector< std::pair<oxm::field<>,
                                   oxm::field<>>>
    virtual_fields(oxm::mask<> by, oxm::mask<> what) const = 0;
};

typedef std::shared_ptr<Flow> FlowPtr;

}
}
