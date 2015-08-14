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
#include "OFMessageHandler.hh"

struct Switch_Point
{
    uint64_t dpid;
    uint32_t port;
};

class ArpHandler : public Application, public OFMessageHandlerFactory {
    SIMPLE_APPLICATION(ArpHandler, "arp-handler")
    public:
        std::string orderingName() const override { return "arp-handler"; }
        bool isPostreq(const std::string &name) const override { return (name == "link-discovery"); }
        std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override { return std::unique_ptr<OFMessageHandler>(new Handler(this)); }
        void init(Loader* loader, const Config& config) override;
    private:
        class HostManager* host_manager;

        class Handler: public OFMessageHandler {
            private:
                ArpHandler *app;
            public:
                Handler(ArpHandler* a){app = a; }
                Action processMiss(OFConnection* ofconn, Flow* flow) override;
         };
};
