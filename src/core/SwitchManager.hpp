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


#include "api/Switch.hpp"
#include "Application.hpp"
#include "Controller.hpp"

#include <runos/core/safe_ptr.hpp>

#include <QtCore>

#include <memory>
#include <cstdint>

namespace runos {

class SwitchManager: public Application {
Q_OBJECT
SIMPLE_APPLICATION(SwitchManager, "switch-manager")
public:
    SwitchManager();
    ~SwitchManager();

    void init(Loader* provider, const Config& config) override;

    safe::shared_ptr<Switch> switch_(uint64_t dpid) /* noexcept */ const;
    std::vector<SwitchPtr> switches() const;

signals:
    void portAdded(PortPtr);
    void portDeleted(PortPtr);
    void linkUp(PortPtr);
    void linkDown(PortPtr);
    void switchUp(SwitchPtr);
    void switchDown(SwitchPtr);

    void portMaintenanceStart(PortPtr);
    void portMaintenanceEnd(PortPtr);
    void switchMaintenanceStart(SwitchPtr);
    void switchMaintenanceEnd(SwitchPtr);

private:
    struct implementation;
    std::shared_ptr<implementation> impl;
};

} // namespace runos
