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

#include "Event.hh"

#include <boost/lexical_cast.hpp>

#include <map>
#include <vector>
#include "RestListener.hh"

uint32_t EventIDs::last_event = 0;

uint32_t EventIDs::getLastID()
{
    return ++last_event;
}

struct EventImpl {
    uint32_t    id;
    time_t      ev_time;
    Event::Type type;
    AppObject*  obj;
    uint32_t _hash;
    Event* _brother;
    std::string app;
};

struct EventManagerImpl {
    std::list<Event*> events;
};

Event::Event(Type type, AppObject *obj, RestHandler* rest)
{
    m = new EventImpl;
    m->ev_time = time(NULL);
    m->id = EventIDs::getLastID();
    m->type = type;
    m->obj = obj;
    if (rest) {
        m->_hash = rest->getHash();
        m->app = rest->restName();
    }

    if (type == Add) {
        m->_brother = nullptr;
    }
}

Event::~Event()
{ 
    delete m;
}

json11::Json Event::to_json() const
{
    std::string ev_type = (type() == Add ? "Add" : (type() == Delete ? "Delete" : "Change"));
    return json11::Json::object {
        {"time", boost::lexical_cast<std::string>(static_cast<uint64_t>(ev_time()))},
        {"event_id", (int)id()},
        {"type", ev_type},
        {"obj_id", obj()->id_str()},
        {"obj_info", json11::Json(obj())},
        {"app", app()}
    };
}

uint32_t Event::id() const
{ return m->id; }

time_t Event::ev_time() const
{ return m->ev_time; }

Event::Type Event::type() const
{ return m->type; }

AppObject *Event::obj() const
{ return m->obj; }

std::string Event::event() const
{ return json11::Json(m->obj).dump(); }

uint32_t Event::hash() const
{ return m->_hash; }

Event* Event::brother() const
{ return m->_brother; }

std::string Event::app() const
{ return m->app; }

EventManager::EventManager()
{ m = new EventManagerImpl; }

EventManager::~EventManager()
{ delete m; }

std::list<Event *> EventManager::events()
{ return m->events; }

void EventManager::addEvent(Event::Type type, AppObject* obj)
{
    Event* ev = new Event(type, obj);
    if (ev != nullptr)
        m->events.push_back(ev);
}

bool EventManager::checkOverlap(Event* test_ev, uint32_t last_ev)
{
    if (test_ev->type() == Event::Add) {
        if (test_ev->brother() != nullptr)
            return true; //no need add
        else
            return false; //need add
    }

    else if (test_ev->type() == Event::Delete) {
        if (test_ev->brother()->id() <= last_ev)
            return false; //need add
        else
            return true; //no need add
    }
    return false;
}

std::pair<uint32_t, json11::Json>
EventManager::timeout(uint32_t hash_map, uint32_t last)
{
    std::map<std::string, std::vector<json11::Json> > result;
    std::pair<uint32_t, json11::Json> ret;
    uint32_t last_event = 0;
    for (auto it : events()) {
        if ((it->hash() & hash_map) && (it->id() > last)) {
            if (!checkOverlap(it, last)) {
                result[it->app()].push_back(it->to_json());
                if (it->id() > last_event) {
                    last_event = it->id();
                }
            }
        }
    }
    if (last_event == 0) {
        last_event = last;
    }
    ret.first = last_event;
    ret.second = json11::Json(result);
    return ret;
}

void EventManager::addToEventList(Event::Type type, AppObject *obj, RestHandler* rest)
{
    if (rest->getHash() == 0) {
        return;
    }

    Event* ev = new Event(type, obj, rest);
    if (ev != nullptr) {
        if (type == Event::Delete) {
            setBrother(ev);
        }
        m->events.push_back(ev);
    }
}

Event* EventManager::findBrother(Event* event)
{
    for (auto it = m->events.rbegin(); it != m->events.rend(); it++) {
        if ((*it)->obj() == event->obj() && (*it)->type() == Event::Add) {
            return *it;
        }
    }
    return nullptr;
}

void EventManager::setBrother(Event *event)
{
    Event* ev = findBrother(event);
    event->m->_brother = ev;
    ev->m->_brother = event;
}
