#pragma once

#include <memory>

namespace runos {

class Flow;
typedef std::shared_ptr<Flow> FlowPtr;
typedef std::weak_ptr<Flow> FlowWeakPtr;

}
