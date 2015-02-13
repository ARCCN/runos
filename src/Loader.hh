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
