/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include <string>

#include "Application.hh"
#include "Common.hh"
#include "Loader.hh"
#include "oxm/openflow_basic.hh"

struct Switch_Point
{
    uint64_t dpid;
    uint32_t port;
};

/**
 * Handler arp packets in network.
 *
 * Application generate arp reply on arp-request packets and send it to ingress port.
 */
class ArpHandler : public Application {
    SIMPLE_APPLICATION(ArpHandler, "arp-handler")
    public:
        void init(Loader* loader, const Config& config) override;
    private:
        class HostManager* host_manager;

};
