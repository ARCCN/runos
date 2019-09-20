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

#include "OFMsgSender.hpp"

#include "SwitchManager.hpp"
#include "FlowEntriesVerifier.hpp"
#include "api/OFAgent.hpp"
#include "lib/poller.hpp"

#include <runos/core/logging.hpp>
#include <runos/core/future.hpp>

#include <boost/chrono.hpp>
#include <mutex>
#include <queue>
#include <utility>

namespace runos {

REGISTER_APPLICATION(OFMsgSender, {"switch-ordering", "flow-entries-verifier", ""})

static constexpr int min_rate = 20; 

namespace barrier_status {
    template<typename R>
    bool received(boost::future<R> const& f)
    {
        return f.wait_for(boost::chrono::seconds(0)) == boost::future_status::ready;
    }
}

struct MsgStatus {
    MsgStatus(OFConnectionPtr conn, uint32_t limit, uint32_t add = 5, uint32_t mult = 2)
        : conn(conn)
        , limit(limit)
        , sent(0)
        , additive_ratio(add)
        , multiplicative_ratio(mult)
    {}

    OFConnectionPtr conn;
    uint32_t limit; // limit for sending msgs per pack
    uint32_t sent;  // amount sent msgs without barrier
    std::queue<std::pair<uint8_t*, size_t>> msgs;
    boost::future<void> barrier;
    boost::chrono::time_point<boost::chrono::steady_clock> updated;
    std::mutex mut;

    void send_barrier();
    void send_pack();

    // AIMD logic for OFMsg congestion control
    uint32_t additive_ratio;
    uint32_t multiplicative_ratio;
    void add_increase();
    void mult_decrease();
};

void MsgStatus::send_barrier()
{
    try {
        barrier = std::move(conn->agent()->barrier());
    } catch (const OFAgent::request_error& e) {
        LOG(ERROR) << "[MsgStatus] - " << e.what();
    }
    updated = boost::chrono::steady_clock::now();
}

void MsgStatus::send_pack()
{
    uint32_t sent_in_pack = 0;

    std::lock_guard lock(mut);
    while (!msgs.empty() && sent_in_pack < limit && sent < limit) {
        auto elem = msgs.front();
        conn->send(elem.first, elem.second);
        fluid_msg::OFMsg::free_buffer(elem.first);
        msgs.pop();
        sent_in_pack++;
        sent++;

        if (sent_in_pack == limit || sent == limit) { // reached limit
            send_barrier();
            sent = 0;
            break;
        }
    }
}

void MsgStatus::add_increase()
{
    limit += additive_ratio;

    VLOG(4) << "Switch " << conn->dpid() << " sent pack successfully";
    VLOG(4) << "Increase limit on " << additive_ratio << " messages. "
            << "Now: " << limit << " messages";
}

void MsgStatus::mult_decrease()
{
    LOG(WARNING) << "Switch " << conn->dpid() << " don't reply on barrier";

    if (limit >= multiplicative_ratio * min_rate) {
        limit /= multiplicative_ratio;
        LOG(WARNING) << "Changed ofmsg limit for switch " << conn->dpid() 
                     << " to " << limit << " messages";
    } else {
        limit = min_rate;
        LOG(WARNING) << "Ofmsg limit for switch " << conn->dpid()
                     << " is min = " << min_rate << " messages";
    }
}

// ** Application ** //

void OFMsgSender::init(Loader* loader, const Config& rootConfig)
{
    verifier = FlowEntriesVerifier::get(loader);

    auto config = config_cd(rootConfig, "ofmsg-sender");
    poll_interval = config_get(config, "poll-interval", 50);    // ms
    wait_interval = config_get(config, "wait-interval", 5000);  // ms

    poller = new Poller(this, poll_interval);
    SwitchOrderingManager::get(loader)->registerHandler(this, 16);
}

void OFMsgSender::startUp(Loader* loader)
{
    poller->run();
}

void OFMsgSender::send(uint64_t dpid, message& msg)
{
    send_impl(dpid, msg);
}

void OFMsgSender::send(uint64_t dpid, message&& msg)
{
    send_impl(dpid, msg);
}

void OFMsgSender::polling()
{
    auto now = boost::chrono::steady_clock::now();
    for (auto& it : status_map) {
        auto status_ptr = it.second;
        {
            std::lock_guard lock(status_ptr->mut);
            if (status_ptr->msgs.empty()) {
                continue;       // no messages for switch
            }
        }

        if (status_ptr->barrier.valid()) {                        // we sent barrier (state is valid)
            if (barrier_status::received(status_ptr->barrier)) {    // and received barrier-reply
                status_ptr->barrier.get();                            // make state invalid
                status_ptr->add_increase();                           // and increase limit
            } else {                                                // or...
                if (now - status_ptr->updated > boost::chrono::milliseconds(wait_interval)) {
                    status_ptr->mult_decrease();                      // send new barrier if time was over
                    status_ptr->send_barrier();
                }
                continue;                                           // wait barrier-reply
            }
        }

        poller->apply([=](){ status_ptr->send_pack(); });
    }
}

void OFMsgSender::switchUp(SwitchPtr sw)
{
    auto prop = sw->property("ofmsg_limit");
    if (not prop.has_value()) return; // No limits

    auto limit = std::any_cast<int64_t>(prop);
    if (limit > 0) {
        auto additive = sw->property("ofmsg_add_ratio", 5);
        auto multiplicative = sw->property("ofmsg_mult_ratio", 2);
        status_map.emplace(sw->dpid(), 
                std::make_shared<MsgStatus>(sw->connection(),
                                            static_cast<uint32_t>(limit),
                                            additive, multiplicative)
        );
    }
}

void OFMsgSender::switchDown(SwitchPtr sw)
{
    status_map.erase(sw->dpid());
}

void OFMsgSender::send_impl(uint64_t dpid, message& msg)
{
    auto status_iter = status_map.find(dpid);
    if (status_iter == status_map.end()) { // no limits
        verifier->send(dpid, msg);
        return;
    }

    try {
        std::lock_guard lock(status_iter->second->mut);
        status_iter->second->msgs.emplace(msg.pack(), msg.length());
    } catch (const invalid_argument& e) {
        LOG(WARNING) << e.what();
    }
}

} // namespace runos
