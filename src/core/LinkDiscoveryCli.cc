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
#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "Application.hpp"
#include "Loader.hpp"
#include "LinkDiscovery.hpp"
#include "CommandLine.hpp"

namespace runos {

class LinkDiscoveryCli : public Application
{
    SIMPLE_APPLICATION(LinkDiscoveryCli, "link-discovery-cli")
public:
    void init(Loader* loader, const Config&) override
    {
        auto app = 
            dynamic_cast<LinkDiscovery*>(ILinkDiscovery::get(loader));
        auto cli = CommandLine::get(loader);

        cli->register_command(
            cli_pattern(R"(link\s+list)"),
            [=](const cli_match&) {
                for (const auto& link : app->links()) {
                    using std::chrono::seconds;
                    using std::chrono::duration_cast;

                    auto now = DiscoveredLink::valid_through_t::clock::now();
                    auto dur = link.valid_through - now;

                    cli->print("{} <-> {} valid in {} seconds",
                               link.source,
                               link.target,
                               duration_cast<seconds>(dur).count());
                }
            });
    }
};

REGISTER_APPLICATION(LinkDiscoveryCli, {"link-discovery", "command-line", ""})

}

