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
#include <vector>
#include <unordered_map>

#include "Common.hh"
#include "Loader.hh"
#include "Application.hh"
#include "Rest.hh"
#include "Switch.hh"
#include "json11.hpp"
#include "OFTransaction.hh"
#include "SwitchConnection.hh"

#include "oxm/field_set.hh"


/**
 * That module allows REST users to get switch statistics that can be delivered by MultipleRequest messages.
 * None of Modify actions are implemented in RestFlowMod and StaticFlowPusher modules.
 *
 * Handling of each GET consists of the following steps:
 *  - switching in handleGET method
 *       - calling ``sendGetRequest(corresponding type, <params>)`` and turning in event loop
 *       - waiting for the response from a switch and it's handling
 *           - handler (`onResponse`) updated variable that corresponds to the reply type(of13::MultipartReplyFlow replyFlow, for example)
 *           - handler wakes up `handleGET` method
 *       - responding to the user
 */
class RestMultipart : public Application, RestHandler {
Q_OBJECT
SIMPLE_APPLICATION(RestMultipart, "rest-multipart")
public:
    void init(Loader* loader, const Config& rootConfig) override;

    // rest
    bool eventable() override {return false;}
    AppType type() override { return AppType::None; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;
    json11::Json handlePOST(std::vector<std::string> params, std::string body) override;
protected slots:
    /// called on switch's response
    void onResponse(SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> reply);
signals:
    // emitted when response handling (in `onResponse`) is finished
    void ResponseHandlingFinished();
private:
    class Controller *ctrl_;
    class SwitchManager *sw_m_;
    OFTransaction *transaction_;

    // a set of sendRequest methods -- per one for each supported rest request
    void sendGetRequest(of13::MultipartRequestFlow &&req, uint64_t dpid);
    void sendGetRequest(of13::MultipartRequestPortStats &&req,
                        uint64_t dpid,
                        std::string port_number);
    void sendGetRequest(of13::MultipartRequestDesc &&req, uint64_t dpid);
    void sendGetRequest(of13::MultipartRequestAggregate &&req, uint64_t dpid);
    void sendGetRequest(of13::MultipartRequestTable &&req, uint64_t dpid);
    void sendGetRequest(of13::MultipartRequestPortDescription &&req, uint64_t dpid);
    void sendGetRequest(of13::MultipartRequestQueue &&req,
                        uint64_t dpid,
                        std::string port_number,
                        std::string queue_id);

    void sendPostRequest(of13::MultipartRequestFlow &&mpReq,
                         uint64_t dpid,
                         json11::Json::object req);
    void sendPostRequest(of13::MultipartRequestAggregate &&mpReq,
                         uint64_t dpid,
                         json11::Json::object req);

    void processInfo(of13::MultipartRequestFlow &mpReq,
                     const json11::Json::object &req);
    void processMatches(of13::MultipartRequestFlow &mpReq,
                        const json11::Json::object &matches);

    void processInfo(of13::MultipartRequestAggregate &mpReq,
                     const json11::Json::object &req);
    void processMatches(of13::MultipartRequestAggregate &mpReq,
                        const json11::Json::object &matches);

    // a set of variables for switch's answer storing -- per one for each supported rest requests type
    std::vector<of13::FlowStats> responseFlow_;
    std::vector<of13::PortStats> responsePort_;
    fluid_msg::SwitchDesc responseSwitchDesc_;
    of13::MultipartReplyAggregate responseAggregate_;
    std::vector<of13::TableStats> responseTable_;
    std::vector<of13::Port> responsePortDesc_;
    std::vector<of13::QueueStats> responseQueue_;
};


