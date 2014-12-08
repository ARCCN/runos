#include "Common.hh"
#include <fstream>
#include <sstream>
#include <json11.hpp>

#include "Loader.hh"

using namespace json11;

Config loadConfig(const std::string& fileName,
                  const std::string& profile)
{
    std::stringstream buffer;
    std::string parseMessage;

    buffer << std::ifstream(fileName).rdbuf();
    Config config =
        Json::parse(buffer.str(), parseMessage)[profile].object_items();

    if (!parseMessage.empty()) {
        LOG(FATAL) << "Can't parse config file : " << parseMessage;
    }

    return config;
}


int main(int argc, char* argv[]) {
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<of13::PortStatus>();
    qRegisterMetaType<of13::FeaturesReply>();
    qRegisterMetaType< std::shared_ptr<of13::Error> >();
    qRegisterMetaType<of13::Port>();

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    QCoreApplication app(argc, argv);

    std::string configFile = "network-settings.json";
    if (argc == 2) {
        configFile = argv[1];
    }
    Config config = loadConfig(configFile, "default");

    Loader loader(config);
    loader.startAll();

    return app.exec();
}
