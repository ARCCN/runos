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

#include "RestListener.hh"

#include <fstream>

REGISTER_APPLICATION(RestListener, {"controller", ""})

void RestListener::registerRestHandler(RestHandler *handler)
{
    VLOG(5) << "Registered handler: " << handler->restName();
    rest_handlers[handler->restName()] = handler;
    if (handler->eventable()) {
        handler->setHash(cur_hash);
        cur_hash *= 2; ///TODO: check max value
        handler->em = em;
    }
    else {
        handler->setHash(0);
    }
}

void RestListener::init(Loader *loader, const Config &config)
{
    auto app_config = config_cd(config, "rest-listener");
    listen_port = config_get(app_config, "port", 8000);
    web_dir = config_get(app_config, "web-dir", "./build/web");

    em = new EventManager;
    server = new HttpServer(listen_port, 1);
    cur_hash = 1;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

#define REGISTER_HANDLER(method) \
    server->resource[path][#method] = [handler] (HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) { \
        json11::Json res; \
        std::stringstream ss##method; \
        request->content >> ss##method.rdbuf(); \
        std::vector<std::string> params = split(request->path, '/'); \
        params.erase(params.begin(), params.begin() + 3); \
        res = handler->handle##method(params, ss##method.str()); \
        std::string content = res.dump(); \
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content; \
    }

void RestListener::startUp(Loader *loader)
{
    // Applications handlers
    for (auto it : rest_handlers) {
        RestHandler* handler = it.second;

        for (auto apath : handler->getPathes()) {
            std::string path = "^/api/" + handler->restName() + "/" + apath.second + "/{0,1}$";
            VLOG(5) << "Registering path in REST: " << path;
            switch(apath.first) {
                case Method::GET:
                    REGISTER_HANDLER(GET);
                    break;
                case Method::PUT:
                    REGISTER_HANDLER(PUT);
                    break;
                case Method::POST:
                    REGISTER_HANDLER(POST);
                    break;
                case Method::DELETE:
                    REGISTER_HANDLER(DELETE);
                    break;
            }
        }
    }

    // Non existed applications
    /*server->resource["^/api/[a-zA-Z]+([a-zA-Z0-9-_/]+)/{0,1}$"]["GET"] = [this](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::string app = request->path;
        json11::Json error = json11::Json::object {
            {"error", "could not find application"},
            {"path", app}
        };
        std::string content = error.dump();
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };*/

    // Application's list
    server->resource["^/apps/{0,1}$"]["GET"] = [this](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::map<std::string, std::map<std::string,std::string> > app_web_info;
        for (auto it : rest_handlers) {
            RestHandler* handler = it.second;
            app_web_info[handler->restName()]["name"] = handler->displayedName();
            app_web_info[handler->restName()]["page"] = handler->page();
            app_web_info[handler->restName()]["type"] = (handler->type() == RestHandler::Application ? "Application" : (handler->type() == RestHandler::Service ? "Service" : "None"));
        }
        std::string content = json11::Json(app_web_info).dump();
        response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };

    // Timeout handler
    server->resource["^/timeout/([a-z&-]+)/([0-9]+)/{0,1}$"]["GET"] = [this](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::string param1 = request->path_match[1];
        std::string param2 = request->path_match[2];
        uint32_t last_event = atoi(param2.c_str());

        auto apps = split(param1, '&'); \
        uint32_t apps_map = 0;
        for (auto app : apps) {
            if (rest_handlers.find(app) == rest_handlers.end()) {
                LOG(ERROR) << "Listener (" << app << ") does not found!";
                continue;
            }
            apps_map = apps_map | rest_handlers[app]->getHash();
        }

        std::map<std::string, json11::Json> result;
        uint32_t last_app_event;
        json11::Json target;
        std::tie(last_app_event, target) = em->timeout(apps_map, last_event);
        result["events"] = target;
        result["last_event"] = (int)last_app_event;

        std::string resp = json11::Json(result).dump();

        response << "HTTP/1.1 200 OK\r\nContent-Length: " << resp.length() << "\r\n\r\n" << resp;
    };

    // Get files and documents
    server->default_resource["GET"] = [this](HttpServer::Response& response, std::shared_ptr<HttpServer::Request> request) {
        std::string filename = web_dir;
        std::string path = request->path;

        size_t pos;
        while((pos=path.find(".."))!=std::string::npos) {
            path.erase(pos, 1);
        }

        filename += path;
        std::ifstream ifs;

        if (path == "/") {
            filename += "start.html";
        }

        ifs.open(filename, std::ifstream::in);
        if(ifs) {
            ifs.seekg(0, std::ios::end);
            size_t length=ifs.tellg();

            ifs.seekg(0, std::ios::beg);

            response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";

            //read and send 128 KB at a time if file-size>buffer_size
            size_t buffer_size=131072;
            if(length>buffer_size) {
                std::vector<char> buffer(buffer_size);
                size_t read_length;
                while((read_length=ifs.read(&buffer[0], buffer_size).gcount())>0) {
                    response.stream.write(&buffer[0], read_length);
                    response << HttpServer::flush;
                }
            }
            else {
                response << ifs.rdbuf();
            }

            ifs.close();
        }
        else {
            std::string content="Could not find " + path;
            response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
        }
    };
    
    // Starting and detaching thread
    std::thread server_thread([this](){
        server->start();
    });
    server_thread.detach();
}
