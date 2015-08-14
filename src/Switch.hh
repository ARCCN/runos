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

#include <time.h>

#include "Common.hh"
#include "Application.hh"
#include "Controller.hh"
#include "Rest.hh"
#include "AppObject.hh"

class SwitchManager;

class Switch: public QObject, AppObject {
Q_OBJECT
public:
    Switch(SwitchManager* mgr, OFConnection* conn, of13::FeaturesReply fr);
    ~Switch();

    std::string idstr() const;
    uint64_t id() const override;
    uint32_t nbuffers() const;
    uint8_t  ntables() const;
    uint32_t capabilites() const;
    OFConnection* ofconn() const;

    std::string mfr_desc() const;
    std::string hw_desc() const;
    std::string sw_desc() const;
    std::string serial_number() const;
    std::string dp_desc() const;

    of13::Port port(uint32_t port_no) const;
    std::vector<of13::Port> ports() const;

    void send(OFMsg* msg);
    void requestPortDescriptions();
    void requestSwitchDescriptions();
    json11::Json to_json() const;
    json11::Json to_floodlight_json() const;

signals:
    void portUp(Switch* dp, of13::Port port);
    void portModified(Switch* dp, of13::Port port, of13::Port oldPort);
    void portDown(Switch* dp, uint32_t port_no);

    void up(Switch* dp);
    void down(Switch* dp);

protected:
    void portStatus(of13::PortStatus ps);
    void portDescArrived(of13::MultipartReplyPortDescription& mrpd);
    void setUp(OFConnection* conn, of13::FeaturesReply fr);
    void setDown();

private:
    friend class SwitchManager;
    friend class SwitchManagerRest;
    struct SwitchImpl* m;
};

class SwitchManager: public Application, RestHandler  {
Q_OBJECT
SIMPLE_APPLICATION(SwitchManager, "switch-manager")
public:
    SwitchManager();
    ~SwitchManager();

    std::string restName() override {return "switch-manager";}
    bool eventable() override {return true;}
    std::string displayedName() override { return "Switch Manager"; }
    std::string page() override { return "switch.html"; }
    AppType type() override { return AppType::Application; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;

    void init(Loader* provider, const Config& config) override;

    Switch* getSwitch(OFConnection* ofconn) const;
    Switch* getSwitch(uint64_t dpid);
    std::vector<Switch*> switches();

signals:
    void switchDiscovered(Switch* dp);
    void switchDown(Switch* dp);

protected slots:
    void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);
    void onPortStatus(OFConnection* ofconn, of13::PortStatus ps);
    void onSwitchDown(OFConnection* ofconn);
    void onPortDescriptions(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> msg);
    void onSwitchDescriptions(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> msg);

private:
    friend class Switch;
    struct SwitchManagerImpl* m;
};
