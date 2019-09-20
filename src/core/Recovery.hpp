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
#include "SwitchOrdering.hpp"
#include "api/OFAgent.hpp"

#include "hb/heartbeatcore.hpp"
#include "lib/poller.hpp"

#include <fluid/ofcommon/openflow-common.hh>

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace runos {

static constexpr int DEFAULT_PRIMARY_CONTROLLER_ID = -1;
static constexpr uint64_t DEFAULT_DPID = 0;

class RecoveryManager;

struct switch_equal_error : exception_root { };
struct switch_timeout_error : exception_root { };

class SwitchView : public QObject
{
    Q_OBJECT
public:
    explicit SwitchView(SwitchPtr sw,
                        const RecoveryManager* const rm);
    SwitchPtr getSwitch() const;
    uint64_t getDPID() const;
    fluid_msg::ofp_controller_role getRole() const;
    void changeSwitchRole(fluid_msg::ofp_controller_role role,
                          bool get_only_generation_id = false,
                          int role_request_times = -1);
    static std::string convertRole(fluid_msg::ofp_controller_role role);

signals:
    void roleMasterToSlaveChanged(uint64_t dpid);

private:
    void change_switch_role(fluid_msg::ofp_controller_role role,
                          bool get_only_generation_id = false,
                          int role_request_times = -1);
    ofp::role_config send_role_request(fluid_msg::ofp_controller_role role);
    void process_role_reply(fluid_msg::ofp_controller_role role,
                            ofp::role_config&& role_config,
                            bool get_only_generation_id,
                            int role_request_times = -1);
    void handle_equal_error(fluid_msg::ofp_controller_role role, int times);
    void detach_from_invalid_switch(uint64_t dpid);

private:
    SwitchPtr sw_;
    uint64_t generation_id_ = 0;
    fluid_msg::ofp_controller_role role_ = fluid_msg::OFPCR_ROLE_EQUAL;
    const RecoveryManager* const recovery_manager_ = nullptr;
};
using SwitchViewPtr = std::shared_ptr<SwitchView>;


class MastershipView : public Polling
{
    Q_OBJECT
public:
    explicit MastershipView(std::promise<void> init_promise,
                            int role_monitoring_interval);
    ~MastershipView() = default;

    ControllerStatus getStatus() const;
    void setStatus(ControllerStatus status);

    SwitchViewPtr addSwitch(SwitchPtr sw, const RecoveryManager* const rm,
                            int role_monitoring_interval);
    void deleteSwitch(SwitchPtr sw);
    std::vector<SwitchViewPtr> view() const;
    void setupNewRoleForAll(fluid_msg::ofp_controller_role role);

private:
    void polling() override;

private:
    // Be careful - ctrl_status_ can be undefined.
    // It is intentionally to avoid extra role changing at the start.
    ControllerStatus ctrl_status_ = ControllerStatus::UNDEFINED;
    std::vector<SwitchViewPtr> view_;
    std::promise<void> init_promise_;

    bool initialised_ = false;
    mutable std::mutex view_mutex_;
    std::unique_ptr<Poller> role_monitoring_poller_;
};


class ClusterNode
{
public:
    explicit ClusterNode(int id, ControllerState state,
                         fluid_msg::ofp_controller_role role,
                         bool is_this_node_current_controller = false);
    int id() const;
    ControllerState state() const;
    void setState(ControllerState state);
    fluid_msg::ofp_controller_role role() const;
    void setRole(fluid_msg::ofp_controller_role role);
    bool isThisNodeCurrentController() const;
    //openflow
    void setOpenflowAddr(const std::pair<std::string, int>& of_addr);
    std::string openflowIP() const;
    int openflowPort() const;
    // heartbeat
    void setCommunicationType(CommunicationType communication_type);
    void setHbStatus(ControllerStatus status);
    void setHeartbeatConnection(const std::string& hb_address, int hb_port);
    ControllerStatus hbStatus() const;

    std::string hbMode() const;
    std::string hbIP() const;
    int hbPort() const;

    int hbInterval() const;
    int hbPrimaryDeadInterval() const;
    int hbBackupDeadInterval() const;
    int hbPrimaryWaitingInterval() const;

    bool isConnection() const;
    void setConnection(bool connect);

    // data store
    void setDb(std::string IP, int port);
    std::string dbIP() const;
    int dbPort() const;

    void setDbType(std::string type);
    std::string dbType() const;

    bool isDbConnection() const;
    void setDbConnection(bool connect);

private:
    //Controller
    int id_;
    ControllerState state_;
    fluid_msg::ofp_controller_role role_;
    bool is_this_node_current_controller_ = false;

    //OpenFlow
    std::pair<std::string, int> of_addr_ = std::make_pair("0.0.0.0", 0);

    //Heartbeat
    std::string hb_mode_ = "broadcast";
    ControllerStatus hb_status_ = ControllerStatus::UNDEFINED;
    std::string hb_ip_ = "127.0.0.1";
    int hb_port_ = 1234;
    int hb_interval_ = 200;
    int hb_primary_dead_interval_ = 800;
    int hb_backup_dead_interval_ = 1000;
    int hb_primary_waiting_interval_ = 600;
    bool is_connection_ = false;

    //Controller Datastore
    std::string db_type_ = "redis";
    std::string db_ip_ = "127.0.0.1";
    int db_port_ = 6379;
    bool is_db_connection_ = false;
};
using ClusterNodePtr = std::shared_ptr<ClusterNode>;

class DpidChecker;
class RecoveryManager: public Application, public SwitchEventHandler
{
    Q_OBJECT
    SIMPLE_APPLICATION(RecoveryManager, "recovery-manager")
public:
    void init(Loader* provider, const Config& rootConfig) override;
    void startUp(Loader* provider) override;

    void startHeartbeat();
    void stopHeartbeat();
    
    bool isPrimary() const;
    bool isBackup() const;
    std::vector<ClusterNodePtr> cluster() const;
    std::shared_ptr<MastershipView> mastershipView() const;
    int getID() const;
    DpidChecker* dpidChecker() const;

    bool processCommand(Config command);

public slots:
    void recovery();
    void backupDead(int backup_id);
    void connectionToPrimaryEstablished(int primary_id);
    void setupPrimaryMode();
    void setupBackupMode(uint64_t dpid_switch_slaved = DEFAULT_DPID);
    void connectionToBackupControllerEstablished(int backup_id);
    void setParams(const ParamsMessage& params);

signals:
    void signalRecovery();
    void signalSetupPrimaryMode();
    void signalSetupBackupMode();
    void linkBetweenControllersIsDown();

    void toStartHeartbeatService(CommunicationType type,
                                 ControllerStatus status);
    void toStopHeartbeatService();
    void dbParamsChanged(const std::string& db_ip, int db_port);

private:
    void switchUp(SwitchPtr sw) override;
    void switchDown(SwitchPtr sw) override;

    void parseConfig(const Config& root_config);
    void initCurrentNode(const Config& root_config);
    void initHeartbeat(const Config& root_config);
    void initMastership();

    void sendStartHeartbeat();
    void setInitControllerStatus();
    void setFormerPrimaryToBackup();
    void setPrimaryNode(int primary_id);

    bool modifyRole(Config command);
    bool modifyDatabase(Config command);
    bool modifyStatus(Config command);

private:
    ClusterNodePtr current_node_;
    std::vector<ClusterNodePtr> cluster_;
    std::shared_ptr<MastershipView> mastership_view_;
    std::future<void> init_future_; // need to define role
    int id_;
    int primary_controller_id_ = DEFAULT_PRIMARY_CONTROLLER_ID;
    ControllerStatus controller_status_;
    CommunicationType heartbeat_communication_type_ =
            CommunicationType::UNDEFINED;
    std::unique_ptr<HeartbeatCore> heartbeat_core_;
    int master_role_monitoring_interval_;
    std::string heartbeat_address_;
    int heartbeat_port_;

    std::unique_ptr<class RecoveryModeChecker> rm_checker_;
    class DatabaseConnector* db_connector_;
    std::chrono::seconds max_waiting_recovery_interval_;
    DpidChecker* dpid_checker_;
};

} // namespace runos
