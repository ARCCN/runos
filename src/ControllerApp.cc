/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include "ControllerApp.hh"

REGISTER_APPLICATION(ControllerApp, {"topology", "switch-manager", "host-manager", "rest-listener", ""})

/**
 * Information about controller's starting parameters
 */
struct ControllerInfo {
    std::string address;
    int port;
    int nthreads;
    bool secure;
    time_t started;

    JSONparser formJSON();
};

ControllerApp::ControllerApp()
{
    r = new ControllerRest("Controller App", "none");
    r->ctrl = this;
    info = new ControllerInfo;
}

ControllerApp::~ControllerApp()
{
    delete r;
    delete info;
}

void ControllerApp::getInfo(std::string addr, int port, int nthreads, bool secure)
{
    info->address = addr;
    info->port = port;
    info->nthreads = nthreads;
    info->secure = secure;
    info->started = time(NULL);
}

void ControllerApp::init(Loader *loader, const Config &config)
{
    ctrl = Controller::get(loader);
    connect(ctrl, &Controller::sendInfo, this, &ControllerApp::getInfo);

    r->addApp("switch-manager", SwitchManager::get(loader)->rest());
    r->addApp("topology", Topology::get(loader)->rest());
    r->addApp("host-manager", HostManager::get(loader)->rest());

    RestListener::get(loader)->newListener("controller-manager", r);
}

std::string ControllerRest::handle(std::vector<std::string> params)
{
    if (params[0] != "GET")
        return "{ \"error\": \"error\" }";

    if (params[4] == "summary") {
        JSONparser res;
        if (apps_list.count("switch-manager"))
            res.addValue("# Switches", apps_list["switch-manager"]->count_objects());
        if (apps_list.count("host-manager"))
            res.addValue("# hosts", apps_list["host-manager"]->count_objects());
        res.addValue("# quarantine ports", 0);
        if (apps_list.count("topology"))
            res.addValue("# inter-switch links", apps_list["topology"]->count_objects());
        return res.get();
    }
    if (params[2] == "info") {
        return ctrl->info->formJSON().get();
    }

    if (params[3] == "health") {
        return "Not used";
    }

    if (params[3] == "memory") {
        return "Not used";
    }

    if (params[3] == "system") {
        return "Not used";
    }
}

JSONparser ControllerInfo::formJSON()
{
    JSONparser json;
    json.addValue("address", address);
    json.addValue("port", port);
    json.addValue("nthreads", nthreads);
    json.addValue("secure", secure);
    json.addValue("started", static_cast<uint64_t>(started));
    return json;
}
