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

#include "Application.hpp"
#include "Loader.hpp"
#include "SwitchManager.hpp"
#include "CommandLine.hpp"

#include <boost/lexical_cast.hpp>
#include <fluid/of13msg.hh>

#include <iostream>
#include <algorithm>

namespace runos {

namespace of13 = fluid_msg::of13;

class SwitchManagerCli : public Application
{
    SIMPLE_APPLICATION(SwitchManagerCli, "switch-manager-cli")
public:
    void init(Loader* loader, const Config&) override
    {
        auto app = SwitchManager::get(loader);
        auto cli = CommandLine::get(loader);

        auto extract_switch = [=](auto const& dpid_str) {
            auto dpid = boost::lexical_cast<uint64_t>(dpid_str);
            auto sw = app->switch_(dpid);
            if (not sw) {
                cli->error("No switch for dpid={0:x}", dpid);
            }
            return sw;
         };

        cli->register_command(
            cli_pattern("switch\\s+info\\s+([0-9]+)"),
            [=](const cli_match& match) {
                auto sw = extract_switch(match[1]);

                cli->print("{:<25}: {:x}", "id", sw->dpid());
                cli->print("");
                cli->print("{:<25}: {}", "manufacturer", sw->manufacturer());
                cli->print("{:<25}: {}", "hardware", sw->hardware());
                cli->print("{:<25}: {}", "software", sw->software());
                cli->print("{:<25}: {}", "serial #", sw->serial_number());
                cli->print("{:<25}: {}", "description", sw->description());
                cli->print("");
                cli->print("{:<25}: {}", "nbuffers", sw->nbuffers());
                cli->print("{:<25}: {}", "ntables", sw->ntables());
                cli->print("{:<25}: {}", "capabilites", sw->capabilites());
                cli->print("");
                cli->print("{:<25}: {}", "miss_send_len", sw->miss_send_len());
                cli->print("{:<25}: {}", "fragment_policy", sw->fragment_policy());

                auto ports = sw->ports();
                std::sort(ports.begin(), ports.end(),
                    [](PortPtr a, PortPtr b) {
                        return a->number() < b->number();
                    });

                cli->print("ports:");
                for (auto& port : ports) {
                    std::ostringstream status;
                    bool sep = false;

                    if (port->link_down()) {
                        status << (sep ? "," : "") << "DOWN";
                        sep = true;
                    } else {
                        status << (sep ? "," : "") << "UP";
                        sep = true;
                    }

                    if (port->live()) {
                        status << (sep ? "," : "") << "LIVE";
                        sep = true;
                    }

                    if (port->blocked()) {
                        status << (sep ? "," : "") << "BLOCKED";
                        sep = true;
                    }

                    cli->print("  {} {:<15} {} [{}]",
                               port->number() != of13::OFPP_LOCAL ?
                                 std::to_string(port->number()) : "L",
                               port->name(),
                               port->hw_addr(),
                               status.str());
                }
            });

        cli->register_command(
            cli_pattern("switch\\s+list"),
            [=](const cli_match&) {
                cli->print("{:^16} {:^20} {:^20}",
                           "DPID", "HARDWARE", "DESC");
                for (const auto& sw : app->switches()) {
                    cli->print("{:^16x} {:^20} {:^20}",
                               sw->dpid(), sw->hardware(), sw->description());
                }
            });
    }
};

REGISTER_APPLICATION(SwitchManagerCli, {"switch-manager", "command-line", ""})

}
