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

#include "Controller.hpp"

#include "OFServer.hpp"
#include "OFMessage.hpp"
#include "lib/qt_executor.hpp"

#include <runos/core/logging.hpp>
#include <runos/core/future.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <map>

namespace runos {

REGISTER_APPLICATION(Controller, {"of-server", ""})

struct ReceiveHandler;
using ReceiveHandlerPtr = std::shared_ptr<ReceiveHandler>;

struct Controller::implementation {
    OFServer* of_server;
    qt_executor executor;
    boost::inline_executor inline_executor;

    explicit implementation(QObject* parent)
        : executor(parent)
    { }

    std::multimap<int, OFMessageHandlerWeakPtr> handlers;
    std::map<uint64_t, ReceiveHandlerPtr> recv_handler;
};

Controller::Controller()
    : impl(new implementation(this))
{ }

Controller::~Controller() = default;

void Controller::init(Loader* loader, const Config& /*rootConfig*/)
{
    //const Config& config = config_cd(rootConfig, "controller");
    impl->of_server = OFServer::get(loader);
    QObject::connect(impl->of_server, &OFServer::switchDiscovered,
                     this, &Controller::onSwitchDiscovered,
                     Qt::DirectConnection);
}

struct ReceiveHandler
    : OFConnection::ReceiveHandler<fluid_msg::OFMsg>
{
    Controller* self;
    OFConnectionPtr conn;

    ReceiveHandler(Controller* self, OFConnectionPtr conn)
        : self(self), conn(conn)
    { }

    void process(fluid_msg::OFMsg& msg) {
        if (not self->dispatch(msg, conn)) {
            //LOG(WARNING) << "OpenFlow message of type " << (unsigned) msg.type()
            //    << " from switch dpid="
            //    << (conn ? std::to_string(conn->dpid()) : "(unknown)")
            //    << " hasn't been dispatched";
        }
    }
};

void Controller::onSwitchDiscovered(OFConnectionPtr conn)
{
    auto dpid = conn->dpid();
    auto recv_handler = std::make_shared<ReceiveHandler>(this, conn);
    conn->receive(recv_handler);

    {
        static boost::mutex m;
        boost::lock_guard<boost::mutex> lock(m);

        impl->recv_handler[dpid] = recv_handler;
    }
}

void Controller::register_handler(OFMessageHandlerPtr handler, int priority)
{
    impl->handlers.emplace(priority, handler);
}

bool Controller::dispatch(fluid_msg::OFMsg& msg, OFConnectionPtr conn)
{
    bool dispatched = false;

    for (auto& map_pair : impl->handlers) {
        if (auto handler = map_pair.second.lock()) {
            auto dispatchable
                = make_dispatchable<LinearDispatch>(msg);
            if (auto do_break = dispatchable->dispatch(*handler, conn)) {
                dispatched  = true;
                if (*do_break) break;
            }
        }
    }

    return dispatched;
}

} // namespace runos
