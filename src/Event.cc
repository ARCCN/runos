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
    std::string app;
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

Event::Event(std::string app, Type type, AppObject *obj)
{
    m = new EventImpl;
    m->ev_time = time(NULL);
    m->id = EventIDs::getLastID();
    m->app = app;
    m->type = type;
    m->obj = obj;
}

Event::~Event()
{
    delete m;
}

JSONparser Event::formJSON()
{
    JSONparser res;
    res.addValue("app", app());
    res.addValue("time", static_cast<uint64_t>(ev_time()));
    res.addValue("event_id", id());
    res.addValue("type", type() == Add ? "Add" : (type() == Delete ? "Delete" : "Change"));
    res.addValue("obj_id", obj()->id());
    res.addValue("obj_info", obj()->formJSON());
    return res;
}

uint32_t Event::id() const
{ return m->id; }

time_t Event::ev_time() const
{ return m->ev_time; }

std::string Event::app() const
{ return m->app; }

Event::Type Event::type() const
{ return m->type; }

AppObject *Event::obj() const
{ return m->obj;}

std::string Event::event() const
{ return m->obj->formJSON().get();}

EventManager::EventManager()
{ m = new EventManagerImpl; }

EventManager::~EventManager()
{ delete m; }

void EventManager::addEvent(Event *ev)
{
    if (ev != NULL)
        m->events.push_back(ev);
}

std::vector<Event*> EventManager::events()
{ return m->events; }

JSONparser EventManager::timeout(std::string app, uint32_t last)
{
    JSONparser res;
    res.addKey(app);
    for (auto it : events()) {
        if (it->id() > last) {
            res.addValue(app, it->formJSON());
        }
    }
    return res;
}
