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

#include "heartbeatprotocol.hpp"
#include "../Config.hpp"
#include <memory>

class HeartbeatCore : public QObject
{
    Q_OBJECT
public:
    explicit HeartbeatCore(const runos::Config& root_config);
    ~HeartbeatCore();

public slots:
    void startService(CommunicationType type, ControllerStatus status);
    void stopService();
    void linkDown();
    void dbParamsChanged(const std::string& db_ip, int db_port);

signals:
    void primaryDied();
    void backupDied(int backup_id);
    void connectionToPrimaryEstablished(int primary_id);
    void connectionToBackupEstablished(int backup_id);
    void modeChangedToPrimary();
    void paramsChanged(const ParamsMessage& params);

private slots:
    void send_request();
    void primary_death();
    void check_primary();
    void backup_death(int backup_id);
    void ready_read_handler();

private:
    void start_receiving(CommunicationType communication_type);
    void start_transmitting(CommunicationType communication_type);

    void init_config(const runos::Config& root_config);
    void bind_socket(CommunicationType communication_type);
    void prepare_connection(CommunicationType type, ControllerStatus status);
    void check_backup_timer(int backup_id);

    void process_message_request(QDataStream& stream);
    void process_message_reply(QDataStream& stream);
    void process_message_params(QDataStream& stream);

    struct implementation;
    std::unique_ptr<implementation> impl_;
};
