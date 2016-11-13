/*
 * Copyright 2016 Applied Research Center for Computer Networks
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

/** @file */

#pragma once

#include <string>
#include <unordered_map>

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFTransaction.hh"
#include "Switch.hh"
#include "Rest.hh"
#include "json11.hpp"


/**
 * REST interface for flow enties construction and deletion of existing ones.
 *
 * Allows to add new flow entries to a switch (to any already existing table excluding Maple's one): You can use table numbers in range of [0, ctrl->handler_table() - 1].
 * POST request:
 *  POST /api/rest-flowmod/flow/<switch_id>
 *  body of the request: JSON description of a new flow
 *
 * Imlemantation logic is the same as in RestMultipart.
 * If you want to add new handler, you need to go trougth the following steps:
 *  - add new path to be handeled: `init`: `acceptPath(Method::POST, "flowentry");`
 *  - implement your logic in a "case" of handlePOST (GET, DELETE, etc)
 */
class RestFlowMod : public Application, RestHandler
{
SIMPLE_APPLICATION(RestFlowMod, "rest-flowmod")
public:
    void init(Loader *loader, const Config& rootConfig) override;
    bool eventable() override { return false; }
    AppType type() override { return AppType::Service; }

    json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
    json11::Json handleDELETE(std::vector<std::string> params, std::string body) override;

    class Controller *ctrl_;
    class SwitchManager *sw_m_;
    /// switch table, reserved for RestFlowMod activity.
    /// note: you can set rules via REST not only in that table, but in any tables excluding Maple's one
    uint8_t tableNo_;
    /// used to form the response massage by different methods
    std::stringstream response_{};

    // Parts of handlePOST task. If problem detected throws std::string with possible reason.
    void processInfoAdd(of13::FlowMod &fm,
                        const json11::Json::object &req);
    void processInfoDelete(of13::FlowMod &fm,
                        const json11::Json::object &req);
    void processMatches(of13::FlowMod &fm, const json11::Json::object &matches);
    void processActions(of13::FlowMod &fm, const json11::Json::array &actions);
    void processAction(const json11::Json::object &action, of13::ApplyActions &applyActions);

    // Parts of handleDELETE task.
    /// removeAll: code from Controller::clearTable
    void clearTables(uint64_t dpid);
    /// set goToMaple: code from Controller::installTableMissRule
    void installTableMissRule(uint64_t dpid, uint8_t table_id);
    /// install in table `table` goto on `table + 1`
    void installGoto(uint64_t dpid, uint8_t table_id);

};

