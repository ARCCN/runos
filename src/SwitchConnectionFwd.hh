#pragma once

#include <memory>

namespace runos {
    class SwitchConnection;
    typedef std::shared_ptr<SwitchConnection> SwitchConnectionPtr;
    typedef std::weak_ptr<SwitchConnection> SwitchConnectionWeakPtr;
}
