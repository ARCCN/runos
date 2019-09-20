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

#include "Recovery.hpp"
#include "RestListener.hpp"

#include "Config.hpp"
#include <runos/core/logging.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <string>

namespace runos {

struct RecoveryCollection : rest::resource
{
    RecoveryManager* app;

    explicit RecoveryCollection(RecoveryManager* app)
        : app(app)
    { }

    rest::ptree Get() const override
    {
        rest::ptree root;
        rest::ptree cluster_nodes;

        for (const auto& node : app->cluster())
        {
            rest::ptree ctrl;
            ctrl.put("ID", node->id());
            ctrl.put("state", +ControllerState::ACTIVE == node->state()
                        ? "Active" : "Not Active");
            ctrl.put("is_Iam",
                     node->isThisNodeCurrentController() ? "true" : "false");
            ctrl.put("hb_status",
                    node->hbStatus() == +ControllerStatus::PRIMARY
                        ?  "Primary" : "Backup");

            if (node->role() == 1) {
                ctrl.put("role","EQUAL");
            } else if (node->role() == 2) {
                ctrl.put("role","MASTER");
            } else if (node->role() == 3) {
                ctrl.put("role","SLAVE");
            } else {
                ctrl.put("role","Not defined");
            }

            ctrl.put("of_IP", node->openflowIP());
            ctrl.put("of_port", node->openflowPort());

            ctrl.put("hb_mode", node->hbMode());
            ctrl.put("hb_status",
                     node->hbStatus() == +ControllerStatus::PRIMARY
                        ? "Primary" : "Backup");

            ctrl.put("hb_IP", node->hbIP());
            ctrl.put("hb_port", node->hbPort());

            ctrl.put("hb_interval", node->hbInterval());
            ctrl.put("hb_primaryDeadInterval", node->hbPrimaryDeadInterval());
            ctrl.put("hb_backupDeadInterval", node->hbBackupDeadInterval());

            ctrl.put("is_Connection", node->isConnection() ? "true" : "false");

            ctrl.put("db_IP", node->dbIP());
            ctrl.put("db_port", node->dbPort());
            ctrl.put("db_type", node->dbType());
            ctrl.put("is_dbConnection",
                     node->isDbConnection() ? "true" : "false");

            cluster_nodes.push_back(std::make_pair("", std::move(ctrl)));
        }
        root.add_child("array", cluster_nodes);
        root.put("_size", cluster_nodes.size());

        return root;
    }

    rest::ptree Put(rest::ptree const& pt) override
    {
        rest::ptree ret;
        std::string parseMessage;

        std::ostringstream buffer;
        boost::property_tree::json_parser::write_json(buffer, pt);
        LOG(ERROR) << "[RecoveryManager-REST] "
                   << buffer.str();

        Config obj = Json::parse(buffer.str(),
                                 parseMessage).object_items();

        if (!parseMessage.empty()) {
            LOG(ERROR) << "[RecoveryManager-REST] Can't parse request : "
                       << parseMessage;
            ret.put("error", "Can't parse request");
            ret.put("msg", parseMessage);
        } else {
            if (app->processCommand(obj)) {
                ret.put("res", "ok");
            } else {
                ret.put("error", "Can't change Recevery settings");
            }
        }
        return ret;
    }
};

struct MasterViewCollection : rest::resource
{
    RecoveryManager* app;

    explicit MasterViewCollection(RecoveryManager* app)
        : app(app)
    { }

    rest::ptree Get() const override
    {
        auto ms_view = app->mastershipView();

        rest::ptree root;
        rest::ptree controllers;
        rest::ptree switches;

        for (auto sw : ms_view->view()) {
            std::string str_dpid = std::to_string(sw->getDPID());
            std::string str_role = SwitchView::convertRole(sw->getRole());
            switches.put(str_dpid.c_str(), str_role.c_str());
        }

        std::string str_ctrl_id = std::to_string(app->getID());
        controllers.push_back(std::make_pair(str_ctrl_id.c_str(), std::move(switches)));

        root.add_child("array", controllers);
        root.put("_size", controllers.size());
        return root;
    }
};

struct SwitchesViewCollection : rest::resource
{
    RecoveryManager* app;

    explicit SwitchesViewCollection(RecoveryManager* app)
        : app(app)
    { }

    rest::ptree Get() const override
    {
        auto ms_view = app->mastershipView();

        rest::ptree root;
        rest::ptree sw_view;

        for (auto sw : ms_view->view()) {
            rest::ptree ctrls;
            std::string str_ctrl_id = std::to_string(app->getID());
            std::string str_role = SwitchView::convertRole(sw->getRole());
            ctrls.put(str_ctrl_id.c_str(), str_role.c_str());

            std::string str_dpid = std::to_string(sw->getDPID());
            sw_view.push_back(std::make_pair(str_dpid.c_str(), std::move(ctrls)));
        }
        root.add_child("array", sw_view);
        root.put("_size", sw_view.size());

        return root;
    }
};

class RecoveryRest: public Application
{
    SIMPLE_APPLICATION(RecoveryRest, "recovery-manager-rest")

public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = RecoveryManager::get(loader);
        auto rest_ = RestListener::get(loader);

        rest_->mount(path_spec("/recovery/"), [=](const path_match&)
        {
            return RecoveryCollection {app};
        });

        rest_->mount(path_spec("/masterview/"), [=](const path_match&)
        {
            return MasterViewCollection {app};
        });

        rest_->mount(path_spec("/switchesview/"), [=](const path_match&)
        {
            return SwitchesViewCollection {app};
        });
    }
};

REGISTER_APPLICATION(RecoveryRest, {"rest-listener", "recovery-manager", ""})

}
