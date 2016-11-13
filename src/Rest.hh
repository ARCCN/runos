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

/** @file */
#pragma once

#include <QtCore>
#include <string>
#include <unordered_map>

#include "Event.hh"
#include "json11.hpp"

#include "server_http.hpp"
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

enum Method {
    GET = 1,
    PUT = 2,
    POST = 4,
    DELETE = 8
};

typedef std::pair<Method, std::string> RestReq;

/**
 * RuNOS supports REST API, and you may write you own REST Application by using RestHandler
 */
class RestHandler {
    std::vector<RestReq> pathes;
    uint32_t _hash;
    EventManager* em;

    void setHash(uint32_t hash) { _hash = hash; }
    friend class RestListener;
protected:
    /**
     *
     * Path of avaible REST request in application
     *
     * @param method one of GET/PUT/POST/DELETE
     * @param path avaible path in regular expression
     */
    void acceptPath(Method method, std::string path) {
        pathes.push_back(std::make_pair(method, path));
    }

    /**
     * If applcation support event model, it can register event by this method
     *
     * @param type type of event : ADD, MODIFY, DELETE.
     * @param obj the object to which the event occurred.
     */
    void addEvent(Event::Type type, AppObject* obj) {
        if (eventable() && em)
            em->addToEventList(type, obj, this);
    }

public:
    // useless thing :). Probably, used just for apps sepparation in web ui
    enum AppType {
        Application = 0,
        Service = 1,
        None = 2
    };

    /**
     * Use to get app name
     */
    virtual std::string provides() const = 0;

    /**
     * Get name of rest application
     */
    virtual std::string restName() final
    {
        return provides();
    }

    /**
     * Supporting or not event model
     */
    virtual bool eventable() = 0;

    /**
    * Handler of GET request
    * @return json which will be replyed.
    */
    virtual json11::Json handleGET(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: GET"}};}

    /**
    * Handler of PUT request
    * @return json which will be replyed.
    */
    virtual json11::Json handlePUT(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: PUT"}};}

    /**
    * Handler of POST request
    * @return json which will be replyed.
    */
    virtual json11::Json handlePOST(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: POST"}};}

    /**
    * Handler of DELETE request
    * @return json which will be replyed.
    */
    virtual json11::Json handleDELETE(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: DELETE"}};}

    std::vector<RestReq> getPathes() { return pathes; }

    /**
     * Every application have own unique hash.
     * Get Hash of application
     */
    uint32_t getHash() { return _hash; }

    /**
     * Get name of rest application (name in webUI (left collumn))
     */
    virtual std::string displayedName() { return restName(); }

    /**
     * Application may has own page, and this method returned path to this page.
     */
    virtual std::string page() { return "none"; }
    virtual AppType type() { return Application; }
};
