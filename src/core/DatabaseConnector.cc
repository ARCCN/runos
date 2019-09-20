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

#include "DatabaseConnector.hpp"

#include "redisdb/redisdatabase.hpp"
#include "Config.hpp"

namespace runos {

REGISTER_APPLICATION(DatabaseConnector, {""})

void DatabaseConnector::init(Loader* loader, const Config& root_config)
{
    auto& config = config_cd(root_config, "database-connector");
    rdb_ = std::make_unique<RedisDatabase>();
    db_address_ = config_get(config, "db-address", "127.0.0.1");
    db_port_ = config_get(config, "db-port", 6379);

    if (rdb_->connectToStore(db_address_.c_str(), db_port_) >= 0) {
        rdb_->setupMasterRole();
    }
}

Json DatabaseConnector::getJson(const std::string& prefix,
                                const std::string& key) const
{
    return rdb_->getDoc(std::string{prefix + ":" + key}.c_str());
}

void DatabaseConnector::delJson(const std::string& prefix,
                                const std::string& key) const
{
    rdb_->delValue(std::string{prefix + ":" + key}.c_str());
}

void DatabaseConnector::putSValue(const std::string &prefix,
                                  const std::string &key,
                                  const std::string &str) const
{
    rdb_->putValue(std::string{prefix + ":" + key}, str);
}

std::string DatabaseConnector::getSValue(const std::string& prefix,
                                         const std::string& key) const
{
    return rdb_->getValue(std::string{prefix + ":" + key});
}

std::vector<std::string>
DatabaseConnector::getKeys(const std::string& prefix) const
{
    auto dotted_prefix = prefix + ":";
    std::vector<std::string> ret = rdb_->getKeys(dotted_prefix);
    for(auto& key : ret) {
        key.erase(0, dotted_prefix.length());
    }
    return ret;
}

void DatabaseConnector::deleteAllKeys() const
{
    rdb_->clearDB();
}

void DatabaseConnector::delPrefix(const std::string& prefix) const
{
    std::vector<std::string> ret = getKeys(prefix);
    for(auto &key : ret) {
        delJson(prefix, key);
    }
}

bool DatabaseConnector::hasConnection() const
{
    return rdb_->hasConnection();
}

void DatabaseConnector::setupMasterRole() const
{
    rdb_->setupMasterRole();
}

void DatabaseConnector::setupSlaveOf(const char* address, int port) const
{
    rdb_->setupSlaveOf(address, port);
}

std::string DatabaseConnector::getDatabaseAddress() const
{
    return db_address_;
}

uint32_t DatabaseConnector::getDatabasePort() const
{
    return db_port_;
}

}
