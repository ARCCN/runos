#pragma once

#include <QtCore>
#include <unordered_map>

#include "Config.hh"
#include "Application.hh"

/**
* Manages controller applications.
*/
class AppProvider : public QObject {
    Q_OBJECT
public:
    typedef Application::AppInterfaceId AppInterfaceId;

    /**
    * Constructor.
    * @param config Reads 'services' key to get list of applications to load.
    */
    explicit AppProvider(const Config& config);

    /**
    * @param interface Application id (given by Application::provides()).
    * @return Application instance or nullptr.
    */
    Application* app(AppInterfaceId interface) const;

    /**
    * Initialize and start all applications referenced in configuration.
    */
    void startAll();

private:
    enum AppState {
        APP_REGISTERED,
        APP_INITIALIZING,
        APP_INITIALIZED,
        APP_STARTED
    };

    enum ProviderState {
        INITIALIZING,
        STARTING,
        RUNNING
    };

    struct AppInfo {
        Application* app;
        AppState state;

        AppInfo() : app(nullptr) { }
        AppInfo(Application* _app) :
            app(_app), state(APP_REGISTERED)
        { }
    };

    const Config& m_config;
    ProviderState m_state;
    std::unordered_map<AppInterfaceId, AppInfo>
        m_apps;

    // Recursive initialization
    AppInfo* initialize(AppInterfaceId service);
};
