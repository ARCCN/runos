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

#include "DatabaseConnector.hpp"
#include "RecoveryModeChecker.hpp"
#include "DpidChecker.hpp"
#include "api/OFAgent.hpp"

#include <json.hpp>
#include <runos/core/logging.hpp>

#include <QThread>
#include <stdexcept>

static constexpr int UNICAST_PRIMARY_ID = 1;
static constexpr int UNICAST_BACKUP_ID = 2;
// Max times we make ROLE_REQUEST if switch returns EQUAL before disconnect
static constexpr int MAX_TIMES_MEET_EQUAL = 1;

static constexpr auto ROLE_REQUEST_TIMEOUT = boost::chrono::seconds(5);

namespace runos
{
using lock_t = std::lock_guard<std::mutex>;

REGISTER_APPLICATION(RecoveryManager, {"switch-ordering", "database-connector",
                                       "dpid-checker", ""})

SwitchView::SwitchView(SwitchPtr sw,
                       const RecoveryManager* const rm)
    : QObject(), sw_(sw), recovery_manager_(rm)
{
    QObject::connect(this, &SwitchView::roleMasterToSlaveChanged,
                     rm, &RecoveryManager::setupBackupMode);
}

uint64_t SwitchView::getDPID() const {
    return sw_->dpid();
}

fluid_msg::ofp_controller_role SwitchView::getRole() const
{
    return role_;
}

void SwitchView::changeSwitchRole(fluid_msg::ofp_controller_role sending_role,
                                  bool get_only_generation_id,
                                  int role_request_times)
{
    CHECK(fluid_msg::OFPCR_ROLE_EQUAL != sending_role);
    try {
        change_switch_role(sending_role,
                           get_only_generation_id,
                           role_request_times);
    } catch (const switch_equal_error& e) {
        LOG(ERROR) << "[RecoveryManager] Mastership view - "
                   << "Switch with dpid=" << getDPID()
                   << " returns EQUAL role. Disconnecting from the bad switch";
        detach_from_invalid_switch(getDPID());
    } catch (const switch_timeout_error& e) {
        LOG(ERROR) << "[RecoveryManager] Role request time exceeded. Dpid="
                   << sw_->dpid() << ", role="
                   << SwitchView::convertRole(sending_role);
    } catch (const OFAgent::openflow_error& e) {
        LOG(ERROR) << "[RecoveryManager] openflow_error during role request."
                   << "Switch dpid=" << e.dpid() << ", msg xid=" << e.xid()
                   << ", type=" << e.type() << ", code=" << e.code();
    } catch (const OFAgent::request_error& e) {
        LOG(ERROR) << "[RecoveryManager] request_error during role request."
                   << "Switch dpid=" << e.dpid() << ", msg xid=" << e.xid();
    } catch (...) {
        LOG(ERROR) << "[RecoveryManager] Undefined error in changeSwitchRole";
    }
}

void SwitchView::change_switch_role(fluid_msg::ofp_controller_role sending_role,
                                    bool get_only_generation_id,
                                    int role_request_times)
{
    auto ret = send_role_request(sending_role);
    const auto ret_role = static_cast<fluid_msg::ofp_controller_role>(ret.role);
    CHECK(fluid_msg::OFPCR_ROLE_NOCHANGE!= ret_role);
    if (fluid_msg::OFPCR_ROLE_NOCHANGE != sending_role &&
        ret_role != sending_role && role_request_times == -1) {
        LOG(ERROR) << "[RecoveryManager] Mastership -"
                   << " New role=" << SwitchView::convertRole(sending_role)
                   << " wasn't set. Role=" << SwitchView::convertRole(ret_role)
                   << " was recieved from switch";
    }
    process_role_reply(sending_role, std::move(ret),
                       get_only_generation_id, role_request_times);

}

ofp::role_config
SwitchView::send_role_request(fluid_msg::ofp_controller_role sending_role)
{
    auto agent = sw_->connection()->agent();
    // request_role produces OFAgent::request_error,
    // but it is caught higher inside changeSwitchRole
    auto f = agent->request_role(static_cast<uint32_t>(sending_role),
                                 generation_id_);
    if (f.wait_for(ROLE_REQUEST_TIMEOUT) == boost::future_status::timeout) {
        THROW(switch_timeout_error{}, "Switch role request timeout exceeded");
    }
    return f.get();
}

void SwitchView::process_role_reply(fluid_msg::ofp_controller_role sending_role,
                                    ofp::role_config&& role_config,
                                    bool get_only_generation_id,
                                    int role_request_times)
{
    generation_id_ = role_config.generation_id;
    if (get_only_generation_id) {
        return;
    }

    const auto returned_role =
            static_cast<fluid_msg::ofp_controller_role>(role_config.role);

    // message exchange order between controller (c) and switch (s)
    // 1. c ---> s  - to get_generation_id with NOCHANGE_ROLE
    // 2. c ---> s  - to set role (MASTER/SLAVE)
    // 3. c ---> s  - NOCHANGE_ROLE every timeout (default timeout=1s)

    // Handling of situation unsupported by openflow switch specification
    // Let's suppose we are EQUAL after start, and switch is after start
    // (role is EQUAL) and we are sending role
    // (after initial ROLE_NOCHANGE to get generation_id
    if (fluid_msg::OFPCR_ROLE_EQUAL == role_ &&
            fluid_msg::OFPCR_ROLE_EQUAL == returned_role) {
        if (fluid_msg::OFPCR_ROLE_NOCHANGE == sending_role) {
            THROW(switch_equal_error{}, "Switch cannot be changed from EQUAL");
        } else { // sending MASTER/SLAVE
            handle_equal_error(sending_role, role_request_times);
        }
    }

    if (returned_role != role_) {
        switch (returned_role) {
        case fluid_msg::OFPCR_ROLE_MASTER:
            break;
        case fluid_msg::OFPCR_ROLE_SLAVE:
            // if only one slave -> make all slaves (splitbrain)
            emit roleMasterToSlaveChanged(sw_->dpid());
            break;
        case fluid_msg::OFPCR_ROLE_EQUAL:
            switch (role_) {
            case fluid_msg::OFPCR_ROLE_MASTER:
            case fluid_msg::OFPCR_ROLE_SLAVE:
                handle_equal_error(sending_role, role_request_times);
                break;
            default:
                break;
            }
            break;
        default:
            CHECK(false);
        }
        LOG(INFO) << "[RecoveryManager] Mastership view - "
                  << "For switch with DPID=" << sw_->dpid()
                  << " role has changed from "
                  << SwitchView::convertRole(role_)
                  << " to " << SwitchView::convertRole(returned_role);
        role_ = returned_role;
    }
}

void
SwitchView::handle_equal_error(fluid_msg::ofp_controller_role sending_role,
                               int role_request_times)
{
    // Handler for error if switch set EQUAL role
    // Problem - if controller fails to set role distinct from EQUAL
    // or switch fails and set EQUAL itself
    int times = (role_request_times == -1) ? 0 : ++role_request_times;
    if (times >= MAX_TIMES_MEET_EQUAL) {
        THROW(switch_equal_error{}, "Switch cannot be changed from EQUAL");
    }
    change_switch_role(sending_role, false, times);
}

void SwitchView::detach_from_invalid_switch(uint64_t dpid)
{
    recovery_manager_->dpidChecker()->removeSwitch(
        getDPID(),
        recovery_manager_->dpidChecker()->role(dpid)
    );
}

std::string SwitchView::convertRole(fluid_msg::ofp_controller_role role)
{
    std::string str;
    switch (role) {
    case fluid_msg::OFPCR_ROLE_NOCHANGE:
        str = "NOCHANGE";
        break;
    case fluid_msg::OFPCR_ROLE_EQUAL:
        str = "EQUAL";
        break;
    case fluid_msg::OFPCR_ROLE_MASTER:
        str = "MASTER";
        break;
    case fluid_msg::OFPCR_ROLE_SLAVE:
        str = "SLAVE";
        break;
    default:
        str = "UNDEFINED";
        break;
    }
    return str;
}

// MastershipView class

MastershipView::MastershipView(std::promise<void> init_promise,
                               int role_monitoring_interval)
    : init_promise_(std::move(init_promise))
    , role_monitoring_poller_(new Poller(this, role_monitoring_interval))
{}

ControllerStatus MastershipView::getStatus() const
{
    return ctrl_status_;
}

SwitchViewPtr MastershipView::addSwitch(SwitchPtr sw,
                                        const RecoveryManager* const rm,
                                        int role_monitoring_interval)
{
    auto switch_view = std::make_shared<SwitchView>(sw, rm);

    {
        lock_t l(view_mutex_);
        view_.push_back(switch_view);
        if (view_.size() == 1) {
            role_monitoring_poller_->run();
        }
    }

    // send role request to get gen_id
    switch_view->changeSwitchRole(fluid_msg::OFPCR_ROLE_NOCHANGE, true);
    return switch_view;
}

void MastershipView::deleteSwitch(SwitchPtr sw)
{
    auto check_dpid = [sw](SwitchViewPtr switch_view_ptr) {
        return sw->dpid() == switch_view_ptr->getDPID();
    };

    {
        lock_t l(view_mutex_);
        view_.erase(std::remove_if(view_.begin(), view_.end(), check_dpid),
                    view_.end());
        if (view_.size() == 0) {
            role_monitoring_poller_->pause();
        }
    }
}

void MastershipView::setStatus(ControllerStatus status)
{
    ctrl_status_ = status;

    if (+ControllerStatus::UNDEFINED != ctrl_status_ && not initialised_) {
        initialised_ = true;
        init_promise_.set_value();
        VLOG(3) << "[RecoveryManager] Mastership view - Promise set";
    }
}

void MastershipView::setupNewRoleForAll(fluid_msg::ofp_controller_role role)
{
    static std::mutex role_request_mutex_;
    lock_t l(role_request_mutex_);

    for (auto& sw_ptr : view()) {
        sw_ptr->changeSwitchRole(role);
    }
}

void MastershipView::polling()
{
    setupNewRoleForAll(fluid_msg::OFPCR_ROLE_NOCHANGE);
}

std::vector<SwitchViewPtr> MastershipView::view() const
{
    lock_t l(view_mutex_);
    return view_;
}

// cluster node

ClusterNode::ClusterNode(int id, ControllerState state,
                         fluid_msg::ofp_controller_role role,
                         bool is_this_node_current_controller)
    : id_(id), state_(state), role_(role),
      is_this_node_current_controller_(is_this_node_current_controller)
{}

int ClusterNode::id() const {
    return id_;
}

ControllerState ClusterNode::state() const {
    return state_;
}
void ClusterNode::setState(ControllerState state) {
    state_ = state;
}

fluid_msg::ofp_controller_role ClusterNode::role() const {
    return role_;
}
void ClusterNode::setRole(fluid_msg::ofp_controller_role role) {
    role_ = role;
}

bool ClusterNode::isThisNodeCurrentController() const {
    return is_this_node_current_controller_;
}

//openflow
void ClusterNode::setOpenflowAddr(const std::pair<std::string, int>& of_addr) {
    of_addr_ = of_addr;
}
std::string ClusterNode::openflowIP() const {
    return of_addr_.first;
}
int ClusterNode::openflowPort() const {
    return of_addr_.second;
}

void ClusterNode::setCommunicationType(CommunicationType communication_type)
{
    CHECK(+CommunicationType::UNDEFINED != communication_type);
    if (+CommunicationType::BROADCAST == communication_type) {
        hb_mode_ = "broadcast";
    } else if (+CommunicationType::MULTICAST == communication_type) {
        hb_mode_ = "multicast";
    } else if (+CommunicationType::UNICAST == communication_type) {
        hb_mode_ = "unicast";
    }
}

void ClusterNode::setHbStatus(ControllerStatus status) {
    hb_status_ = status;
}

void ClusterNode::setHeartbeatConnection(const std::string &hb_address,
                                         int hb_port)
{
    hb_ip_ = hb_address;
    hb_port_ = hb_port;
}

ControllerStatus ClusterNode::hbStatus() const {
    return hb_status_;
}

std::string ClusterNode::hbMode() const {
    return hb_mode_;
}
std::string ClusterNode::hbIP() const {
    return hb_ip_;
}
int ClusterNode::hbPort() const {
    return hb_port_;
}

int ClusterNode::hbInterval() const {
    return hb_interval_;
}
int ClusterNode::hbPrimaryDeadInterval() const {
    return hb_primary_dead_interval_;
}
int ClusterNode::hbBackupDeadInterval() const {
    return hb_backup_dead_interval_;
}
int ClusterNode::hbPrimaryWaitingInterval() const {
    return hb_primary_waiting_interval_;
}

bool ClusterNode::isConnection() const {
    return is_connection_;
}
void ClusterNode::setConnection(bool connect) {
    is_connection_ = connect;
}

// data store
void ClusterNode::setDb(std::string IP, int port) {
    db_ip_ = IP;
    db_port_ = port;
}
std::string ClusterNode::dbIP() const  {
    return db_ip_;
}
int ClusterNode::dbPort() const {
    return db_port_;
}

void ClusterNode::setDbType(std::string type) {
    db_type_ = type;
}
std::string ClusterNode::dbType() const {
    return db_type_;
}

bool ClusterNode::isDbConnection() const {
    return is_db_connection_;
}
void ClusterNode::setDbConnection(bool connect) {
    is_db_connection_ = connect;
}

// Recovery manager

void RecoveryManager::parseConfig(const Config& root_config)
{
    auto& config = config_cd(root_config, "recovery-manager");

    const auto status = config_get(config, "status", "backup");
    CHECK("primary" == status or "backup" == status or "recovery" == status);
    if ("primary" == status) {
        controller_status_ = ControllerStatus::PRIMARY;
    } else if ("backup" == status) {
        controller_status_ = ControllerStatus::BACKUP;
    } else if ("recovery" == status) {
        controller_status_ = ControllerStatus::RECOVERY;
    }

    id_ = config_get(config, "id", 777);
    primary_controller_id_ = DEFAULT_PRIMARY_CONTROLLER_ID;

    // read heartbeat service settings
    const auto heartbeat_mode = config_get(config, "hb-mode", "broadcast");
    CHECK("broadcast" == heartbeat_mode or
          "multicast" == heartbeat_mode or
          "unicast" == heartbeat_mode);
    if ("broadcast" == heartbeat_mode) {
        heartbeat_communication_type_ = CommunicationType::BROADCAST;
    } else if ("multicast" == heartbeat_mode) {
        heartbeat_communication_type_ = CommunicationType::MULTICAST;
    }  else if ("unicast" == heartbeat_mode) {
        heartbeat_communication_type_ = CommunicationType::UNICAST;
    }
    CHECK(+CommunicationType::UNDEFINED != heartbeat_communication_type_);

    switch (heartbeat_communication_type_) {
    case CommunicationType::BROADCAST:
        heartbeat_address_ = "broadcast";
        heartbeat_port_ = config_get(config, "hb-port-broadcast", 50000);
        break;
    case CommunicationType::MULTICAST:
        heartbeat_address_ =
                config_get(config, "hb-address-multicast", "127.0.0.1");
        heartbeat_port_ =
                config_get(config, "hb-port-multicast", 50000);
        break;
    case CommunicationType::UNICAST:
        if (UNICAST_PRIMARY_ID == id_) {
            heartbeat_address_ =
                    config_get(config, "hb-address-primary", "127.0.0.1");
            heartbeat_port_ = config_get(config, "hb-port-primary", 1234);
        } else if (UNICAST_BACKUP_ID == id_) {
            heartbeat_address_ =
                    config_get(config, "hb-address-backup", "127.0.0.1");
            heartbeat_port_ = config_get(config, "hb-port-backup", 1237);
        }
        break;
    default:
        CHECK(false);
        break;
    }
    CHECK(not heartbeat_address_.empty());
    CHECK(heartbeat_port_ > 0);

    master_role_monitoring_interval_ = config_get(config, 
                                              "role-monitoring-interval", 1000);
    max_waiting_recovery_interval_ = std::chrono::seconds(config_get(config,
                                              "recovery-waiting-seconds", 0));
}

void RecoveryManager::initCurrentNode(const Config& root_config)
{
    current_node_ = std::make_shared<ClusterNode>(id_,
                                                  ControllerState::ACTIVE,
                                                  fluid_msg::OFPCR_ROLE_EQUAL,
                                                  true);
    auto of_server_config = config_cd(root_config, "of-server");
    auto of_address = config_get(of_server_config, "address", "127.0.0.1");
    auto of_port = config_get(of_server_config, "port", 6653);
    current_node_->setOpenflowAddr(std::make_pair(of_address, of_port));
    cluster_.push_back(current_node_);

    current_node_->setDbType("redis");
    current_node_->setDb(db_connector_->getDatabaseAddress(),
                         db_connector_->getDatabasePort());
    current_node_->setDbConnection(db_connector_->hasConnection());
}

void RecoveryManager::initHeartbeat(const Config& root_config)
{
    heartbeat_core_ = std::make_unique<HeartbeatCore>(root_config);
    auto heartbeat_thread = new QThread(this);
    heartbeat_core_->moveToThread(heartbeat_thread);
    // signals from recovery manager to heartbeat core
    QObject::connect(this, &RecoveryManager::toStartHeartbeatService,
                     heartbeat_core_.get(), &HeartbeatCore::startService,
                     Qt::QueuedConnection);
    QObject::connect(this, &RecoveryManager::toStopHeartbeatService,
                     heartbeat_core_.get(), &HeartbeatCore::stopService,
                     Qt::QueuedConnection);
    QObject::connect(this, &RecoveryManager::dbParamsChanged,
                     heartbeat_core_.get(), &HeartbeatCore::dbParamsChanged,
                     Qt::QueuedConnection);
    heartbeat_thread->start();

    // signals from heartbeat core to recovery manager
    QObject::connect(heartbeat_core_.get(),
                     &HeartbeatCore::connectionToBackupEstablished, this,
                     &RecoveryManager::connectionToBackupControllerEstablished,
                     Qt::QueuedConnection);
    QObject::connect(heartbeat_core_.get(),
                     &HeartbeatCore::connectionToPrimaryEstablished, this,
                     &RecoveryManager::connectionToPrimaryEstablished,
                     Qt::QueuedConnection);
    QObject::connect(heartbeat_core_.get(), &HeartbeatCore::backupDied,
                     this, &RecoveryManager::backupDead, Qt::QueuedConnection);
    QObject::connect(heartbeat_core_.get(), &HeartbeatCore::primaryDied,
                     this, &RecoveryManager::recovery, Qt::QueuedConnection);
    QObject::connect(heartbeat_core_.get(),
                     &HeartbeatCore::modeChangedToPrimary,
                     this, &RecoveryManager::setupPrimaryMode,
                     Qt::QueuedConnection);
    QObject::connect(heartbeat_core_.get(), &HeartbeatCore::paramsChanged,
                     this, &RecoveryManager::setParams, Qt::QueuedConnection);
}

void RecoveryManager::initMastership()
{
    std::promise<void> init_promise;
    init_future_ = std::move(init_promise.get_future());

    mastership_view_ = std::make_shared<MastershipView>(
        std::move(init_promise), master_role_monitoring_interval_);

    CHECK(heartbeat_core_);
    QObject::connect(this, &RecoveryManager::linkBetweenControllersIsDown,
                     heartbeat_core_.get(), &HeartbeatCore::linkDown,
                     Qt::QueuedConnection);

    CHECK(db_connector_);
    rm_checker_ = std::make_unique<RecoveryModeChecker>(this, db_connector_);
}

void RecoveryManager::sendStartHeartbeat()
{
    CHECK(+CommunicationType::UNDEFINED != heartbeat_communication_type_);
    CHECK(+ControllerStatus::UNDEFINED != controller_status_);

    emit toStartHeartbeatService(heartbeat_communication_type_,
                                 controller_status_);
    LOG(WARNING) << "[RecoveryManager] Heartbeat - Start heartbeat in "
                 << heartbeat_communication_type_._to_string()
                 << " " << controller_status_._to_string()
                 << " mode with address=" << heartbeat_address_
                 << " and port=" << heartbeat_port_;
}

void RecoveryManager::setInitControllerStatus()
{
    // warning
    // for backup broadcast and multicast
    // leave status inside mastership undefined
    // because if there is only one controller
    // then role will become master in some time
    // and we avoid several role changes

    if (controller_status_ == +ControllerStatus::BACKUP) {
        if (heartbeat_communication_type_ == +CommunicationType::MULTICAST ||
            heartbeat_communication_type_ == +CommunicationType::BROADCAST) {
            return;
        }
    }

    mastership_view_->setStatus(controller_status_);
}

void RecoveryManager::init(Loader* loader, const Config& root_config)
{
    qRegisterMetaType<CommunicationType>("CommunicationType");
    qRegisterMetaType<ControllerStatus>("ControllerStatus");
    qRegisterMetaType<ParamsMessage>("ParamsMessage");

    SwitchOrderingManager::get(loader)->registerHandler(this, 0);
    dpid_checker_ = DpidChecker::get(loader);
    db_connector_ = DatabaseConnector::get(loader);

    parseConfig(root_config);
    initCurrentNode(root_config);
    initHeartbeat(root_config);
    initMastership();
}

void RecoveryManager::startUp(Loader *provider)
{
    startHeartbeat();
}

void RecoveryManager::switchUp(SwitchPtr sw)
{
    auto switch_view_ptr = mastership_view_->addSwitch(
        sw, this, master_role_monitoring_interval_);

    init_future_.wait();
    try {
        switch(mastership_view_->getStatus()) {
        case ControllerStatus::UNDEFINED:
            VLOG(3) << "[RecoveryManager] switchUp - "
                       "mastership_view status is undefined";
            break;
        case ControllerStatus::PRIMARY:
            VLOG(3) << "[RecoveryManager] switchUp - "
                       "mastership_view status is primary";
            switch_view_ptr->changeSwitchRole(fluid_msg::OFPCR_ROLE_MASTER);
            break;
        case ControllerStatus::BACKUP:
            VLOG(3) << "[RecoveryManager] switchUp - "
                       "mastership_view status is backup";
            switch_view_ptr->changeSwitchRole(fluid_msg::OFPCR_ROLE_SLAVE);
            break;
        case ControllerStatus::RECOVERY:
            VLOG(3) << "[RecoveryManager] switchUp - "
                       "mastership_view status is recovery";
            break;
        }
    } catch(const std::future_error& e) {
        LOG(ERROR) << "[RecoveryManager] switchUp - "
                      "future error inside switchUp. error_code=" << e.code();
    }

    rm_checker_->updateSwitch(sw->dpid(), Action::ADD);
}

void RecoveryManager::switchDown(SwitchPtr sw)
{
    mastership_view_->deleteSwitch(sw);
    rm_checker_->updateSwitch(sw->dpid(), Action::DELETE);
}

void RecoveryManager::startHeartbeat()
{
    CHECK(+CommunicationType::UNDEFINED != heartbeat_communication_type_);
    current_node_->setCommunicationType(heartbeat_communication_type_);
    current_node_->setHbStatus(controller_status_);
    current_node_->setHeartbeatConnection(heartbeat_address_, heartbeat_port_);

    sendStartHeartbeat();
    setInitControllerStatus();
}

void RecoveryManager::stopHeartbeat()
{
    emit toStopHeartbeatService();
}

bool RecoveryManager::isPrimary() const
{
    return +ControllerStatus::PRIMARY == controller_status_;
}

bool RecoveryManager::isBackup() const
{
    return +ControllerStatus::BACKUP == controller_status_;
}

void RecoveryManager::recovery()
{
    if (+ControllerStatus::PRIMARY == controller_status_) {
        return;
    }

    if (+ControllerStatus::BACKUP == controller_status_) {
        LOG(ERROR) << "[RecoveryManager] Heartbeat - Primary "
                      "controller has failed. Start recovery procedure";
    } else if (+ControllerStatus::RECOVERY == controller_status_) {
        LOG(ERROR) << "[RecoveryManager] Heartbeat - "
                   << "All switches are connected. Start recovery procedure";
    }

    controller_status_ = ControllerStatus::PRIMARY;

    // change role in MastershipView and on switches
    mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_MASTER);
    mastership_view_->setStatus(controller_status_);

    for (auto node : cluster_) {
        // set failed controller as not_active
        if (+ControllerStatus::PRIMARY == node->hbStatus() &&
                (node->id() != id_)) {
            node->setState(ControllerState::NOT_ACTIVE);
            node->setHbStatus(ControllerStatus::BACKUP);
        } else if (node->id() == id_) {
            node->setState(ControllerState::ACTIVE);
            node->setHbStatus(ControllerStatus::PRIMARY);
        } else {
            node->setHbStatus(ControllerStatus::BACKUP);
        }
    }

    sendStartHeartbeat();

    // recovery datastore
    db_connector_->setupMasterRole();
    emit signalRecovery();
    LOG(WARNING) << "[RecoveryManager] Heartbeat - Service restarted"
                    " in Primary mode. Recovery is completed";
}

void RecoveryManager::backupDead(int backup_id)
{
    auto backup_lamdba = [backup_id](ClusterNodePtr ptr)
    {
        return ptr->id() == backup_id;
    };
    auto cluster_node_it = find_if(cluster_.begin(), cluster_.end(),
                                   backup_lamdba);
    CHECK(cluster_.end() != cluster_node_it);
    if (+ControllerStatus::BACKUP == (*cluster_node_it)->hbStatus()) {
        LOG(WARNING) << "[RecoveryManager] Heartbeat - Backup controller with id="
                     << backup_id << " failed";
        (*cluster_node_it)->setState(ControllerState::NOT_ACTIVE);
    }
}

void RecoveryManager::setFormerPrimaryToBackup()
{
    auto primary_lambda = [this](ClusterNodePtr ptr)
    {
        return ptr->id() == this->primary_controller_id_;
    };
    auto cluster_node_it = find_if(cluster_.begin(), cluster_.end(),
                                   primary_lambda);
    CHECK(cluster_.end() != cluster_node_it);
    (*cluster_node_it)->setState(ControllerState::NOT_ACTIVE);
    (*cluster_node_it)->setHbStatus(ControllerStatus::BACKUP);
}

void RecoveryManager::setPrimaryNode(int primary_id)
{
    auto cluster_node_it = find_if(cluster_.begin(),
                                   cluster_.end(),
                                   [primary_id](ClusterNodePtr ptr){
            return ptr->id() == primary_id;
    });
    if (cluster_.end() == cluster_node_it) {
        auto new_cluster_node
                = std::make_shared<ClusterNode>(primary_id,
                                                ControllerState::ACTIVE,
                                                fluid_msg::OFPCR_ROLE_MASTER);

        new_cluster_node->setCommunicationType(heartbeat_communication_type_);
        new_cluster_node->setHbStatus(ControllerStatus::PRIMARY);

        cluster_.insert(cluster_node_it, new_cluster_node);

        LOG(WARNING) << "[RecoveryManager] Cluster - New cluster node "
                        "has been added. Cluster size=" << cluster_.size();
    } else {
        (*cluster_node_it)->setState(ControllerState::ACTIVE);
        (*cluster_node_it)->setHbStatus(ControllerStatus::PRIMARY);
    }
}

void RecoveryManager::connectionToPrimaryEstablished(int primary_id)
{
    if (+ControllerStatus::PRIMARY == controller_status_) {
        return;
    }

    if (DEFAULT_PRIMARY_CONTROLLER_ID == primary_controller_id_) {
        primary_controller_id_ = primary_id;
    }

    LOG(WARNING) << "[RecoveryManager] Heartbeat - Connection to primary "
                    "controller with id=" << primary_id << " established";
    this->setPrimaryNode(primary_id);

    if (primary_controller_id_ == primary_id) {
        // mastership view configuration update
        controller_status_ = ControllerStatus::BACKUP;
        mastership_view_->setStatus(controller_status_);
    } else {
        // primary controller was alive but has been changed
        this->setFormerPrimaryToBackup();
        primary_controller_id_ = primary_id;
    }
}

void RecoveryManager::setupPrimaryMode()
{
    if (+ControllerStatus::PRIMARY == controller_status_) {
        return;
    }

    if (+ControllerStatus::RECOVERY == controller_status_) {
        LOG(WARNING) << "[RecoveryManager] Heartbeat - "
                        "No any primary controller. Staying in RECOVERY mode";

        connect(rm_checker_.get(), &RecoveryModeChecker::readyToRecover,
                [this]() {
                    controller_status_ = ControllerStatus::BACKUP;
                    recovery();
                });
        connect(rm_checker_.get(), &RecoveryModeChecker::failedRecover,
                [this]() {
                    controller_status_ = ControllerStatus::BACKUP;
                    setupPrimaryMode();
                });

        rm_checker_->startRecoveryCheck(max_waiting_recovery_interval_);

        // reload heartbeat to PRIMARY
        current_node_->setHbStatus(ControllerStatus::PRIMARY);
        emit toStartHeartbeatService(heartbeat_communication_type_,
                                     ControllerStatus::PRIMARY);

    } else if (+ControllerStatus::BACKUP == controller_status_) {
        controller_status_ = ControllerStatus::PRIMARY;

        LOG(WARNING) << "[RecoveryManager] Heartbeat - Switching from BACKUP "
                        "to PRIMARY mode";
        // change role in MastershipView and on switches
        mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_MASTER);
        mastership_view_->setStatus(controller_status_);
        current_node_->setHbStatus(controller_status_);
        sendStartHeartbeat();
        emit signalSetupPrimaryMode();
    }
}

void RecoveryManager::setupBackupMode(uint64_t dpid_switch_slaved)
{
    if (+ControllerStatus::BACKUP == controller_status_) {
        return;
    }
    controller_status_ = ControllerStatus::BACKUP;
    emit linkBetweenControllersIsDown();

    auto log = "[RecoveryManager] Heartbeat - "
               "Switching from PRIMARY to BACKUP mode";

    (DEFAULT_DPID == dpid_switch_slaved)
        ?  LOG(INFO) << log
        :  LOG(INFO) << log << ". Failed switch dpid=" << dpid_switch_slaved;

    // change role in MastershipView and on switches
    mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_SLAVE);
    mastership_view_->setStatus(controller_status_);
    current_node_->setHbStatus(controller_status_);

    for (auto node : cluster_) {
        if (node->id() != id_) {
            node->setState(ControllerState::NOT_ACTIVE);
            node->setHbStatus(ControllerStatus::BACKUP);
        }
    }

    sendStartHeartbeat();
    emit signalSetupBackupMode();
}

void RecoveryManager::setParams(const ParamsMessage& p)
{
    const auto controller_id = p.unique_node_id;
    auto cluster_node_it = find_if(cluster_.begin(),
                                   cluster_.end(),
                                   [controller_id](ClusterNodePtr ptr)
    {
        return ptr->id() == controller_id;
    });
    CHECK(cluster_.end() != cluster_node_it);

    auto b = [](const QHostAddress& ip) -> std::string
    {
        return QHostAddress::Broadcast == ip ?
                    "broadcast" : ip.toString().toStdString();
    };

    VLOG(10) << "[RecoveryManager] Parameters came. id=" << controller_id
             << ", heartbeat_ip=" << b(p.heartbeat_connection_params.first)
             << ", heartbeat_port=" << p.heartbeat_connection_params.second
             << ", db_ip=" << b(p.db_connection_params.first)
             << ", db_port=" << p.db_connection_params.second
             << ", openflow_ip=" << b(p.openflow_connection_params.first)
             << ", openflow_port=" << p.openflow_connection_params.second;

    auto of_pair = std::make_pair(b(p.openflow_connection_params.first),
                                  p.openflow_connection_params.second);
    (*cluster_node_it)->setOpenflowAddr(of_pair);
    (*cluster_node_it)->setHeartbeatConnection(
                b(p.heartbeat_connection_params.first),
                p.heartbeat_connection_params.second);
    (*cluster_node_it)->setDb(b(p.db_connection_params.first),
                              p.db_connection_params.second);


    if (+ControllerStatus::PRIMARY != controller_status_) {
        // find master
        auto find_lambda = [](ClusterNodePtr ptr)
        {
            return +ControllerStatus::PRIMARY == ptr->hbStatus();
        };
        auto primary_it =
                find_if(cluster_.begin(), cluster_.end(), find_lambda);
        if (cluster_.end() == primary_it) {
            // if 3 controllers, may be the situation when
            // one backup received params from another backup
            VLOG(7) << "[RecoveryManager] Primary node not found";
            return;
        }

        db_connector_->setupSlaveOf((*primary_it)->dbIP().c_str(),
                                    (*primary_it)->dbPort());

        VLOG(7) << "[RecoveryManager] Controller became slave of db_ip="
                << (*primary_it)->dbIP()
                << " and db_port=" << (*primary_it)->dbPort();
    }
}

void RecoveryManager::connectionToBackupControllerEstablished(int backup_id)
{
    LOG(WARNING) << "[RecoveryManager] Heartbeat - Connection to backup "
                    "controller with id=" << backup_id << " established";
    auto cluster_node_it = find_if(cluster_.begin(),
                                   cluster_.end(),
                                   [backup_id](ClusterNodePtr ptr)
    {
            return ptr->id() == backup_id;
    });
    if (cluster_.end() == cluster_node_it) {
        auto new_cluster_node = std::make_shared<ClusterNode>(backup_id,
                ControllerState::ACTIVE,
                fluid_msg::OFPCR_ROLE_SLAVE);

        new_cluster_node->setCommunicationType(heartbeat_communication_type_);
        new_cluster_node->setHbStatus(ControllerStatus::BACKUP);
        cluster_.insert(cluster_node_it, new_cluster_node);
    } else {
        (*cluster_node_it)->setState(ControllerState::ACTIVE);
        (*cluster_node_it)->setHbStatus(ControllerStatus::BACKUP);
    }
}

std::vector<ClusterNodePtr> RecoveryManager::cluster() const {
    return cluster_;
}

std::shared_ptr<MastershipView> RecoveryManager::mastershipView() const {
    return mastership_view_;
}

int RecoveryManager::getID() const {
    return id_;
}

DpidChecker* RecoveryManager::dpidChecker() const
{
    return dpid_checker_;
}

bool RecoveryManager::processCommand(Config command)
{
    std::string command_str = config_get(command, "command", "");

    if ("changeRole" == command_str) {
        VLOG(2) << "[RecoveryManager-REST] processCommand: modifyRole";
        modifyRole(command);
    } else if ("changeHeartbeat" == command_str) {
        LOG(ERROR) << "[RecoveryManager-REST] modify "
                      "heartbeat button deprecated";
    } else if ("changeDatabase" == command_str) {
        VLOG(2) << "[RecoveryManager-REST] processCommand: changeDatabase";
        modifyDatabase(command);
    } else if ("changeStatus" == command_str) {
        VLOG(2) << "[RecoveryManager-REST] processCommand: changeStatus";
        modifyStatus(command);
    } else if ("stopHeartbeat" == command_str) {
        VLOG(2) << "[RecoveryManager-REST] processCommand: stopHeartbeat";
        stopHeartbeat();
    } else {
        LOG(WARNING) << "[RecoveryManager-REST] Uknown command from WEB UI";
    }
    return true;
}

bool RecoveryManager::modifyRole(Config command)
{
    std::string newRole = config_get(command, "role", "NOCHANGE");

    if ("NOCHANGE" == newRole) {
        mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_NOCHANGE);
    } else if ("MASTER" == newRole) {
        mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_MASTER);
    } else if ("EQUAL" == newRole) {
        mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_EQUAL);
    } else if ("SLAVE" == newRole) {
        mastership_view_->setupNewRoleForAll(fluid_msg::OFPCR_ROLE_SLAVE);
    } else {
        LOG(WARNING) << "[RecoveryManager-REST] Unknown new role";
        return false;
    }
    return true;
}

bool RecoveryManager::modifyDatabase(Config command)
{
    auto new_db_ip = config_get(command, "db_IP", "127.0.0.1");
    auto new_db_port = std::stoi(config_get(command, "db_port", "6379"));

    VLOG(10) << "[RecoveryManager] ModifyDatabase. Db address=" << new_db_ip
             << ", db port=" << new_db_port;

    emit dbParamsChanged(new_db_ip, new_db_port);
    return true;
}

bool RecoveryManager::modifyStatus(Config command)
{
    std::string new_status = config_get(command, "hb_status", "");

    if ("Primary" == new_status &&
             +ControllerStatus::PRIMARY != controller_status_) {
        this->recovery();
        db_connector_->setupMasterRole();
    } else if ("Backup" == new_status) {
        this->setupBackupMode();
    } else {
        LOG(WARNING) << "[RecoveryManager] modifyStatus error. new_status="
                     << new_status;
        return false;
    }
    return true;
}

} // namespace runos
