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

#include "RestListener.hpp"
#include "Recovery.hpp"
#include "DatabaseConnector.hpp"

#include <json.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <sstream>
#include <vector>

namespace runos {

using json = nlohmann::json;

struct AuxDevice {
    std::string name;
    std::string desc;
    std::string icon;

    uint64_t sw_dpid;
    uint32_t sw_port;

    json to_json() const {
        json ret = {
            {"name", name},
            {"desc", desc},
            {"icon", icon},
            {"sw_dpid", sw_dpid},
            {"sw_port", sw_port}
        };

        return ret;
    }
};

class AuxDevicesRest : public Application
{
    SIMPLE_APPLICATION(AuxDevicesRest, "aux-devices-rest")
public:
    void init(Loader* loader, const Config&) override;
    void startUp(Loader* loader) override;
    void timerEvent(QTimerEvent *event) override;
    bool addDevice(const json& dev);
    bool delDevice(const std::string& name);

    void save_to_database();
    void load_from_database();

    std::vector<AuxDevice> devices;
    RecoveryManager* recovery;
    DatabaseConnector* db_connector_;
};
REGISTER_APPLICATION(AuxDevicesRest, {"rest-listener", "recovery-manager",
                                      "database-connector", ""})

/*** Rest handlers ***/

struct AuxDeviceCollection : rest::resource {
    AuxDevicesRest* app;

    explicit AuxDeviceCollection(AuxDevicesRest* app)
        : app(app) {}

    rest::ptree Get() const override {
        rest::ptree ret, devices;

        for (const auto& dev : app->devices) {
            rest::ptree tmp;
            std::stringstream ss;
            ss << dev.to_json();
            boost::property_tree::json_parser::read_json(ss, tmp);
            devices.push_back(std::make_pair("", tmp));
        }
        ret.add_child("array", devices);
        ret.put("_size", devices.size());

        return ret;
    }
};

struct AuxDeviceResource : rest::resource {
    AuxDevicesRest* app;
    std::string name;

    explicit AuxDeviceResource(AuxDevicesRest* app, std::string name)
        : app(app), name(name) {}

    rest::ptree Get() const override {
        return rest::ptree();
    }

    rest::ptree Put(const rest::ptree& pt) override {
        using namespace boost::property_tree::json_parser;
        rest::ptree ret;
        json device;
        try {
            std::ostringstream buffer;
            write_json(buffer, pt);
            device = json::parse(buffer.str());
        } catch (const json_parser_error& ex) {
            THROW(rest::http_error(400), "Can't parse request");
        }

        if (std::find_if(app->devices.begin(), app->devices.end(),
            [name = name](const auto& dev) {
                if (dev.name == name) return true;
                return false;
        }) != app->devices.end()) {
            THROW(rest::http_error(400), "Device is already exists");
        }

        device["name"] = name;
        auto res = app->addDevice(device);
        if (res) {
            ret.put("res", "Device added");
        } else {
            ret.put("error", true);
            ret.put("res", "Can't create device");
        }

        return ret;
    }

    rest::ptree Delete() override {
        rest::ptree ret;

        auto res = app->delDevice(name);
        if (res) {
            ret.put("res", "Device removed");
        } else {
            ret.put("error", true);
            ret.put("res", "Can't remove device");
        }

        return ret;
    }
};

/*** AuxDevicesRest methods ***/

void AuxDevicesRest::init(Loader* loader, const Config&)
{
    using rest::path_spec;
    using rest::path_match;

    auto rest_ = RestListener::get(loader);
    recovery = RecoveryManager::get(loader);
    db_connector_ = DatabaseConnector::get(loader);

    rest_->mount(path_spec("/aux-devices/"), [=](const path_match& m) {
        return AuxDeviceCollection(this);
    });
    rest_->mount(path_spec("/aux-devices/(\\S+)/"), [=](const path_match& m) {
        std::string name = m[1].str();
        return AuxDeviceResource(this, name);
    });
}

void AuxDevicesRest::startUp(Loader* loader)
{
    load_from_database();
    startTimer(5 * 1000);
}

void AuxDevicesRest::timerEvent(QTimerEvent* event)
{
    if (not recovery->isPrimary()) {
        load_from_database();
    }
}

bool AuxDevicesRest::addDevice(const json& dev)
{
    if (not recovery->isPrimary()) return false;

    if (!dev.count("name") || !dev.count("desc") || !dev.count("icon")
        || !dev.count("sw_dpid") || !dev.count("sw_port"))
        return false;
    
    std::string dpid = dev["sw_dpid"];
    std::string port = dev["sw_port"];
    AuxDevice new_dev;
    new_dev.name = dev["name"];
    new_dev.desc = dev["desc"];
    new_dev.icon = dev["icon"];
    new_dev.sw_dpid = boost::lexical_cast<uint64_t>(dpid);
    new_dev.sw_port = boost::lexical_cast<uint32_t>(port);

    devices.push_back(new_dev);
    save_to_database();
    return true;
}

bool AuxDevicesRest::delDevice(const std::string& name)
{
    if (not recovery->isPrimary()) return false;

    auto found = std::find_if(devices.begin(), devices.end(),
        [name](const auto& dev) {
            if (dev.name == name) return true;
            return false;
    });

    if (found != devices.end()) {
        devices.erase(found);
        save_to_database();
        return true;
    }

    return false;
}

void AuxDevicesRest::save_to_database()
{
    auto jdump = json::array();
    for (const auto& d : devices) {
        jdump.push_back(d.to_json());
    }
    db_connector_->putSValue("aux-devices-rest", "devices", jdump.dump());
}

void AuxDevicesRest::load_from_database()
{
    devices.clear();
    auto devs = db_connector_->getSValue("aux-devices-rest", "devices");
    if (devs.size()) {
        auto devs_json = json::parse(devs);
        for (const auto& d : devs_json) {
            AuxDevice new_dev;
            new_dev.name = d["name"];
            new_dev.desc = d["desc"];
            new_dev.icon = d["icon"];
            new_dev.sw_dpid = d["sw_dpid"];
            new_dev.sw_port = d["sw_port"];
            devices.push_back(new_dev);
        }
    }
}

} //namespace runos
