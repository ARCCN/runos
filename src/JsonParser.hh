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

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <glog/logging.h>

class JSONparser
{
public:
    std::string src;
public:
    JSONparser();
    JSONparser(bool array);
    void addToArray(JSONparser value);
    std::string get();
    void fill(std::string JSONtext);
    void addKey(std::string key);
    void addKey(std::string key, std::string baseKey);
    void addValue(std::string baseKey, std::string value);
    void addValue(std::string baseKey, JSONparser value);
    void addValue(std::string baseKey, int value);
    void addValue(std::string baseKey, uint32_t value);
    void addValue(std::string baseKey, uint64_t value);
    void addValue(std::string baseKey, bool value);
    void addValue(std::string baseKey, const char* value);
    std::string getValue(std::string baseKey);
    std::string getValue(int num);
};
