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

#include "Loader.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "api/OFConnection.hpp"
#include "api/Port.hpp"
#include "api/SwitchFwd.hpp"

#include <runos/ResourceLocator.hpp>

#include <runos/core/logging.hpp>
#include <runos/core/catch_all.hpp>
//#include <runos/core/crash_reporter.hpp>

#include <cxxopts.hpp>

namespace runos {

class RunosApplication : public QCoreApplication
{
public:
    RunosApplication(int &argc, char *argv[])
        : QCoreApplication(argc, argv)
    { }

    bool notify( QObject * receiver, QEvent * event ) override
    {
        bool ret = false;
        catch_all_and_log([&]() {
            ret = QCoreApplication::notify(receiver, event);
        });
        return ret;
    }
};

static void initQtMetaTypes()
{
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<runos::OFConnectionPtr>("OFConnectionPtr");
    qRegisterMetaType<runos::PortPtr>("PortPtr");
    qRegisterMetaType<runos::SwitchPtr>("SwitchPtr");
}

// Singletons
static Rc<ResourceLocator> resource_locator;
Rc<ResourceLocator> ResourceLocator::get() { return resource_locator; }

} // namespace runos

#include <signal.h>

int main(int argc, char* argv[]) {
    using namespace runos;

    signal(SIGPIPE, SIG_IGN);

    initQtMetaTypes();

    cxxopts::Options options(argv[0], " - RUNOS OpenFlow controller");

    options.add_options()
        ("c,conf", "Path to configuration file", cxxopts::value<std::string>()
            ->default_value("network-settings.json"))
        ("tooldir", "Path to tools executables", cxxopts::value<std::string>())
        ("etcdir", "Path to etc dir", cxxopts::value<std::string>())
        ("dumpdir", "Path to crashdump dir", cxxopts::value<std::string>())
        //("openflow", "OpenFlow address and port to listen", cxxopts::value<std::string>())
        //("rest", "REST address and port to listen", cxxopts::value<std::string>())
        //("disable", "Disable application", cxxopts::value<std::vector<std::string>>())
        //("enable", "Enable application", cxxopts::value<std::vector<std::string>>())
        ("help", "Print this")
    ;
    options.parse(argc, argv);
    if (options.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        return 0;
    }

    resource_locator = ResourceLocator::make(options["etcdir"].as<std::string>(),
                                             options["tooldir"].as<std::string>());

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
    google::InstallFailureWriter(&MessageHandle);
    //crash_reporter::setup(options["dumpdir"].as<std::string>().c_str());

    RunosApplication app(argc, argv);

    Config config = loadConfig(options["conf"].as<std::string>());
    Loader loader(config);
    loader.startAll();

    return app.exec();
}
