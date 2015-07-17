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
};

struct EventManagerImpl {
    std::vector<Event*> events;
};

Event::Event()
{
    m = new EventImpl;
    m->ev_time = time(NULL);
    m->id = -1;
}

Event::Event(Type type, AppObject *obj)
{
    m = new EventImpl;
    m->ev_time = time(NULL);
    m->id = EventIDs::getLastID();
    m->type = type;
    m->obj = obj;
}

Event::~Event()
{ 
    delete m;
}

json11::Json Event::to_json() const
{
    std::string ev_type = (type() == Add ? "Add" : (type() == Delete ? "Delete" : "Change"));
    return json11::Json::object {
        {"time", AppObject::uint64_to_string(static_cast<uint64_t>(ev_time()))},
        {"event_id", (int)id()},
        {"type", ev_type},
        {"obj_id", obj()->id_str()},
        {"obj_info", json11::Json(obj())}
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

EventManager::EventManager()
{ m = new EventManagerImpl; }

EventManager::~EventManager()
{ delete m; }

std::vector<Event*> EventManager::events()
{ return m->events; }

void EventManager::addEvent(Event::Type type, AppObject* obj)
{
    Event* ev = new Event(type, obj);
    if (ev != NULL)
        m->events.push_back(ev);
}

bool EventManager::checkOverlap(Event* test_ev, uint32_t last_ev)
{
    if (test_ev->type() == Event::Add) {
        for (auto it : events()) {
            if (it->id() <= test_ev->id())
                continue;
            if (it->type() == Event::Delete && it->obj() == test_ev->obj())
                return true;
        }
        return false;
    }

    else if (test_ev->type() == Event::Delete) {
        for (auto it : events()) {
            if (it->id() >= test_ev->id() || it->id() <= last_ev)
                continue;
            if (it->type() == Event::Add && it->obj() == test_ev->obj())
                return true;
        }
        return false;
    }
    return false;
}

std::pair<uint32_t, json11::Json>
EventManager::timeout(uint32_t last)
{
    std::pair<uint32_t, json11::Json> ret;
    std::vector<Event*> suitable_ev;
    uint32_t last_event = 0;
    for (auto it : events()) {
        if (it->id() > last) {
            if (!checkOverlap(it, last)) {
                suitable_ev.push_back(it);
                if (it->id() > last_event)
                    last_event = it->id();
            }
        }
    }
    ret.first = last_event;
    ret.second = json11::Json(suitable_ev);
    return ret;
}
