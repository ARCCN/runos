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

#include <time.h>
#include <string>
#include <list>

#include "json11.hpp"
#include "AppObject.hh"

class RestHandler;

/**
 * Generate unique identifier for events.
 * All events in all applications have unique id.
 */
class EventIDs {
    static uint32_t last_event;
public:
    static uint32_t getLastID();
};

/**
 *
 */
class Event {
public:

    /**
     * Type of occurred event:
     * Add      - appeared new object in application
     * Delete   - disappeared some object in application
     * Change   - some object changed some properties
     */
    enum Type {
        Add,
        Delete,
        Change
    };

    /**
     * Getters:
     *  - event identifier
     *  - event time
     *  - event type
     *  - object corresponding to the event
     *  - JSON-description of the object
     */
    uint32_t id() const;
    time_t ev_time() const;
    Type type() const;
    AppObject* obj() const;
    std::string event() const;
    uint32_t hash() const;
    Event* brother() const;
    std::string app() const;

    /**
     * Main constructor in event model.
     * When event occurred, you must:
     *  - add new event: my_app->addEvent(Event::Add, new_obj);
     */
    Event(Type type, AppObject* obj, RestHandler *rest = nullptr);

    Event() = delete;
    ~Event();

    /**
     * Returns JSON-object corresponding the event
     */
    json11::Json to_json() const;

private:
    struct EventImpl* m;
    friend class EventManager;
};

class EventManager {
public:
    EventManager();
    ~EventManager();

    bool checkOverlap(Event *test_ev, uint32_t last_ev);
    void addEvent(Event::Type type, AppObject* obj);
    void addToEventList(Event::Type type, AppObject* obj, RestHandler *rest);
    std::list<Event*> events();

    std::pair<uint32_t, json11::Json> timeout(uint32_t hash_map, uint32_t last);
private:
    struct EventManagerImpl* m;

    Event* findBrother(Event *event);
    void setBrother(Event* event);
};
