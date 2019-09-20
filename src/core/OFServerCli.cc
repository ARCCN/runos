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

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "Application.hpp"
#include "Loader.hpp"
#include "OFServer.hpp"
#include "CommandLine.hpp"

namespace runos {

class OFServerCli : public Application
{
    SIMPLE_APPLICATION(OFServerCli, "of-server-cli")
public:
    void init(Loader* loader, const Config&) override
    {
        auto app = OFServer::get(loader);
        auto cli = CommandLine::get(loader);

        auto extract_connection = [=](auto const& dpid_str) {
            auto dpid = boost::lexical_cast<uint64_t>(dpid_str);
            auto conn = app->connection(dpid);
            if (not conn.is_ready()) {
                cli->error("No connection for dpid={0:x}", dpid);
            }
            return conn.get();
         };

         cli->register_command(
            cli_pattern(R"(show\s+info)"),
            [=](cli_match const& match) {
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

                cli->print("{:-^90}", "  RUNOS CONTROL STATISTICS  ");
                cli->print("{:<50} {:^16d}", "Number of switches: ", conn_num);
                cli->print("{:<50} {:^16d}", "RX OpenFlow packets: ", rx);
                cli->print("{:<50} {:^16d}", "Packet-In packets: ", pkt_in);
                cli->print("{:<50} {:^16d}", "TX OpenFlow packets: ", tx);
                cli->print("{:<50} {:^16}", "RUNOS uptime(sec): ", uptime);
                cli->print("{:<50} {:^32}", "RUNOS start time:", std::ctime(&t));
                cli->print("{:-^90}", "");
            });

        cli->register_command(
            cli_pattern(R"(connection\s+close\s+([0-9]+))"),
            [=](cli_match const& match) {
                extract_connection(match[1])->close();
            });

        cli->register_command(
            cli_pattern(R"(connection\s+list)"),
            [=](cli_match const&) {
                cli->print("{:^16} {:^6} {:^32} {:^16} {:^16} {:^16} {:^16} {:^32}",
                           "DPID", "STATUS", "PEER", "UPTIME", "RX", "TX", "PACKET-IN", "START TIME");
                std::chrono::system_clock::time_point end_conn = std::chrono::system_clock::now();
                uint64_t uptime;

                for (const auto& conn : app->connections()) {
                    std::chrono::system_clock::time_point start_conn = conn->get_start_time();
                    std::time_t conn_t = std::chrono::system_clock::to_time_t(start_conn);
                    uptime = std::chrono::duration_cast<std::chrono::seconds> (end_conn - start_conn).count();

                    cli->print("{:^16x} {:^6} {:^32} {:^16d} {:^16d} {:^16d} {:^16d} {:^32}",
                               conn->dpid(),
                               conn->alive() ? "UP" : "DOWN",
                               conn->peer_address(),
                               conn->alive() ? uptime : 0,
                               conn->get_rx_packets(),
                               conn->get_tx_packets(),
                               conn->get_pkt_in_packets(),
                               conn->alive() ? std::ctime(&conn_t) : "--" );
                }
            });
    }
};

REGISTER_APPLICATION(OFServerCli, {"of-server", "command-line", ""})

}
