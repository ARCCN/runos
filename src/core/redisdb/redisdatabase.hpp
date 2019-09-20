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

#ifndef REDISDATABASE_H
#define REDISDATABASE_H

#include "redisclient.hpp"

#include <mutex>
#include <string>
#include <json11.hpp>

namespace runos {

class RedisDatabase
{
public:
    RedisDatabase();
    ~RedisDatabase() = default;

    int connectToStore(const char* address, int port);
    int auth(const char* pswd);
    bool hasConnection();
    void closeConnection();

    // Methods for individual values
    /*!
     * \brief putValue
     * \param key
     * \param value
     * \return if (ret < 0) then ERROR else the number of receiving bytes
     */
    int putValue(const std::string& key, const std::string& value);

    /*!
     * \brief getValue
     * \param key
     * \return the value that is stored on the key
     */
    std::string getValue(const std::string& key) const;

    /*!
     * \brief delValue
     * \param key
     * \return the number of delete keys in Redis Database
     */
    int delValue(const std::string& key);

     /*!
     * \brief getKeys method to get all keys 
     * \param key_pattern
     * \return vector of keys
     */
    std::vector<std::string> getKeys(const std::string& key_pattern) const;

    /*!
     * \brief get_doc
     * \param key
     * \return JSON Object that is stored on the key
     */
    Json getDoc(const char* key) const;

    /*!
     * \brief clearDB method to delete all keys from Redis Database
     * \return if (ret < 0) then ERROR else the number of deleted keys
     */
    int clearDB();

    // Methods for Redis Database control [Redis version > 3.0]
    /*!
     * \brief setupMasterRole method to setup Master Role for this data store node in Redis cluster (replication mode)
     * \return if (ret < 0) then ERROR else the number of receiving bytes
     */
    int setupMasterRole();

    /*!
     * \brief setupSlaveOf method to setup Slave role for this data store node with respect to Master node (address, port)
     * \param address address of Master Redis node
     * \param port port of Master Redis node
     */
    int setupSlaveOf(const char* address, int port);

private:
    std::unique_ptr<SimpleRedisClient> rclient;
    int connection_state_;
    mutable std::mutex client_mutex_;
};

} // runos

#endif // REDISDATABASE_H
