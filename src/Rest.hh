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
#include <string>
#include <unordered_map>

#include "Event.hh"
#include "json11.hpp"

/**
 * Implements the REST-interface for applications
 */
class Rest : public QObject {
    friend class RestListener;
    friend class Listeners;

    bool has_event_model;
public:

    /**
     * Type of the application. Not used in this version
     */
    enum Type {
        Application,
        Service
	};

    /**
     * Default constructor which define displayed name of application in GUI,
     * link to webpage of application, and type of application
     */
    Rest(std::string name, std::string page, Type type): has_event_model(false), 
                                                         app_type(type),
                                                         em(NULL),
                                                         display_name(name),
                                                         webpage(page) 
    {}

    /**
     * Method is used for handling REST-requests
     * The input is the vector of input parameters
     * params[0] is request type: GET, POST, PUT, DELETE, etc;
     * params[1] is the name of application;
     * params[2..k] are GET parameters in address bar;
     * params[k..n] are POST parameters in request's body.
     * The output is reply for request in JSON-string format.
     */
    virtual std::string handle(std::vector<std::string> params) = 0;

    /**
     * Method returns the number of objects in application
     * For example, number of switches in Switch Manager apllication
     */
    virtual int count_objects() { return 0; }

    /**
     * Method returns true if application supports event model
     */
    bool hasEventModel() {
        return has_event_model;
    }

    json11::Json to_json() const {
        return json11::Json::object{
            {"name", display_name},
            {"page", webpage}
        };
    }

protected:
    /**
     * List of applications with whom your application can communicate
     */
    std::unordered_map<std::string, Rest*> apps_list;

    /**
     * Application type
     */
    Type app_type;

    /**
     * Event class for your application. By default is NULL.
     * You can create event object by makeEventApp() method.
     */
    EventManager* em;

    /**
     * Displayed name of the application in Web GUI
     */
    std::string display_name;

    /**
     * Webpage of the application in Web GUI
     */
    std::string webpage;

    /**
     * Enable event model for your application.
     * Now you can generate some events in your application,
     * and users can ask you app ("/timeout/<your_app>/<last_event>") about new events
     * Events must be comply with certain objects (see AppObject.hh)
     */
    void makeEventApp() {
        em = new EventManager;
        has_event_model = true;
    }

    /**
     * Add new event occurred in your application
     * Executing only if your application supprots event model
     */
    void addEvent(Event::Type type, AppObject* obj) {
        if (has_event_model)
            em->addEvent(type, obj);
    }
    void addApp(std::string name, Rest* app) {
        if (app != NULL)
            apps_list[name] = app;
    }  
};
