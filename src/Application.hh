/** @file */
#pragma once

#include <QtCore>

#include <vector>
#include <string>
#include "Config.hh"

class AppProvider;

/**
* An interface to build applications for the controller.
*/
class Application : public QObject {
    Q_OBJECT
public:
    typedef std::string AppInterfaceId;
    typedef AppInterfaceId const * DependencyList;

    /**
    * A unique string that identifies app and can be used by other apps
    * to access your application's signals and methods.
    */
    virtual AppInterfaceId provides() const = 0;

    /**
    * A list of application dependencies. You are able to use
    * configuration of your app to form more precise list.
    */
    virtual DependencyList dependsOn(const Config& config) const = 0;

    /**
    * Called when application is ready to be initialized, e.g. all
    * dependencies are ready to start. You should connect signals
    * and setup other handlers and properties here.
    * @param provider An interface you could use to access other applications.
    * @param config Configuration.
    */
    virtual void init(AppProvider* provider, const Config& config) = 0;

    /**
    * Called when all applications are initialized. At this point your
    * app should start doing side-effects such as listening for sockets,
    * activating timers and emitting signals.
    * @param provider An interface you could use to access other applications.
    */
    virtual void startUp(AppProvider* provider) = 0;

    /**
    * Application constructor. Typically your application lives as a
    * static object, so this constructor will be called before main().
    * Note, that AppProvider can move your application to another thread
    * before init is called using moveToThread().
    */
    Application();

    /**
    * Destructor. Currently called on application exit.
    */
    virtual ~Application();
protected:
    friend class AppProvider;
    static std::vector<Application*> registry;
};

/** A shortcut to quickly override Application class (headers). */
#define APPLICATION(klass) \
    AppInterfaceId provides() const override; \
    DependencyList dependsOn(const Config&) const override; \
    static klass* get(AppProvider* provider); \
    klass(); ~klass();

/** A shortcut to quickly announce your application (without dependencies) */
#define REGISTER_APPLICATION_NODEPENDS(klass, provides_val) \
    Application::AppInterfaceId klass::provides() const { return provides_val; } \
    klass* klass::get(AppProvider* provider) { \
        return dynamic_cast<klass*>(provider->app(provides_val)); \
    } \
    static klass application_instance;

/** A shortcut to quickly announce your application (with static dependencies) */
#define REGISTER_APPLICATION(klass, provides, ...) \
    Application::DependencyList klass::dependsOn(const Config& ) const  \
        { static AppInterfaceId const ret[] = __VA_ARGS__; return ret; } \
    REGISTER_APPLICATION_NODEPENDS(klass, provides)
