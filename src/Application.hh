/** @file */
#pragma once

#include <string>
#include <vector>

#include <QtCore>

#include "Config.hh"

/**
* A starting point of your RuNOS application.
*/
class Application : public QObject {
public:
    typedef std::string const * DependencyList;

    /**
    * A unique string that identifies the app and can be used by other apps
    * to access your application's signals and methods.
    */
    virtual std::string provides() const = 0;

    /**
    * A list of application dependencies. You are able to use
    * configuration of your app to make more precise result.
    */
    virtual DependencyList dependsOn(const Config& config) const = 0;

    /**
    * Called when application is ready to be initialized, e.g. all
    * dependencies are ready to start. You should connect signals
    * and setup other handlers and properties here.
    * @param loader An interface you could use to access other applications.
    * @param config Configuration.
    */
    virtual void init(class Loader* loader, const Config& config) = 0;

    /**
    * Called when all applications are initialized. At this point your
    * app should start doing side-effects such as listening for sockets,
    * activating timers and emitting signals.
    * @param loader An interface you could use to access other applications.
    */
    virtual void startUp(class Loader*) { }

    /**
    * Application constructor. Typically your application lives as a
    * static object, so this constructor will be called before main().
    * Note, that Loader can move your application to another thread
    * (using QObject::moveToThread()) before init().
    */
    Application();

    /**
    * Destructor. Currently called at application exit.
    */
    virtual ~Application();
protected:
    friend class Loader;

    // List of all registered applications.
    static std::vector<Application*> registry;
};

/** A shortcut to quickly register your application */
#define REGISTER_APPLICATION_NODEPENDS(klass) \
    static klass application_instance;

/** A shortcut to quickly register your application (with static dependencies) */
#define REGISTER_APPLICATION(klass, ...) \
    Application::DependencyList klass::dependsOn(const Config& ) const  \
        { static std::string const ret[] = __VA_ARGS__; return ret; } \
    REGISTER_APPLICATION_NODEPENDS(klass)
