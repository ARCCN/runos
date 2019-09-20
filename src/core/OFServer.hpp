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

#include <runos/core/future-decl.hpp>
#include "api/OFConnection.hpp"
#include "api/OFAgentFwd.hpp"

#include <memory>
#include <chrono>

namespace runos {

class OFServer : public Application
{
    Q_OBJECT
    SIMPLE_APPLICATION(OFServer, "of-server")
public:
    OFServer();
    ~OFServer();

    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

    future<OFConnectionPtr> connection(uint64_t dpid) const;
    future<OFAgentPtr> agent(uint64_t dpid) const;
    std::vector<OFConnectionPtr> connections() const;

    std::chrono::system_clock::time_point get_start_time() const;
    uint64_t get_rx_openflow_packets() const;
    uint64_t get_tx_openflow_packets() const;
    uint64_t get_pkt_in_openflow_packets() const;

signals:
    void switchDiscovered(OFConnectionPtr conn);
    void connectionUp(OFConnectionPtr conn);
    void connectionDown(OFConnectionPtr conn);

private:
    struct implementation;
    std::unique_ptr<implementation> impl;
};

} // namespace runos
