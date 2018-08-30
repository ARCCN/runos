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

/** @file */
#pragma once

#include "server_http.hpp"

namespace web {

using ServerBase = SimpleWeb::ServerBase<SimpleWeb::HTTP>;

class Server: public ServerBase {
// Becouse SimpleWeb::Server<HTTP> doesn't allow specify listening port
// I had to copy the class and change constructor
public:
    Server(uint16_t port) noexcept : ServerBase::ServerBase(port)  {}

protected:
    void accept() override {
        auto connection = create_connection(*io_service);

        acceptor->async_accept(*connection->socket,
          [this, connection](const SimpleWeb::error_code &ec) {
            auto lock = connection->handler_runner->continue_lock();
            if(!lock)
                return;

            // Immediately start accepting a new connection (unless io_service has been stopped)
            if(ec != boost::asio::error::operation_aborted)
                this->accept();

            auto session = std::make_shared<Session>(
                    config.max_request_streambuf_size,
                    connection
                );

            if(!ec) {
                boost::asio::ip::tcp::no_delay option(true);
                SimpleWeb::error_code ec;
                session->connection->socket->set_option(option, ec);

                this->read(session);
            } else if(this->on_error)
                this->on_error(session->request, ec);
        });
    }
};
} // namespace web
