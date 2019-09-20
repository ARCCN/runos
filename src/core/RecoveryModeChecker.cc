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

#include "RecoveryModeChecker.hpp"

#include "Recovery.hpp"
#include "DatabaseConnector.hpp"

#include <json.hpp>
#include <algorithm>

namespace runos {

using std::chrono::seconds;

RecoveryModeChecker::RecoveryModeChecker(RecoveryManager* app,
                                         DatabaseConnector* db_connector) :
    checker_(std::make_unique<QTimer>())
  , app_(app)
  , db_connector_(db_connector)
{
    load_from_database();
    connect(checker_.get(), &QTimer::timeout,
            this, &RecoveryModeChecker::checkAllSwitchesConnected);
    connect(app, &RecoveryManager::signalSetupBackupMode, [this]() {
        checker_->stop();
    });
}

void RecoveryModeChecker::load_from_database()
{
    auto switches = db_connector_->getSValue("recovery", "switches");

    if (switches.size() == 0) return;
    auto switches_json = nlohmann::json::parse(switches);

    waiting_for_switches_.reserve(switches_json.size());
    for (const auto& dpid : switches_json) {
        waiting_for_switches_.emplace_back(dpid);
    }
}

// note: should lock RecoveryModeChecker::mut_ before using.
void RecoveryModeChecker::save_to_database()
{
    nlohmann::json switches;
    for (const auto& dpid : connected_switches_) {
        switches.push_back(dpid);
    }

    db_connector_->putSValue("recovery", "switches", switches.dump());
}

void RecoveryModeChecker::startRecoveryCheck(seconds max_waiting_interval)
{
    LOG(ERROR) << "Start recovery on startup process!";
    checker_->start(check_interval_);

    if (max_waiting_interval.count()) {
        expire_time_ = std::chrono::steady_clock::now() + max_waiting_interval;
    } else  {
        expire_time_ = std::chrono::time_point<std::chrono::steady_clock>::max();
    }

}

void RecoveryModeChecker::updateSwitch(uint64_t dpid, Action act)
{
    std::lock_guard<std::mutex> lock(mut_);

    if (act == Action::ADD) {
        connected_switches_.emplace_back(dpid);
    } else if (act == Action::DELETE) {
        connected_switches_.erase(std::remove(connected_switches_.begin(),
                                             connected_switches_.end(),
                                             dpid), connected_switches_.end());
    }

    // save into DB only when we are primary
    if (app_->isPrimary()) {
        save_to_database();
    }
}

void RecoveryModeChecker::checkAllSwitchesConnected()
{
    if (expire_time_ < std::chrono::steady_clock::now()) {
        LOG(WARNING) << "Failed recovery process!";
        checker_->stop();
        save_to_database();
        emit failedRecover();
        return;
    }

    std::vector<uint64_t> lacking_switches = waiting_for_switches_;

    { // lock

    std::lock_guard<std::mutex> lock(mut_);

    for (const auto& dpid : connected_switches_) {
        auto end_vec = waiting_for_switches_.end();
        if (std::find(waiting_for_switches_.begin(), end_vec, dpid) != end_vec) {
            lacking_switches.erase(std::remove(lacking_switches.begin(),
                                               lacking_switches.end(),
                                               dpid), lacking_switches.end());
        }
    }

    } // unlock

    if (lacking_switches.empty()) {
        LOG(WARNING) << "All switches connected!";
        checker_->stop();
        save_to_database();
        emit readyToRecover();
    } else {
        std::ostringstream ss;
        for (auto dpid : lacking_switches) {
            ss << dpid << " ";
        }
        LOG(WARNING) << "Waiting for switch: " << ss.str();
    }
}

} // namespace runos
