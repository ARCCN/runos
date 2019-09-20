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

#include "redisdatabase.hpp"

#include <iostream>
#include <cstdio>
#include <string>
#include <runos/core/logging.hpp>

namespace runos {

using lock_t = std::lock_guard<std::mutex>;

RedisDatabase::RedisDatabase() :
    rclient(new SimpleRedisClient())
  , connection_state_(-1)
{
}

int RedisDatabase::connectToStore(const char* address, int port)
{
    lock_t lock(client_mutex_);
    rclient->setHost(address);
    rclient->setPort(port);
    rclient->setBufferSize(2048*4);

    connection_state_ = rclient->redis_connect();
    if (connection_state_ < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS(" << address << ":" << port
                   << ") connection ERROR(" << connection_state_
                   << "). Please, check Runos config file,"
                   " Redis Config file and connection with Redis.";
        return -1;
    } else {
        LOG(WARNING) << "[RedisDatabase] REDIS(" << address << ":"
                     << port << ") connection is OK!";
    }
    return connection_state_;
}

int RedisDatabase::auth(const char* pswd)
{
    lock_t lock(client_mutex_);
    int ret = rclient->auth(pswd);
    if (ret < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS authorization ERROR(" << ret
                   << "): authorization is not passed (Incorrect password).";
        return -1;
    } else {
        LOG(WARNING) << "[RedisDatabase] REDIS "
                        "authorization is successfully completed!";
    }
    return ret;
}

bool RedisDatabase::hasConnection()
{
    lock_t lock(client_mutex_);
    if (connection_state_ < 0)
        return false;
    return true;
}

void RedisDatabase::closeConnection()
{
    lock_t lock(client_mutex_);
	rclient->redis_close();
}

int RedisDatabase::putValue(const std::string& key, const std::string& value)
{
    lock_t lock(client_mutex_);
    int ret = rclient->set(key.c_str(), value.c_str());
    if (ret < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS SET(" << key
                   << ") fail: ERROR(" << ret << ").";
        return -1;
    }
    return ret;
}

std::string RedisDatabase::getValue(const std::string& key) const
{
    lock_t lock(client_mutex_);
    std::string str;
    int ret = rclient->get(key.c_str());

    if (ret < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS GET(" << key
                   << ") fail: ERROR(" << ret << ").";
    } else {
        char* gr = rclient->getData();
        if (gr != 0) {
            str = std::string(gr);
        }
    }
    return str;
}

int RedisDatabase::delValue(const std::string& key)
{
    lock_t lock(client_mutex_);
    int ret = rclient->del(key.c_str());
    if (ret < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS DEL(" << key
                   << ") fail: ERROR(" << ret << ").";
    }
    return ret;
}

std::vector<std::string>
RedisDatabase::getKeys(const std::string& key_pattern) const
{
    lock_t lock(client_mutex_);
    std::vector<std::string> keys;
    int n = rclient->keys(std::string{key_pattern + "*"}.c_str());
    if (n > 0) {
        auto ret = rclient->getMultiBulkVectorData();
        if (ret > 0) {
            keys = rclient->getVectorData();
        }
    }
    return keys;
}

Json RedisDatabase::getDoc(const char* key) const
{
    Json doc = nullptr;
    std::string str = getValue(key);
    if (str.empty()) {
        LOG(ERROR) << "[RedisDatabase] REDIS GET json("
                   << key << ") fail.";
    } else {
        std::string err;
        doc = Json::parse(str, err);
    }
    return doc;
}

int RedisDatabase::clearDB()
{
    lock_t lock(client_mutex_);
    int ret = rclient->flushall();
    if (ret < 0) {
        LOG(ERROR) << "[RedisDatabase] REDIS FLUSHALL fail.";
    } else {
        LOG(WARNING) << "[RedisDatabase] REDIS FLUSHALL is "
                        "successfully completed!";
    }

    return ret;
}

int RedisDatabase::setupMasterRole()
{
    lock_t lock(client_mutex_);
    int ret = rclient->setMasterRole();
    if (ret > 0) {
        LOG(WARNING) << "[RedisDatabase] Setup Redis-role MASTER("
                     << rclient->getHost() << ":" << rclient->getPort()
                     << ") in standalone mode";
    }
    return ret;
}

int RedisDatabase::setupSlaveOf(const char *address, int port)
{
    lock_t lock(client_mutex_);
    int ret = rclient->setSlaveOf(address, std::to_string(port).c_str());
    if (ret > 0) {
        LOG(WARNING) << "[RedisDatabase] Setup Redis-role SLAVE("
                     << rclient->getHost() << ":" << rclient->getPort()
                     << ") for MASTER(" << address << ":" << port << ")";
        if (int ret = rclient->setSlaveReadOnlyNo() < 0) {
            LOG(ERROR) << "[RedisDatabase] ERROR (" << ret
                       << "): not set 'slave-read-only no'.";
        }
    } else {
        LOG(ERROR) << "[RedisDatabase] ERROR (" << ret << "): not setup SLAVE("
                   << rclient->getHost() << ":" << rclient->getPort()
                   << ") for MASTER(" << address << ":" << port << ")";
    }
    return ret;
}

} // runos
