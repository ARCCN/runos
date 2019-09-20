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
#include "json.hpp"

#include "runos/core/exception.hpp"
#include "lib/better_enum.hpp"

#include <list>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace runos {

BETTER_ENUM(SwitchRole, int, UNDEFINED,
                             AR,
                             DR);

using SwitchList = std::list<uint64_t>;
using SwitchListMap = std::unordered_map<int, SwitchList>;
using json = nlohmann::json;

class DpidChecker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(DpidChecker, "dpid-checker")

public:
    struct parse_error : exception_root {
        explicit parse_error(SwitchRole role, json obj) noexcept
            : role_(role)
            , object_(std::move(obj))
        {
            with("role", role._to_string());
            with("object", obj.dump());
        }

        SwitchRole role() const noexcept { return role_; }
        json object() const noexcept { return object_; }

    protected:
        SwitchRole role_;
        json object_;
    };

    void init(Loader* loader, const Config& config) override;
    bool isRegistered(uint64_t dpid,
                      SwitchRole role = SwitchRole::UNDEFINED) const;
    SwitchRole role(uint64_t dpid) const;
    void addSwitch(uint64_t dpid, SwitchRole role);
    void removeSwitch(uint64_t dpid, SwitchRole role);
    SwitchList switchList(SwitchRole role) const;
    json switchJsonList(SwitchRole role);
    void syncSwitchList(const json& switch_list, SwitchRole role);
    void syncSwitchLists(const json& ar_json, const json& dr_json);

    static SwitchList parse(const json& switch_list, SwitchRole role);

signals:
    void switchListChanged(SwitchRole role);
    void switchUnregistered(uint64_t dpid);

private:
    mutable std::mutex switch_list_map_mutex_;
    SwitchListMap switches_;

    const SwitchList& switch_list_ref(SwitchRole role) const;
    SwitchList& switch_list_ref(SwitchRole role);
    SwitchList sync_switch_list_impl(SwitchList switches,
                                     SwitchRole role);
    SwitchList clear_switch_list_impl(SwitchRole role);
    void unregister(const SwitchList& switches, SwitchRole role);
};

}
