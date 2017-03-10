#pragma once

#include "oxm/field_set.hh"

#include <memory>

namespace runos {
namespace maple {

class Flow {
protected:
    oxm::field_set match;
    uint16_t m_priority;
public:
    virtual const Flow& operator=(const Flow&) {return *this; }
    virtual ~Flow(){ }
};

typedef std::shared_ptr<Flow> FlowPtr;

}
}
