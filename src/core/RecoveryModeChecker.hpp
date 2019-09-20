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

#include <QObject>
#include <QTimer>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

namespace runos {

class RecoveryManager;
class DatabaseConnector;

enum class Action {
    ADD, DELETE
};

class RecoveryModeChecker : public QObject
{
    Q_OBJECT
public:
    RecoveryModeChecker(RecoveryManager* app, DatabaseConnector* db_connector);

    void startRecoveryCheck(std::chrono::seconds max_waiting_interval);
    void updateSwitch(uint64_t dpid, Action act);

signals:
    void readyToRecover();
    void failedRecover();

private:
    void load_from_database();
    void save_to_database();

    std::mutex mut_; // protect connected_switches_
    std::unique_ptr<QTimer> checker_;
    RecoveryManager* app_;
    DatabaseConnector* db_connector_;
    std::vector<uint64_t> waiting_for_switches_;
    std::vector<uint64_t> connected_switches_;

    std::chrono::time_point<std::chrono::steady_clock> expire_time_;
    static constexpr int check_interval_ = 1000;

private slots:
    void checkAllSwitchesConnected();
};

} // namespace runos
