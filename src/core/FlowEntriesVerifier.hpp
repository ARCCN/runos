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
#include "lib/poller.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of13msg.hh>
#include <json.hpp>

#include <boost/thread/shared_mutex.hpp>

#include <memory>
#include <map>
#include <unordered_map>

namespace runos {

namespace of13 = fluid_msg::of13;

using json = nlohmann::json;
using FlowModPtr = std::unique_ptr<of13::FlowMod>;

class VerifierDatabase {
public:
    using SwitchStatePtr = std::shared_ptr<class SwitchState>;
    using SwitchStatePtrMap = std::unordered_map<uint64_t, SwitchStatePtr>;

    using dump_json = std::map<std::string, json>;

    template<class... Args>
    void addState(uint64_t dpid, Args&&... args)
    {
        auto new_state_ptr =
            std::make_shared<SwitchState>(std::forward<Args>(args)...);

        add_state_impl(dpid, new_state_ptr);
    }

    void removeState(uint64_t dpid);
    void clear();

    void process(uint64_t dpid, of13::FlowMod& fm);
    FlowModPtr process(uint64_t dpid, of13::FlowRemoved& fr);

    void fromJson(dump_json& db_dump);
    dump_json toJson() const;

    void restoreStates(const class MessageSender* sender) const;

private:
    SwitchStatePtrMap states_;
    mutable boost::shared_mutex states_mut_;

    void add_state_impl(uint64_t dpid, SwitchStatePtr& new_state_ptr);
    SwitchStatePtr find_state(uint64_t dpid) const;
};

class FlowEntriesVerifier : public Application
                          , public SwitchEventHandler
{
    Q_OBJECT
    SIMPLE_APPLICATION(FlowEntriesVerifier, "flow-entries-verifier");
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader *loader) override;

    void send(uint64_t dpid, fluid_msg::OFMsg& msg);

protected slots:
    void polling();

private:
    struct implementation;

    std::shared_ptr<implementation> impl_;
    VerifierDatabase data_;
    std::unique_ptr<class Poller> poller_;
    bool is_active_;

    void switchUp(SwitchPtr sw) override;
    void switchDown(SwitchPtr sw) override;
};

} // namespace runos
