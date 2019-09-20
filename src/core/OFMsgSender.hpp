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
#include "SwitchOrdering.hpp"
#include "api/Switch.hpp"

#include <fluid/of13msg.hh>
#include <fluid/ofcommon/msg.hh>

#include <map>
#include <memory>

namespace runos {

namespace of13 = fluid_msg::of13;

using message = fluid_msg::OFMsg;
using msg_status_ptr = std::shared_ptr<struct MsgStatus>;

class OFMsgSender : public Application
                  , public SwitchEventHandler
{
    Q_OBJECT
    SIMPLE_APPLICATION(OFMsgSender, "ofmsg-sender");
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader *loader) override;
    
    void send(uint64_t dpid, message& msg);
    void send(uint64_t dpid, message&& msg);

protected slots:
    void polling();

private:
    void switchUp(SwitchPtr sw) override;
    void switchDown(SwitchPtr sw) override;

    void send_impl(uint64_t dpid, message& msg);

    class Poller* poller;
    class FlowEntriesVerifier* verifier;
    std::map<uint64_t, msg_status_ptr> status_map;
    uint16_t poll_interval;
    uint16_t wait_interval;
};

} // namespace runos
