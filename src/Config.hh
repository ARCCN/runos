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

#include <json11.hpp>

// TODO: implement full-featured interface
typedef json11::Json::object Config;

inline std::string config_get(const Config& config,
                              const std::string& key,
                              const std::string& defval)
{
    auto it = config.find(key);
    return (it != config.end()) ?
        it->second.string_value() : defval;
}

inline std::string config_get(const Config& config,
                              const std::string& key,
                              const char* defval)
{
    return config_get(config, key, std::string(defval));
}

inline int config_get(const Config& config,
                      const std::string& key,
                      int defval)
{
    auto it = config.find(key);
    return (it != config.end()) ?
        it->second.int_value() : defval;
}

inline double config_get(const Config& config,
                         const std::string& key,
                         double defval)
{
    auto it = config.find(key);
    return (it != config.end()) ?
        it->second.number_value() : defval;
}

inline bool config_get(const Config& config,
                       const std::string& key,
                       bool defval)
{
    auto it = config.find(key);
    return (it != config.end()) ?
        it->second.bool_value() : defval;
}

inline Config config_cd(const Config& config,
                        const std::string& key)
{
    auto it = config.find(key);
    return (it != config.end()) ? 
        it->second.object_items() : Config();
}
