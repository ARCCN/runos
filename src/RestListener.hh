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

#include <unordered_map>
#include <string>

#include "Rest.hh"
#include "Application.hh"
#include "Controller.hh"
#include "Loader.hh"

#define DEBUG 1

/**
 * This application creates TCP server and listen some port (8000, by default).
 * It handles all HTTP requests including webpages and REST
 * If REST-request arrived, it parses request and sends parameters to requested REST application.
 * When this application returnes reply, RestListener sends answer to client.
 *
 * Note: Do not confuse RestListener and RestHandler classes.
 */
class RestListener: public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(RestListener, "rest-listener")
public:
    void init(Loader* loader, const Config& config) override;

    /// the main method: http-server running is here
    void startUp(Loader* loader) override;

    void registerRestHandler(RestHandler* handler);
private:
    EventManager* em;
    HttpServer* server;
    std::unordered_map<std::string, RestHandler*> rest_handlers;
    uint32_t cur_hash;

    int listen_port;
    std::string web_dir;
};
