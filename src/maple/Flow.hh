#pragma once

#include <memory>

namespace runos {
namespace maple {

class Flow {
public:
    virtual Flow& operator=(const Flow&) { return *this; }
};

typedef std::shared_ptr<Flow> FlowPtr;

}
}
