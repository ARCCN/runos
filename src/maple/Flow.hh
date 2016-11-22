#pragma once

#include <memory>

namespace runos {
namespace maple {

class Flow {
public:
    virtual ~Flow(){ }
};

typedef std::shared_ptr<Flow> FlowPtr;

}
}
