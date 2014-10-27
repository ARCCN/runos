#include "Common.hh"
#include <fstream>
#include <sstream>

#include <QtCore>
#include <json11.hpp>

#include "AppProvider.hh"

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
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    QCoreApplication app(argc, argv);

    std::string configFile = "network-settings.json";
    if (argc == 2) {
        configFile = argv[1];
    }
    Config config = loadConfig(configFile, "default");

    AppProvider appProvider(config);
    appProvider.startAll();

    return app.exec();
}
