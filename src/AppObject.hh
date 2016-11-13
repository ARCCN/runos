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

#include <string>
#include <time.h>

#include "json11.hpp"

/**
* This abstract class is used in applications.
* Objects in your app must inherit this class if app uses event model
*
* It is used to logging your application activity. So you need to inherit this class and implement a few methods to let
* the controller to log you application activity. Logging is based on remembering object's state (in json representation)
* For example, you can see the result of logging here: http://192.168.56.101:8000/timeout/switch-manager&topology&host-manager&flow/0
*/
class AppObject
{
    /// When object was created
    time_t since;
public:
    AppObject(): since(0) {}
    virtual ~AppObject() { }

    /**
     * You must define JSON representation for your object
     */
    virtual json11::Json to_json() const = 0;

    /**
     * 64-bit unique identifier for your object
     */
    virtual uint64_t id() const = 0;

    std::string id_str() const;

    /**
     * Object's created time getter and setter
     */
    time_t connectedSince() const;
    void connectedSince(time_t time);

    /**
     * Define the equality operator between objects
     */
    friend bool operator==(const AppObject& o1, const AppObject& o2);

    static std::string uint32_t_ip_to_string(uint32_t ip);
};
