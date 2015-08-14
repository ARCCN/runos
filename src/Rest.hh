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

#include "server_http.hpp"
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

enum Method {
    GET = 1,
    PUT = 2,
    POST = 4,
    DELETE = 8
};

typedef std::pair<Method, std::string> RestReq;

class RestHandler {
    std::vector<RestReq> pathes;
    uint32_t _hash;
    EventManager* em;

    void setHash(uint32_t hash) { _hash = hash; }
    friend class RestListener;
protected:
    void acceptPath(Method method, std::string path) {
        pathes.push_back(std::make_pair(method, path));
    }
    void addEvent(Event::Type type, AppObject* obj) {
        if (eventable() && em)
            em->addToEventList(type, obj, this);
    }

public:
    enum AppType {
        Application = 0,
        Service = 1,
        None = 2
    };

    virtual std::string restName() = 0;

    virtual bool eventable() = 0;

    virtual json11::Json handleGET(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: GET"}};}
    virtual json11::Json handlePUT(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: PUT"}};}
    virtual json11::Json handlePOST(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: POST"}};}
    virtual json11::Json handleDELETE(std::vector<std::string> params, std::string body){return json11::Json::object{{restName(), "not allowed method: DELETE"}};}

    std::vector<RestReq> getPathes() { return pathes; }
    uint32_t getHash() { return _hash; }

    virtual std::string displayedName() { return restName(); }
    virtual std::string page() { return "none"; }
    virtual AppType type() { return Application; }
};
