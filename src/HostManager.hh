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

#pragma once

#include <vector>
#include <mutex>
#include <unordered_map>
#include <time.h>
#include "Controller.hh"
#include "Common.hh"
#include "Application.hh"
#include "Switch.hh"
#include "OFMessageHandler.hh"
#include "Rest.hh"
#include "RestListener.hh"
#include "AppObject.hh"

/**
 * Host object corresponding end host
 * Inherits AppObject for using event model
 */
class Host : public AppObject {
    struct HostImpl* m;
public:
    uint64_t id() const override;
    json11::Json to_json() const override;
    json11::Json formFloodlightJSON();
    std::string mac() const;
    uint64_t switchID() const;
    uint32_t switchPort()const;
    void switchID(uint64_t id);
    void switchPort(uint32_t port);

    Host(std::string mac);
    ~Host();
};

class HostManagerRest: public Rest {
    friend class HostManager;
    class HostManager* m;
public:
    HostManagerRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;
    int count_objects() override;
};

/**
 * HostManager is looking for new hosts in the network.
 * This application connects switch-manager's signal &SwitchManager::switchDiscovered
 * and remember all switches and their mac addresses.
 *
 * Also application handles all packet_ins and if eth_src field is not belongs to switch
 * it means that new end host appears in the network.
 */
class HostManager: public Application, OFMessageHandlerFactory {
    Q_OBJECT
    SIMPLE_APPLICATION(HostManager, "host-manager")
public:
    HostManager();
    ~HostManager();

    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "host-finder"; }
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override
    { return std::unique_ptr<OFMessageHandler>(new Handler(this)); }
    bool isPrereq(const std::string &name) const;
    std::unordered_map<std::string, Host*> hosts();
    Host* getHost(std::string mac);
    Rest* rest() {return dynamic_cast<Rest*>(r); }

public slots:
    void onSwitchDiscovered(Switch* dp);
    void newPort(Switch* dp, of13::Port port);
signals:
    void hostDiscovered(Host* dev);
private:
    struct HostManagerImpl* m;
    std::vector<std::string> switch_macs;
    HostManagerRest* r;
    SwitchManager* m_switch_manager;

    Host* addHost(std::string mac);
    bool findMac(std::string mac);
    bool isSwitch(std::string mac);
    void attachHost(std::string mac, uint64_t id, uint32_t port);

    class Handler: public OFMessageHandler {
    public:
        Handler(HostManager* app_) : app(app_) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    private:
        HostManager* app;
    };
};
