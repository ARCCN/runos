/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include "Application.hpp"
#include "Loader.hpp"
#include "redisdb/redisdatabase.hpp"
#include "json.hpp"

namespace runos {
using json = nlohmann::json;

class DatabaseConnector final : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(DatabaseConnector, "database-connector")

public:
    void init(Loader* loader, const Config& config) override;

    // T must have a method std::string T::dump();
    // So it can be used with hlohmann::json and Json from json11
    template<typename T>
    void putJson(const std::string& prefix,
                 const std::string& key,
                 const T& json) const
    {
        rdb_->putValue(std::string{prefix + ":" + key}, json.dump());
    }

    Json getJson(const std::string& prefix, const std::string& key) const;
    void delJson(const std::string& prefix, const std::string& key) const;
    void putSValue(const std::string& prefix,
                   const std::string& key,
                   const std::string& str) const;
    std::string getSValue(const std::string& prefix,
                          const std::string& key) const;
    std::vector<std::string> getKeys(const std::string& prefix) const;
    void deleteAllKeys() const;
    void delPrefix(const std::string& prefix) const;

    bool hasConnection() const;
    void setupMasterRole() const;
    void setupSlaveOf(const char* address, int port) const;

    std::string getDatabaseAddress() const;
    uint32_t getDatabasePort() const;

private:
    std::unique_ptr<RedisDatabase> rdb_;
    std::string db_address_;
    uint32_t db_port_;
};

}
