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
#include "RestListener.hh"
#include "Event.hh"
#include "AppObject.hh"

class SwitchManager;

class Switch: public QObject, AppObject {
Q_OBJECT
public:
    Switch(SwitchManager* mgr, OFConnection* conn, of13::FeaturesReply fr);
    ~Switch();

    std::string idstr() const;
    uint64_t id() const;
    uint32_t nbuffers() const;
    uint8_t  ntables() const;
    uint32_t capabilites() const;

    of13::Port port(uint32_t port_no) const;
    std::vector<of13::Port> ports() const;

    void send(OFMsg* msg);
    void requestPortDescriptions();
    JSONparser formJSON() override;
    JSONparser formFloodlightJSON();

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

/**
 * REST interface for SwitchManager
 */
class SwitchManagerRest : public Rest {
    Q_OBJECT
    friend class SwitchManager;
    class SwitchManager* m;
public:
    SwitchManagerRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;
    int count_objects() override;
protected slots:
    void onSwitchDiscovered(Switch* dp);
    void onSwitchDown(Switch* dp);
};

class SwitchManager: public Application  {
Q_OBJECT
SIMPLE_APPLICATION(SwitchManager, "switch-manager")
public:
    SwitchManager();
    ~SwitchManager();

    void init(Loader* provider, const Config& config) override;

    Switch* getSwitch(OFConnection* ofconn) const;
    Switch* getSwitch(uint64_t dpid);
    std::vector<Switch*> switches();
    Rest* rest() {return dynamic_cast<Rest*>(r); }

signals:
    void switchDiscovered(Switch* dp);
    void switchDown(Switch* dp);

protected slots:
    void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);
    void onPortStatus(OFConnection* ofconn, of13::PortStatus ps);
    void onSwitchDown(OFConnection* ofconn);
    void onPortDescriptions(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> msg);

private:
    friend class Switch;
    struct SwitchManagerImpl* m;
    SwitchManagerRest* r;
};
