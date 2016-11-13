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

/** @file */
#pragma once

#include <memory>
#include <time.h>

#include "Common.hh"
#include "Application.hh"
#include "Controller.hh"
#include "SwitchConnectionFwd.hh"
#include "Rest.hh"
#include "AppObject.hh"
#include "OFTransaction.hh"

using namespace runos;

class SwitchManager;

/**
 * Abstraction of virtual switch.
 */
class Switch: public QObject, AppObject {
Q_OBJECT
public:
    Switch(SwitchManager* mgr,
           SwitchConnectionPtr conn,
           of13::FeaturesReply fr);
    ~Switch();

    /**
     * You may get connection with physical switch by this method,
     * and then you can send OpenFlow message to it
     */
    SwitchConnectionPtr connection() const;

    /**
     * Get identificator of switch in string format
     */
    std::string idstr() const;

    /**
     * get identificator of switch
     */
    uint64_t id() const override;
    uint32_t nbuffers() const;
    /**
     * get count of tables, that switch support
     */
    uint8_t  ntables() const;
    /**
     * get switch capabilites
     */
    uint32_t capabilites() const;

    std::string mfr_desc() const;

    /**
     * get hardware description of switch
     */
    std::string hw_desc() const;
    std::string sw_desc() const;
    std::string serial_number() const;
    std::string dp_desc() const;

    /**
     * get information about port by its number
     */
    of13::Port port(uint32_t port_no) const;

    /**
     * get information about all ports
     */
    std::vector<of13::Port> ports() const;

    /**
     * send to switch Port Description request
     */
    void requestPortDescriptions();

    /**
     * send to switch request about its description
     */
    void requestSwitchDescriptions();
    json11::Json to_json() const override;
    json11::Json to_floodlight_json() const;

signals:

    /**
     * port up on switch
     */
    void portUp(Switch* dp, of13::Port port);

    /**
     * port modified on switch
     */
    void portModified(Switch* dp, of13::Port port, of13::Port oldPort);

    /**
     * port down on switch
     */
    void portDown(Switch* dp, uint32_t port_no);

    /**
     * switch was up
     */
    void up(Switch* dp);

    /**
     * switch downed
     */
    void down(Switch* dp);

protected:
    void portStatus(of13::PortStatus ps);
    void portDescArrived(of13::MultipartReplyPortDescription& mrpd);
    void setUp(SwitchConnectionPtr conn, of13::FeaturesReply fr);
    void setDown();

private:
    friend class SwitchManager;
    friend class SwitchManagerRest;
    std::unique_ptr<struct SwitchImpl> m;
};

/**
 * You may use this application for manage switchs, which connected to controller.
 *
 * You can get information about switchs connected to controller by GET request : GET /api/switch-manager/switches/all
 *
 * Application support event model.
 */

class SwitchManager: public Application, RestHandler  {
Q_OBJECT
SIMPLE_APPLICATION(SwitchManager, "switch-manager")
public:
    SwitchManager();
    ~SwitchManager();

    // rest
    bool eventable() override {return true;}
    std::string displayedName() override { return "Switch Manager"; }
    std::string page() override { return "switch.html"; }
    AppType type() override { return AppType::Application; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;

    void init(Loader* provider, const Config& config) override;

    /**
     * You can get abstaction of switch by its datapath id
     * @param dpid of switch
     * @return switch abstaction
     */
    Switch* getSwitch(uint64_t dpid);
    /**
     * Get all switchs, connected to controller
     */
    std::vector<Switch*> switches();

signals:

    /**
     * Application discovered new switch in network
     *
     * Unlike switchUp signal, this signal emitted providing new switch appear in network.
     *
     * @param dp abstraction of this switch
     */
    void switchDiscovered(Switch* dp);

    /**
     * Application set up switch
     *
     * Unlike switchDiscovered signal, this signal emitted when controller setup connection with switch.
     * It may be new switch, or switch which connection was broken and setup now.
     *
     * @param dp abstraction of this switch
     */
    void switchUp(Switch* dp);

    /**
     * Switch disappeared in network or connection with its closed or failed
     * @param dp abstraction of this switch
     */
    void switchDown(Switch* dp);

protected slots:
    void onSwitchUp(SwitchConnectionPtr conn, of13::FeaturesReply fr);
    void onPortStatus(SwitchConnectionPtr conn, of13::PortStatus ps);
    void onSwitchDown(SwitchConnectionPtr conn);
    void onPortDescriptions(SwitchConnectionPtr conn,
                            std::shared_ptr<OFMsgUnion> msg);
    void onSwitchDescriptions(SwitchConnectionPtr ofconn, std::shared_ptr<OFMsgUnion> msg);

private:
    friend class Switch;
    std::unique_ptr<struct SwitchManagerImpl> m;
};
