#include <vector>

#include "Common.hh"
#include "AppProvider.hh"

AppProvider::AppProvider(const Config& config)
    : m_config(config), m_state(INITIALIZING)
{ }

Application* AppProvider::app(AppInterfaceId interface) const {
    auto info_it = m_apps.find(interface);
    if (info_it == m_apps.end())
        throw std::out_of_range(std::string("App ") + interface + " not found");

    if (info_it->second.state != APP_INITIALIZED)
        throw std::out_of_range("App is not initialized yet");
    return info_it->second.app;
}

void AppProvider::startAll()
{
    for (auto app : Application::registry) {
        DLOG(INFO) << "Registering interface " << app->provides();
        if (!m_apps.insert({app->provides(), app}).second) {
            LOG(DFATAL) << "Trying to register already known interface (" <<
                    app->provides() << ")";
        }
    }

    LOG(INFO) << "Initializing...";
    auto services = m_config.at("services");
    for (auto& serviceName : services.array_items())
        initialize(serviceName.string_value());

    // TODO: start applications in separate threads
    LOG(INFO) << "Starting...";
    m_state = STARTING;
    for (auto& servicePair : m_apps) {
        auto& info = servicePair.second;
        if (info.state != APP_INITIALIZED)
            continue;
        
        try {
            LOG(INFO) << "  startUp(" << servicePair.first << ")";
            info.app->startUp(this);
            info.state = APP_STARTED;
        } catch(...) {
            LOG(FATAL) << "Failed to start " << servicePair.first;
        }
    }

    m_state = RUNNING;
    LOG(INFO) << "Controller is up!";
}

AppProvider::AppInfo* AppProvider::initialize(AppInterfaceId serviceId)
{
    auto app_it = m_apps.find(serviceId);
    if (app_it == m_apps.end()) {
        LOG(ERROR) << "Can't find application " << serviceId;
        return nullptr;
    }

    AppInfo& appInfo = app_it->second;
    Application* app = appInfo.app;
    if (appInfo.state != APP_REGISTERED)
        return &appInfo;

    appInfo.state = APP_INITIALIZING;

    for (auto dependsOn = app->dependsOn(m_config);
         !dependsOn->empty();
         ++dependsOn)
    {
        AppInfo* dependency = initialize(*dependsOn);
        if (dependency == nullptr) {
            return nullptr;
        }
        if (dependency->state == APP_INITIALIZING) {
            LOG(FATAL) << "Cyclic dependencies detected";
            // TODO: print cycle to log
        }
    }

    LOG(INFO) << "  init(" << serviceId << ")";
    appInfo.app->init(this, m_config);
    appInfo.state = APP_INITIALIZED;

    return &appInfo;
}
