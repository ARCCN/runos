/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include "Loader.hh"

#include <vector>
#include <unordered_map>
#include <mutex>

#include "Common.hh"
#include "Application.hh"
#include "Config.hh"

enum ApplicationState {
    APP_REGISTERED,
    APP_INITIALIZING,
    APP_INITIALIZED,
    APP_STARTED
};

enum LoaderState {
    INITIALIZING,
    STARTING,
    RUNNING
};

struct AppInfo {
    Application* app;
    ApplicationState state;

    AppInfo() : app(nullptr) { }
    AppInfo(Application* _app) :
        app(_app), state(APP_REGISTERED)
    { }
};

class AppInitializer : public QObject {
    Q_OBJECT
public:
    AppInitializer(const Config& config_)
        : config(config_)
    {
        QObject::connect(this, &AppInitializer::initializeApp,
                         this, &AppInitializer::initializeAppImpl,
                         Qt::QueuedConnection);
        QObject::connect(this, &AppInitializer::startApp,
                         this, &AppInitializer::startAppImpl,
                         Qt::QueuedConnection);
    }
signals:
    void initializeApp(Loader*, Application*, std::mutex*);
    void startApp(Loader*, Application*, std::mutex*);
protected slots:
    void initializeAppImpl(Loader*, Application*, std::mutex*);
    void startAppImpl(Loader*, Application*, std::mutex*);
private:
    const Config& config;
};

void AppInitializer::initializeAppImpl(Loader* loader, Application* app, std::mutex* end_mutex)
{
    app->init(loader, config);
    end_mutex->unlock();
}

void AppInitializer::startAppImpl(Loader* loader, Application* app, std::mutex* end_mutex)
{
    app->startUp(loader);
    end_mutex->unlock();
}

class AppThread : public QThread {
    Q_OBJECT
public:
    AppThread(const Config& config) 
        : initializer(config)
    {
        initializer.moveToThread(this);
    }

    AppInitializer initializer;
};

struct LoaderImpl {
    Loader* this_;
    const Config& config;

    LoaderState state;
    std::unordered_map<std::string, AppInfo>
        apps;

    std::vector<AppThread*> thread;
    size_t last_thread;

    // Recursive initialization
    AppInfo* initialize(std::string service);

    LoaderImpl(Loader* loader, const Config& config_)
        : this_(loader),
          config(config_),
          state(INITIALIZING),
          last_thread(0)
    {
        size_t nthreads = config_get(config_cd(config, "loader"),
                                     "threads", 1);
        thread.resize(nthreads);
        for (size_t i = 0; i < nthreads; ++i) {
            thread[i] = new AppThread(config);
            thread[i]->start();
        }
    }
};

Loader::Loader(const Config& config)
    : m(new LoaderImpl(this, config))
{ }

Loader::~Loader()
{ delete m; }

Application* Loader::app(std::string interface) const {
    auto info_it = m->apps.find(interface);
    if (info_it == m->apps.end())
        throw std::out_of_range(std::string("App ") + interface + " not found");

    if (info_it->second.state != APP_INITIALIZED)
        throw std::out_of_range("App is not initialized yet");
    return info_it->second.app;
}

void Loader::startAll()
{
    for (auto app : Application::registry) {
        DLOG(INFO) << "Registering interface " << app->provides();
        if (!m->apps.insert({app->provides(), app}).second) {
            LOG(DFATAL) << "Trying to register already known interface (" <<
                    app->provides() << ")";
        }
    }

    LOG(INFO) << "Initializing...";
    auto services = m->config.at("services");
    for (auto& serviceName : services.array_items())
        m->initialize(serviceName.string_value());

    LOG(INFO) << "Starting...";
    m->state = STARTING;
    for (auto& servicePair : m->apps) {
        auto& info = servicePair.second;
        if (info.state != APP_INITIALIZED)
            continue;
        
        try {
            LOG(INFO) << "  startUp(" << servicePair.first << ")";
            AppThread* app_thread = qobject_cast<AppThread*>(info.app->thread());
            std::mutex start_mutex;
            start_mutex.lock();
            emit app_thread->initializer.startApp(m->this_, info.app, &start_mutex);
            start_mutex.lock();
            info.state = APP_STARTED;
        } catch(...) {
            LOG(FATAL) << "Failed to start " << servicePair.first;
        }
    }

    m->state = RUNNING;
    LOG(INFO) << "Controller is up!";
}

AppInfo* LoaderImpl::initialize(std::string serviceId)
{
    auto app_it = apps.find(serviceId);
    if (app_it == apps.end()) {
        LOG(ERROR) << "Can't find application " << serviceId;
        return nullptr;
    }

    AppInfo& appInfo = app_it->second;
    Application* app = appInfo.app;
    if (appInfo.state != APP_REGISTERED)
        return &appInfo;

    appInfo.state = APP_INITIALIZING;

    for (auto dependsOn = app->dependsOn(config);
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

    AppThread *app_thread;
    int specific_thread = config_get(config_cd(config, app->provides()),
            "pin-to-thread", -1);

    if (specific_thread >= 0 && (size_t) specific_thread < thread.size()) {
        app_thread = thread[specific_thread];

        VLOG(2) << "  moveToThread(" << app->provides() 
            << ", " << specific_thread << ':' << app_thread << ")";
    } else {
        app_thread = thread[last_thread];
        if (++last_thread == thread.size())
            last_thread = 0;

        VLOG(2) << "  moveToThread(" << app->provides() 
            << ", " << last_thread << ':' << app_thread << ")";
    }


    LOG(INFO) << "  init(" << serviceId << ")";
    app->moveToThread(app_thread);

    std::mutex init_mutex;
    init_mutex.lock();
    emit app_thread->initializer.initializeApp(this_, app, &init_mutex);
    init_mutex.lock();

    appInfo.state = APP_INITIALIZED;
    return &appInfo;
}

#include "Loader.moc"
