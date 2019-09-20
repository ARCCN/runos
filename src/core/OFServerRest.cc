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

#include "OFServer.hpp"
#include "RestListener.hpp"

#include <chrono>

namespace runos {

struct OFServerCollection : rest::resource
{
    OFServer* app;

    explicit OFServerCollection(OFServer* app)
        : app(app)
    { }

    rest::ptree Get() const override {
        rest::ptree root;

        std::chrono::system_clock::time_point start = app->get_start_time();
        std::time_t t = std::chrono::system_clock::to_time_t(start);
        std::chrono::system_clock::time_point end =
                    std::chrono::system_clock::now();
        uint64_t rx = app->get_rx_openflow_packets();
        uint64_t tx = app->get_tx_openflow_packets();
        uint64_t pkt_in = app->get_pkt_in_openflow_packets();
        uint64_t uptime = std::chrono::duration_cast<std::chrono::seconds>
                    (end - start).count();
        int conn_num = app->connections().size();

        root.put("ctrl_switches", conn_num);
        root.put("ctrl_start_time", t);
        root.put("ctrl_start_time_t", std::ctime(&t));
        root.put("ctrl_uptime_sec", uptime);
        root.put("ctrl_rx_ofpackets", rx);
        root.put("ctrl_pkt_in_ofpackets", pkt_in);
        root.put("ctrl_tx_ofpackets", tx);

        return root;
    }
};

class OFServerRest: public Application
{
    SIMPLE_APPLICATION(OFServerRest, "of-server-rest")

public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = OFServer::get(loader);
        auto rest_ = RestListener::get(loader);

        rest_->mount(path_spec("/of-server/info/"), [=](const path_match&)
        {
            return OFServerCollection {app};
        });
    }
};

REGISTER_APPLICATION(OFServerRest, {"rest-listener", "of-server", ""})

}
