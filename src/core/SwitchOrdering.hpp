/*
 * Copyright 2019 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Application.hpp"
#include "Loader.hpp"
#include "api/Switch.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

class QTimer;

namespace runos {

enum EventType {
    AT_SWITCH_UP    = (1 << 0),
    AT_SWITCH_DOWN  = (1 << 1),
    AT_LINK_UP      = (1 << 2),
    AT_LINK_DOWN    = (1 << 3)
};

using HandlersMap = std::multimap<int, class SwitchEventHandler*>;
using Switches = std::vector<uint64_t>;
using Links = std::map<uint64_t, std::vector<PortPtr>>;

class SwitchEventHandler {
protected:
    virtual void switchUp(SwitchPtr sw) {};
    virtual void switchDown(SwitchPtr sw) {};
    virtual void linkUp(PortPtr sw) {};
    virtual void linkDown(PortPtr sw) {};
private:
    uint8_t types;

    friend class SwitchOrderingManager;
    friend class MainHandler;
};

class SwitchOrderingManager : public Application {
    SIMPLE_APPLICATION(SwitchOrderingManager, "switch-ordering")
    Q_OBJECT
public:
    void init(Loader* loader, const Config& rootConfig) override;
    void registerHandler(SwitchEventHandler* handler, int prio,
                         uint8_t types = 0xf);
    bool isConnected(uint64_t dpid) const;

protected slots:
    void onSwitchUp(SwitchPtr sw);
    void onSwitchDown(SwitchPtr sw);
    void onLinkUp(PortPtr port);
    void onLinkDown(PortPtr port);
private:
    void run_handling(SwitchPtr sw, EventType type);

    HandlersMap handlers_;
    Switches connected_;
    Links cached_;

    mutable std::mutex mut_;
    std::unique_ptr<class SwitchLocker> locker_;

    std::chrono::seconds echo_interval_;
    std::unordered_map<uint64_t, QTimer*> barrier_timers_;
};

} // namespace runos
