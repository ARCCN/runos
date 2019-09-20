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
 
#include <runos/core/logging.hpp>

#include "../OFServer.hpp"

extern "C"
#undef OFP_VERSION
#undef DESC_STR_LEN
#undef SERIAL_NUM_LEN
#undef OFP_MAX_TABLE_NAME_LEN
#undef OFP_MAX_PORT_NAME_LEN
#undef OFP_TCP_PORT
#undef OFP_SSL_PORT
#undef OFP_ETH_ALEN
#undef OFP_DEFAULT_MISS_SEND_LEN
#undef OFP_FLOW_PERMANENT
#undef OFP_DEFAULT_PRIORITY
#undef OFPQ_ALL
#undef OFPQ_MIN_RATE_UNCFG

{
#include "cpqd-print/ofl-messages.h"
}

namespace runos {

class OFLog : public Application
{
    Q_OBJECT
    SIMPLE_APPLICATION(OFLog, "oflog")
public:

    enum print_direction { print_in, print_out };

    static void print(fluid_msg::OFMsg& fluid_msg, print_direction d)
    {
        auto buf = fluid_msg.pack();

        struct ofl_msg_header* msg = nullptr;
        ofl_msg_unpack(buf, fluid_msg.length(), &msg, NULL, NULL);
        if (msg) {
            //char* serialized = ofl_msg_to_string(msg, NULL);
            fprintf(stderr, (d == print_in) ? "> " : "< ");
            ofl_msg_print(stderr, msg, NULL);
            fprintf(stderr, "\n");
            //DVLOG(10) << serialized;
            //free(serialized);
        }

        ofl_msg_free(msg, NULL);
        fluid_msg::OFMsg::free_buffer(buf);
    }

    struct ReceiveHandler
        : OFConnection::ReceiveHandler<fluid_msg::OFMsg>
    {
        void process(fluid_msg::OFMsg& msg) {
            OFLog::print(msg, print_in);
        }
    };

    struct SendHandler
        : OFConnection::SendHookHandler<fluid_msg::OFMsg>
    {
        void process(fluid_msg::OFMsg& msg) {
            OFLog::print(msg, print_out);
        }
    };

    void init(Loader* loader, const Config& /*rootConfig*/) override
    {
        //const Config& config = config_cd(rootConfig, "oflog");
        auto ofserver = OFServer::get(loader);
        QObject::connect(ofserver, &OFServer::switchDiscovered,
                         this, &OFLog::onSwitchDiscovered,
                         Qt::DirectConnection);

        recv_handler = std::make_shared<ReceiveHandler>();
        send_handler = std::make_shared<SendHandler>();
    }

public slots:

    void onSwitchDiscovered(OFConnectionPtr conn)
    {
        conn->receive(recv_handler);
        conn->send_hook(send_handler);
    }

private:
    std::shared_ptr<ReceiveHandler> recv_handler;
    std::shared_ptr<SendHandler> send_handler;
};

REGISTER_APPLICATION(OFLog, {""})

} // namespace runos

#include "OFLog.moc"
