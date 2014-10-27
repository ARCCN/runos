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
