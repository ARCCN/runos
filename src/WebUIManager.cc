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

#include "WebUIManager.hh"

#include <boost/lexical_cast.hpp>

#include "Switch.hh"
#include "RestListener.hh"
#include "HostManager.hh"

REGISTER_APPLICATION(WebUIManager, {"switch-manager", "host-manager", "rest-listener", ""})

struct WebObjectImpl {
    uint64_t id;
    Position pos;
    std::string icon;
    std::string display_name;

    std::vector<class WebObject*> neighbors;
    int lvl;
};

struct WebUIManagerImpl {
    std::vector<WebObject*> objects;
};

WebObject::WebObject(uint64_t _id, bool is_router)
{
    m = new WebObjectImpl;
    m->id = _id;
    m->pos.x = -1;
    m->pos.y = -1;
    if (is_router)
        m->icon = "router";
    else
        m->icon = "aimac";
    m->display_name = boost::lexical_cast<std::string>(_id);
}

WebObject::~WebObject()
{
    delete m;
}

uint64_t WebObject::id() const
{
    return m->id;
}

json11::Json WebObject::to_json() const
{
    return json11::Json::object {
        {"ID", id_str()},
        {"x_coord", pos().x},
        {"y_coord", pos().y},
        {"icon", icon()},
        {"display_name", display_name()}
    };
}

Position WebObject::pos() const
{ return m->pos; }

std::string WebObject::icon() const
{ return m->icon; }

std::string WebObject::display_name() const
{ return m->display_name; }

void WebObject::pos(int x, int y)
{
    m->pos.x = x;
    m->pos.y = y;
}

void WebObject::display_name(std::string name)
{ m->display_name = name; }

WebUIManager::WebUIManager()
{
    m = new WebUIManagerImpl;
}

WebUIManager::~WebUIManager()
{
    delete m;
}

std::vector<WebObject*> WebUIManager::getObjects()
{ return m->objects; }

WebObject* WebUIManager::getObject(uint64_t _id)
{
    for (auto it : m->objects) {
        if (it->id() == _id)
            return it;
    }
    return nullptr;
}

void WebUIManager::init(Loader *loader, const Config &config)
{
    m_switch_manager = SwitchManager::get(loader);
    HostManager* host_manager = HostManager::get(loader);

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
                     this, &WebUIManager::newSwitch);

    QObject::connect(host_manager, &HostManager::hostDiscovered,
                     this, &WebUIManager::newHost);

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "webinfo/all");
    acceptPath(Method::GET, "webinfo/[0-9]+");

    acceptPath(Method::PUT, "coord/[0-9]+");
    acceptPath(Method::PUT, "name/[0-9]+");
}

void WebUIManager::newSwitch(Switch *dp)
{
    WebObject* obj = new WebObject(dp->id(), true);
    m->objects.push_back(obj);
}

void WebUIManager::newHost(Host* dev)
{
    WebObject* obj = new WebObject(dev->id(), false);
    obj->display_name(dev->mac());
    m->objects.push_back(obj);
}

json11::Json WebUIManager::handleGET(std::vector<std::string> params, std::string body)
{
    if (params[1] == "all") {
        if (getObjects().size())
            return json11::Json(getObjects());
        else
            return "{ \"error\": \"empty\" }";
    }
    else {
        uint64_t id = std::stoull(params[1]);
        WebObject* obj = getObject(id);
        if (obj) {
            return json11::Json(obj);
        }
        else {
            return "{ \"error\": \"Object not found\" }";
        }
    }
}

json11::Json WebUIManager::handlePUT(std::vector<std::string> params, std::string body)
{
    std::string parseMessage;
    if (params[0] == "coord") {
        uint64_t id = std::stoull(params[1]);
        json11::Json::object coords = json11::Json::parse(body, parseMessage).object_items();

        if (!parseMessage.empty()) {
            LOG(WARNING) << "Can't parse input request : " << parseMessage;
            return json11::Json("{}");
        }

        int x = coords["x_coord"].number_value();
        int y = coords["y_coord"].number_value();
        WebObject* obj = getObject(id);
        obj->pos(x, y);
        return "{ \"webui\": \"OK\" }";
    }

    if (params[0] == "name") {
        uint64_t id = std::stoull(params[1]);
        json11::Json::object jname = json11::Json::parse(body, parseMessage).object_items();

        if (!parseMessage.empty()) {
            LOG(WARNING) << "Can't parse input request : " << parseMessage;
            return json11::Json("{}");
        }

        std::string new_name = jname["name"].string_value();
        WebObject* obj = getObject(id);
        obj->display_name(new_name);
        return "{ \"webui\": \"OK\" }";
    }
    return "{}";
}
