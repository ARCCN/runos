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

#include "DpidChecker.hpp"

#include "runos/core/assert.hpp"
#include "runos/core/logging.hpp"
#include "runos/core/config.hpp"
#include "runos/core/throw.hpp"

#include <boost/lexical_cast.hpp>

#include <sstream>

namespace runos {

REGISTER_APPLICATION(DpidChecker, {""})

using lock_t = std::lock_guard<std::mutex>;

void DpidChecker::init(Loader *loader, const Config& rootConfig)
{
    // Scan switch type in network-settings
    auto config = config_cd(rootConfig, "dpid-checker");
    std::string format = config_get(config, "dpid-format", "dec");
    if (format != "dec" && format != "hex") {
        LOG(FATAL) << "[DpidChecker] Reading from network-setting.json - "
                  "Incorrect 'format' value in 'dpid-checker' in settings file: "
                   << format;
    }

    switches_.emplace((+SwitchRole::AR)._to_integral(), SwitchList());
    switches_.emplace((+SwitchRole::DR)._to_integral(), SwitchList());

    auto scan_switches = [&config, &format, this] (SwitchRole role)
    {
        const auto type = role._to_string();

        if (config.find(type) != config.end()) {
            for (auto sw : config.at(type).array_items()) {
                std::stringstream ss(sw.string_value());
                uint64_t dpid = strtoull(sw.string_value().c_str(),
                                         NULL,
                                         (format == "hex" ? 16 : 10));
                if (dpid == 0) {
                    LOG(ERROR) << "[DpidChecker] Incorrect DPID: "
                               << sw.string_value();
                } else {
                    lock_t l(switch_list_map_mutex_);
                    auto& switch_list = switch_list_ref(role);
                    switch_list.push_back(dpid);
                }
            }
        }
    };

    scan_switches(SwitchRole::DR);
    scan_switches(SwitchRole::AR);
}

bool DpidChecker::isRegistered(uint64_t dpid, SwitchRole role) const
{
    auto find_lambda = [this, dpid](const SwitchList& list)
    {
        lock_t l(switch_list_map_mutex_);
        return std::find(list.begin(), list.end(), dpid) != list.end();
    };

    if (+SwitchRole::UNDEFINED == role) {
        return find_lambda(switch_list_ref(SwitchRole::AR)) or
               find_lambda(switch_list_ref(SwitchRole::DR));
    }
    return find_lambda(switch_list_ref(role));
}

SwitchRole DpidChecker::role(uint64_t dpid) const
{
    if (this->isRegistered(dpid, SwitchRole::AR)) {
        return SwitchRole::AR;
    } else if (this->isRegistered(dpid, SwitchRole::DR)) {
        return SwitchRole::DR;
    } else {
        return SwitchRole::UNDEFINED;
    }
}

void DpidChecker::addSwitch(uint64_t dpid, SwitchRole role)
{
    CHECK(+SwitchRole::UNDEFINED != role);

    if (this->isRegistered(dpid)) {
        LOG(WARNING) << "[DpidChecker] Dpid=" << dpid
                     <<  " is already registered";
        return;
    }

    { // lock
    lock_t l(switch_list_map_mutex_);
    auto& list = switch_list_ref(role);
    list.push_back(dpid);
    VLOG(30) << "[DpidChecker] Dpid=" << dpid
             << " was successfully added to " << role._to_string()
             << " list. Size=" << list.size();
    } // unlock

    emit this->switchListChanged(role);
}

void DpidChecker::removeSwitch(uint64_t dpid, SwitchRole role)
{
    CHECK(+SwitchRole::UNDEFINED != role);

    { // lock
    lock_t l(switch_list_map_mutex_);
    auto& list = switch_list_ref(role);
    auto it = std::find(list.begin(), list.end(), dpid);
    if (list.end() != it) {
        list.erase(it);
    } else {
        return;
    }
    } // unlock

    emit this->switchUnregistered(dpid);
    emit this->switchListChanged(role);
}

SwitchList DpidChecker::switchList(SwitchRole role) const
{
    CHECK(+SwitchRole::UNDEFINED != role);

    lock_t l(switch_list_map_mutex_);
    return switch_list_ref(role);
}

json DpidChecker::switchJsonList(SwitchRole role)
{
    CHECK(+SwitchRole::UNDEFINED != role);

    auto list_json = json::array();
    { // lock
    lock_t l(switch_list_map_mutex_);
    const auto& switch_list = switch_list_ref(role);
    for (auto& sw : switch_list) {
        list_json.push_back(std::to_string(sw));
    }
    } // unlock
    return list_json;
}

void DpidChecker::syncSwitchList(const json& switch_list, SwitchRole role)
{
    CHECK(+SwitchRole::UNDEFINED != role);

    VLOG(30) << "[DpidChecker] Switch list syncronization. For role="
             << role._to_string()
             << " input list=" << switch_list.dump();

    if (switch_list.empty()) return;

    auto&& new_list = parse(switch_list, role);
    SwitchList removed_switches;

    { // lock
    lock_t l(switch_list_map_mutex_);
    removed_switches = sync_switch_list_impl(new_list, role);
    } // unlock

    emit switchListChanged(role);
    unregister(removed_switches, role);
}

void DpidChecker::syncSwitchLists(const json& ar_json, const json& dr_json)
{
    VLOG(30) << "[DpidChecker] Switch list syncronization."
             << " AR list: " << ar_json.dump()
             << " DR list: " << dr_json.dump();

    auto&& ar_list = parse(ar_json, SwitchRole::AR);
    auto&& dr_list = parse(dr_json, SwitchRole::DR);

    SwitchList removed_ar;
    SwitchList removed_dr;

    { // lock
    lock_t l(switch_list_map_mutex_);
    removed_ar = sync_switch_list_impl(ar_list, SwitchRole::AR);
    removed_dr = sync_switch_list_impl(dr_list, SwitchRole::DR);
    } // unlock

    emit switchListChanged(SwitchRole::AR);
    emit switchListChanged(SwitchRole::DR);

    unregister(removed_ar, SwitchRole::AR);
    unregister(removed_dr, SwitchRole::DR);

    VLOG(20) << "[DpidChecker] Removed switches unregistered";
}

SwitchList DpidChecker::parse(const json& switch_list, SwitchRole role)
{
    using boost::lexical_cast;

    SwitchList parsed;
    for (auto& sw : switch_list) {
        try {
            auto sw_dpid = lexical_cast<uint64_t>(sw.get<std::string>());
            parsed.push_back(sw_dpid);
        } catch (const boost::bad_lexical_cast& e) {
            THROW(parse_error(role, sw), "Bad lexical cast");
        } catch (const std::domain_error& e) {
            THROW(parse_error(role, sw), "Can't parse dpid as string");
        }
    }
    return parsed;
}

const SwitchList& DpidChecker::switch_list_ref(SwitchRole role) const
{
    CHECK(+SwitchRole::UNDEFINED != role);
    auto role_key = role._to_integral();
    return switches_.at(role_key);
}

SwitchList& DpidChecker::switch_list_ref(SwitchRole role)
{
    CHECK(+SwitchRole::UNDEFINED != role);
    auto role_key = role._to_integral();
    return switches_.at(role_key);
}

SwitchList DpidChecker::sync_switch_list_impl(SwitchList switches,
                                              SwitchRole role)
{
    SwitchList removed_switches;
    auto& current_list = switch_list_ref(role);
    for (auto& sw : current_list) {
        auto sw_iter = std::find(switches.begin(), switches.end(), sw);
        if (switches.end() == sw_iter) {
            // new list doesn't have an old dpid - disconnect from this
            removed_switches.push_back(sw);
        }
    }
    current_list = std::move(switches);
    return removed_switches;
}

void DpidChecker::unregister(const SwitchList& switches, SwitchRole role)
{
    for (auto& dpid : switches) {
        emit this->switchUnregistered(dpid);
    }
    if (!switches.empty()) {
        emit this->switchListChanged(role);
    }
}

}
