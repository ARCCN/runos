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

#include <memory>
#include <cstdint>
#include <functional>
#include <chrono>

#include <fluid/ofcommon/msg.hh>

#include "DoubleDispatcher.hpp"
#include "OFAgentFwd.hpp"

namespace runos {

class OFConnection {
protected:
    using message = fluid_msg::OFMsg;

    struct receive_tag;
    struct send_hook_tag;

    using SendHookDispatch = DoubleDispatcher<
        send_hook_tag,
        message,
        void
    >;

    using ReceiveDispatch = DoubleDispatcher<
        receive_tag,
        message,
        void
    >;

public:

    template<class Message>
    using SendHookHandler = SendHookDispatch::Handler<Message>;
    using SendHookHandlerPtr = std::shared_ptr<SendHookDispatch::HandlerBase>;

    template<class Message>
    using ReceiveHandler = ReceiveDispatch::Handler<Message>;
    using ReceiveHandlerPtr = std::shared_ptr<ReceiveDispatch::HandlerBase>;

    virtual uint64_t dpid() const = 0;
    virtual bool alive() const = 0;
    virtual uint8_t protocol_version() const = 0;
    virtual std::string peer_address() const = 0;
    virtual OFAgentPtr agent() const = 0;

    virtual void set_start_time() = 0;
    virtual void reset_stats() = 0;
    virtual std::chrono::system_clock::time_point get_start_time() const = 0;
    virtual uint64_t get_rx_packets() const = 0;
    virtual uint64_t get_tx_packets() const = 0;
    virtual uint64_t get_pkt_in_packets() const = 0;
    virtual void packet_in_counter() = 0;

    virtual void send(message const& msg) = 0;
    virtual void send(void* msg, size_t size) = 0;
    virtual void close() = 0;

    virtual void send_hook(SendHookHandlerPtr handler) = 0;
    virtual void receive(ReceiveHandlerPtr handler) = 0;
};

typedef std::shared_ptr<OFConnection> OFConnectionPtr;
typedef std::weak_ptr<OFConnection> OFConnectionWeakPtr;

} // namespace runos
