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
#include "Common.hh"

#include <unordered_map>

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "FluidUtils.hh"

class SimpleLearningSwitch : public Application, OFMessageHandlerFactory {
SIMPLE_APPLICATION(SimpleLearningSwitch, "simple-learning-switch")
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override;
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override;

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        // MAC -> port mapping for EVERY switch
        std::unordered_map<EthAddress, uint32_t> seen_port;
    };
};
