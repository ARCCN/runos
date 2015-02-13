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

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"

class CBench : public Application, public OFMessageHandlerFactory {
    SIMPLE_APPLICATION(CBench, "cbench")
public:
    void init(Loader* loader, const Config& config) override;

    std::string orderingName() const override { return "cbench"; }
    OFMessageHandler* makeOFMessageHandler() override { return new Handler(); }
    // Hackish way to sure that it's the only processor registered
    bool isPrereq(const std::string& name) const override { return true; }
    bool isPostreq(const std::string& name) const override { return true; }

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };
};
