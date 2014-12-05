#pragma once

#include <QtCore>

#include "Config.hh"

class Application;

/**
* Manages RuNOS applications.
*/
class Loader : public QObject {
    Q_OBJECT
public:
    /**
    * Constructor.
    * @param config Reads 'services' key to get list of applications to load.
    */
    explicit Loader(const Config& config);
    ~Loader();

    /**
    * @param interface Application id (given by Application::provides()).
    * @return Application instance or nullptr.
    */
    Application* app(std::string interface) const;

    /**
    * Initialize and start all applications referenced in configuration.
    */
    void startAll();

private:
    struct LoaderImpl *m;
};

/**
 * Defines an RuNOS interface. You should use this macro inside class before 
 * any class definitions (just like Q_OBJECT).
 */
#define INTERFACE(klass, provides_val) \
    public: \
        static std::string provides() { return provides_val; } \
        static QObject* get(Loader* loader) { \
            return dynamic_cast<QObject*>(loader->app(provides_val)); \
        } \
    private: \

/**
 * A shortcut to quickly override required definitions.
 * In this variant you should split your application implementation and interface.
 */
#define APPLICATION(klass, interface) \
    Q_INTERFACES(interface) \
    public: \
        std::string provides() const override final { return interface::provides(); } \
        DependencyList dependsOn(const Config&) const override final; \
    private:

/**
 * A shortcut to quickly override required definitions.
 */
#define SIMPLE_APPLICATION(klass, provides_val) \
    public: \
        std::string provides() const override final { return provides_val; } \
        static klass* get(Loader* loader) { \
            return dynamic_cast<klass*>(loader->app(provides_val)); \
        } \
        DependencyList dependsOn(const Config&) const override final; \
    private:
