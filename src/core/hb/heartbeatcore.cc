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
#include "heartbeatcore.hpp"

#include "runos/core/logging.hpp"
#include "runos/core/assert.hpp"

#include <QtNetwork/QUdpSocket>
#include <QHostAddress>
#include <QSignalMapper>
#include <QDataStream>
#include <QTimer>
#include <QTime>

#include <memory>
#include <unordered_map>

using runos::config_cd;
using runos::config_get;

static constexpr qint32 UNICAST_PRIMARY_ID = 1;
static constexpr qint32 UNICAST_BACKUP_ID = 2;
static constexpr qint32 DEFAULT_ID = -1;

static auto deleter = [](QTimer* timer)
{
    timer->deleteLater();
};

static QHostAddress toHostAddress(const std::string& str)
{
    return QHostAddress(QString::fromStdString(str));
}

enum class TimerType {
    HEARTBEAT,
    PRIMARY_DEAD,
    PRIMARY_WAITING_TIMER,
    BACKUP_DEAD,
    ALL
};

// HeartbeatSocket implementation
class HeartbeatSocket : public QUdpSocket
{
    Q_OBJECT
public:
    HeartbeatSocket(QObject* parent = Q_NULLPTR);
    template <class Message>
    void send(uint16_t cmd, Message&& msg);
    void setConnection(const Connection& connection);
    QByteArray receiveData();

private:
    Connection connection_;
};

HeartbeatSocket::HeartbeatSocket(QObject *parent)
    : QUdpSocket(parent)
{
}

template <class Message>
void HeartbeatSocket::send(uint16_t cmd, Message&& msg)
{
    QByteArray buf;
    QDataStream s(&buf, QIODevice::ReadWrite);
    s.setVersion(QDataStream::Qt_5_9);
    s << cmd << msg;
    s.device()->seek(0);
    QByteArray message_array = s.device()->readAll();

    auto bytes = this->writeDatagram(message_array.data(),
        message_array.size(), connection_.first, connection_.second);
    CHECK(-1 != bytes);
}


void HeartbeatSocket::setConnection(const Connection& connection)
{
    connection_ = connection;
}

QByteArray HeartbeatSocket::receiveData()
{
    CHECK(this->hasPendingDatagrams());
    QByteArray datagram;
    datagram.resize(this->pendingDatagramSize());
    this->readDatagram(datagram.data(), datagram.size());
    return datagram;
}

struct Timers {
    struct BackupTimerData {
        bool is_connection_to_backup_established;
        std::unique_ptr<QTimer, decltype(deleter)> timer;
    };

    std::unordered_map<int, BackupTimerData> backup_dead_timers;
    QSignalMapper* backup_timer_signal_mapper;
    QTimer* heartbeat_timer;
    QTimer* primary_dead_timer;
    QTimer* primary_waiting_timer;
    std::chrono::milliseconds backup_dead_interval;

    explicit Timers(QObject* parent = nullptr);
    void setInterval(TimerType type, std::chrono::milliseconds ms);
    void startTimer(TimerType type, qint32 backup_node_id = DEFAULT_ID);
    void stopTimer(TimerType type, qint32 backup_node_id = DEFAULT_ID);
private:
    void stop_backup_timer(qint32 unique_node_id = DEFAULT_ID);
};

struct HeartbeatCore::implementation
{
    struct HeartbeatSettings {
        QHostAddress unicast_host;
        quint16 unicast_port;
        QHostAddress unicast_host_send_to;
        quint16 unicast_port_send_to;
        quint16 broadcast_port;
        QHostAddress multicast_host;
        quint16 multicast_port;
    };
    HeartbeatSettings settings;

    Timers timers;
    HeartbeatSocket* udp_socket = nullptr;
    qint64 hbcounter = 0;
    bool is_connection_to_primary_established = false;
    bool is_link_between_controllers_down = false; // TODO - think of splitbrain
    qint32 unique_node_id = DEFAULT_ID;
    qint32 primary_node_id = DEFAULT_ID;
    ControllerStatus heartbeat_mode = ControllerStatus::UNDEFINED;
    CommunicationType communication_type = CommunicationType::UNDEFINED;
    ParamsMessage cached_params_message;
    QTime hb_start_time;

    explicit implementation(QObject* parent = nullptr);
};

// HeartbeatCore implementation

HeartbeatCore::HeartbeatCore(const runos::Config& root_config)
    : QObject()
    , impl_(std::make_unique<implementation>(this))
{
    init_config(root_config);
    
    //std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    impl_->hb_start_time = QTime::currentTime();

    QObject::connect(impl_->timers.primary_dead_timer, &QTimer::timeout,
                     this, &HeartbeatCore::primary_death);
    QObject::connect(impl_->timers.primary_waiting_timer, &QTimer::timeout,
                     this, &HeartbeatCore::check_primary);
    QObject::connect(impl_->timers.heartbeat_timer, &QTimer::timeout,
                     this, &HeartbeatCore::send_request);

    QObject::connect(impl_->timers.backup_timer_signal_mapper,
                     qOverload<int>(&QSignalMapper::mapped),
                     this, &HeartbeatCore::backup_death);
}

void HeartbeatCore::init_config(const runos::Config &root_config)
{
    using ms = std::chrono::milliseconds;
    // read Recovery application settings
    auto config = config_cd(root_config, "recovery-manager");
    // root config
    impl_->unique_node_id = config_get(config, "id", 777);

    // set intervals to timers
    auto heartbeat_interval = config_get(config, "hb-interval", 200);
    impl_->timers.setInterval(TimerType::HEARTBEAT,
                              std::move(ms{heartbeat_interval}));
    auto primary_dead_interval =
            config_get(config, "hb-primaryDeadInterval", 800);
    primary_dead_interval *= impl_->unique_node_id;
    impl_->timers.setInterval(TimerType::PRIMARY_DEAD,
                              std::move(ms{primary_dead_interval}));
    auto backup_dead_interval =
            config_get(config, "hb-backupDeadInterval", 1000);
    impl_->timers.setInterval(TimerType::BACKUP_DEAD,
                              std::move(ms{backup_dead_interval}));
    auto primary_waiting_interval =
            config_get(config, "hb-primaryWaitingInterval", 600);
    impl_->timers.setInterval(TimerType::PRIMARY_WAITING_TIMER,
                             std::move(ms{primary_waiting_interval}));
    // address settings
    if (UNICAST_PRIMARY_ID == impl_->unique_node_id) {
        impl_->settings.unicast_host = toHostAddress(
                config_get(config, "hb-address-primary", "127.0.0.1"));
        impl_->settings.unicast_port =
                config_get(config, "hb-port-primary", 1234);
        impl_->settings.unicast_host_send_to = toHostAddress(
                config_get(config, "hb-address-backup", "127.0.0.1"));
        impl_->settings.unicast_port_send_to =
                config_get(config, "hb-port-backup", 1237);
    } else if (UNICAST_BACKUP_ID == impl_->unique_node_id) {
        impl_->settings.unicast_host = toHostAddress(
                config_get(config, "hb-address-backup", "127.0.0.1"));
        impl_->settings.unicast_port =
                config_get(config, "hb-port-backup", 1237);
        impl_->settings.unicast_host_send_to = toHostAddress(
                config_get(config, "hb-address-primary", "127.0.0.1"));
        impl_->settings.unicast_port_send_to =
                config_get(config, "hb-port-primary", 1234);
    }

    impl_->settings.broadcast_port =
            config_get(config, "hb-port-broadcast", 50000);
    impl_->settings.multicast_host = toHostAddress(
            config_get(config, "hb-address-multicast", "127.0.0.1"));
    impl_->settings.multicast_port =
            config_get(config, "hb-port-multicast", 50000);
    // db params
    auto db_config = config_cd(root_config, "database-connector");
    auto db_address = toHostAddress(
            config_get(db_config, "db-address", "127.0.0.1"));
    quint16 db_port = config_get(db_config, "db-port", 6379);
    // of-server config
    auto of_server_config = config_cd(root_config, "of-server");
    auto openflow_address = toHostAddress(
            config_get(of_server_config, "address", "127.0.0.1"));
    quint16 openflow_port = config_get(of_server_config, "port", 6653);
    // form cached params
    impl_->cached_params_message = { impl_->unique_node_id,
                  {QHostAddress(), 0}, // heartbeat will be changed after start
                  {openflow_address, openflow_port},
                  {db_address, db_port}
                            };
}

HeartbeatCore::~HeartbeatCore() {}

void HeartbeatCore::startService(CommunicationType type,
                                 ControllerStatus controller_status)
{
    CHECK(+CommunicationType::UNDEFINED != type);
    CHECK(+ControllerStatus::UNDEFINED != controller_status);
    switch (controller_status) {
    case ControllerStatus::PRIMARY:
        start_transmitting(type);
        break;
    case ControllerStatus::BACKUP:
    case ControllerStatus::RECOVERY:
        start_receiving(type);
        break;
    default:
        break;
    }
}

void HeartbeatCore::stopService()
{
    impl_->timers.stopTimer(TimerType::ALL);
    impl_->is_connection_to_primary_established = false;
    VLOG(5) << "[RecoveryManager] Heartbeat - Heartbeat stopped slot raised";
}

void HeartbeatCore::start_receiving(CommunicationType communication_type)
{
    prepare_connection(communication_type, ControllerStatus::BACKUP);
    if (not impl_->is_connection_to_primary_established and
            not impl_->is_link_between_controllers_down) {
        impl_->timers.startTimer(TimerType::PRIMARY_WAITING_TIMER);
        VLOG(8) << "[HeartbeatCore] primary_waiting_timer start";
    }

    VLOG(5) << "[HeartbeatCore] Start receiving";
}

void HeartbeatCore::start_transmitting(CommunicationType communication_type)
{
    prepare_connection(communication_type, ControllerStatus::PRIMARY);
    impl_->timers.startTimer(TimerType::HEARTBEAT);

    VLOG(5) << "[HeartbeatCore] Start transmitting - heartbeat_timer start";
}

void HeartbeatCore::send_request()
{
    impl_->udp_socket->send(HeartbeatCommand::HEARTBEAT_ECHO_REQUEST,
        EchoMessage{impl_->unique_node_id, impl_->hb_start_time, ++(impl_->hbcounter)});
}

void HeartbeatCore::bind_socket(CommunicationType communication_type)
{
    if (QAbstractSocket::UnconnectedState != impl_->udp_socket->state()) {
        return;
    }

    if (+CommunicationType::UNICAST == communication_type) {
        VLOG(7) << "[RecoveryManager] Heartbeat core - Unicast binding. "
           <<  "Host=" << impl_->settings.unicast_host.toString().toStdString()
           << ", port=" << impl_->settings.unicast_port;
        auto c = impl_->udp_socket->bind(impl_->settings.unicast_host,
                                         impl_->settings.unicast_port);
        CHECK(c);
    } else if (+CommunicationType::BROADCAST == communication_type) {
        auto c = impl_->udp_socket->bind(impl_->settings.broadcast_port,
                                         QUdpSocket::ShareAddress);
        CHECK(c);
    } else if (+CommunicationType::MULTICAST == communication_type) {
        auto c = impl_->udp_socket->bind(QHostAddress::AnyIPv4,
                                         impl_->settings.multicast_port,
                                         QUdpSocket::ShareAddress);
        CHECK(c);
        impl_->udp_socket->joinMulticastGroup(impl_->settings.multicast_host);
    }

    QObject::connect(impl_->udp_socket, &QUdpSocket::readyRead,
                     this, &HeartbeatCore::ready_read_handler,
                     Qt::UniqueConnection);
}

void HeartbeatCore::prepare_connection(CommunicationType type,
                                       ControllerStatus status)
{
    CHECK(+CommunicationType::UNDEFINED != type);
    impl_->communication_type = type;

    stopService();

    Connection connection;
    switch (impl_->communication_type) {
    case CommunicationType::BROADCAST:
        connection = qMakePair(QHostAddress::Broadcast,
                               impl_->settings.broadcast_port);
        break;
    case CommunicationType::MULTICAST:
        connection = qMakePair(impl_->settings.multicast_host,
                               impl_->settings.multicast_port);
        break;
    case CommunicationType::UNICAST:
        connection = qMakePair(impl_->settings.unicast_host_send_to,
                               impl_->settings.unicast_port_send_to);
        break;
    default:
        CHECK(false);
        break;
    }

    impl_->cached_params_message.heartbeat_connection_params = connection;
    impl_->udp_socket->setConnection(connection);
    impl_->heartbeat_mode = status;
    bind_socket(type);
}

void HeartbeatCore::check_backup_timer(int backup_id)
{
    auto backup_timer_it = impl_->timers.backup_dead_timers.find(backup_id);
    CHECK(impl_->timers.backup_dead_timers.end() != backup_timer_it);
    auto& backup_timer_data = (*backup_timer_it).second;
    if (not backup_timer_data.is_connection_to_backup_established) {
        backup_timer_data.is_connection_to_backup_established = true;

        emit connectionToBackupEstablished(backup_id);
        impl_->udp_socket->send(HeartbeatCommand::HEARTBEAT_PARAMETERS_UPDATE,
                                impl_->cached_params_message);
    }
}

void HeartbeatCore::primary_death()
{
    stopService();
    emit primaryDied();

    VLOG(5) << "[HeartbeatCore] primaryDeath slot. Signal primaryDied emmited";
}

void HeartbeatCore::check_primary()
{
    impl_->is_link_between_controllers_down = true;
    impl_->timers.stopTimer(TimerType::PRIMARY_WAITING_TIMER);

    if (not impl_->is_connection_to_primary_established) {
        emit modeChangedToPrimary();
        start_transmitting(impl_->communication_type);

        VLOG(5) << "[HeartbeatCore] checkPrimary slot. "
                   "Signal setupPrimaryMode emmited";
    }
}

void HeartbeatCore::backup_death(int backup_id)
{
    VLOG(3) << "[HeartbeatCore] backupDeath slot raised. backup_id="
            << backup_id;

    impl_->timers.stopTimer(TimerType::BACKUP_DEAD, backup_id);
    emit backupDied(backup_id);

    VLOG(3) << "[HeartbeatCore] backupDeath slot. Signal backupDied emmited";
}

void HeartbeatCore::ready_read_handler()
{
    while (impl_->udp_socket->hasPendingDatagrams()) {
        auto datagram = impl_->udp_socket->receiveData();

        QDataStream stream(datagram);
        stream.startTransaction();
        uint16_t cmd_code = 0;
        stream >> cmd_code;

        auto cmd = HeartbeatCommand::_from_integral(cmd_code);
        switch (cmd) {
        case HeartbeatCommand::HEARTBEAT_ECHO_REQUEST:
            process_message_request(stream);
            break;

        case HeartbeatCommand::HEARTBEAT_ECHO_REPLY:
            process_message_reply(stream);
            break;

        case HeartbeatCommand::HEARTBEAT_PARAMETERS_UPDATE:
            process_message_params(stream);
            break;

        default:
            CHECK(false);
            break;
        }

        if (not stream.commitTransaction()) {
            LOG(ERROR) << "[RecoveryManager] Heartbeat problem - "
                          "Error during message receiving."
                          "QDataStream transaction error";
            return;
        }
    }
}

void HeartbeatCore::process_message_request(QDataStream& stream)
{
    if (+ControllerStatus::BACKUP == impl_->heartbeat_mode) {
        EchoMessage msg;
        stream >> msg;
        
        VLOG(5) << "[RecoveryManager] Heartbeat message REQUEST received"
                << ", unique_node_id=" << msg.unique_node_id
                << ", number=" << msg.message_number;
		
        // Don't receive message from myself
        if (msg.unique_node_id == impl_->unique_node_id && msg.hb_start_time == impl_->hb_start_time) {
            return;
        }
        
        // FATAL ERROR Controller ID is not unique
        if (msg.unique_node_id == impl_->unique_node_id && msg.hb_start_time != impl_->hb_start_time) {
            
            LOG(FATAL) << "[RecoveryManager] FATAL ERROR! Controller ID is not unique! A controller with ID =" 
                       << msg.unique_node_id 
                       << " already exists in the cluster. "
                       << "Please, change ID in the network settings config file.";
        }

        impl_->timers.startTimer(TimerType::PRIMARY_DEAD);
        impl_->is_link_between_controllers_down = false;

        // set up primaty node id
        if (DEFAULT_ID == impl_->primary_node_id) {
            impl_->primary_node_id = msg.unique_node_id;
        }

        impl_->udp_socket->send(HeartbeatCommand::HEARTBEAT_ECHO_REPLY,
            EchoMessage{impl_->unique_node_id, impl_->hb_start_time, msg.message_number});

        if (not impl_->is_connection_to_primary_established or
                msg.unique_node_id != impl_->primary_node_id) {
            impl_->primary_node_id = msg.unique_node_id;
            impl_->is_connection_to_primary_established = true;
            emit connectionToPrimaryEstablished(msg.unique_node_id);

            impl_->udp_socket->send(HeartbeatCommand::HEARTBEAT_PARAMETERS_UPDATE,
                                    impl_->cached_params_message);
        }
    }
}

void HeartbeatCore::process_message_reply(QDataStream& stream)
{
    EchoMessage msg;
    stream >> msg;

    VLOG(5) << "[RecoveryManager] Heartbeat message REPLY received"
            << ", unique_node_id=" << msg.unique_node_id
            << ", number=" << msg.message_number;

    // Don't receive message from myself
    if (msg.unique_node_id == impl_->unique_node_id) {
        return;
    }

    // REPLY message has come only from backup timer
    // Add it and start or just restart
    impl_->is_link_between_controllers_down = false;
    impl_->timers.startTimer(TimerType::BACKUP_DEAD, msg.unique_node_id);
    check_backup_timer(msg.unique_node_id);
}

void HeartbeatCore::process_message_params(QDataStream& stream)
{
    ParamsMessage msg;
    stream >> msg;

    if (msg.unique_node_id == impl_->unique_node_id) {
        return;
    }

    emit paramsChanged(msg);
}

void HeartbeatCore::linkDown()
{
    impl_->is_link_between_controllers_down = true;
    VLOG(5) << "[RecoveryManager] Heartbeat - linkDown slot raised";
}

void HeartbeatCore::dbParamsChanged(const std::string &db_ip, int db_port)
{
    VLOG(10) << "[HeartbeatCore] Database params changed. Db ip=" << db_ip
             << ", db port=" << db_port;

    impl_->cached_params_message.db_connection_params =
            qMakePair(toHostAddress(db_ip), db_port);

    impl_->udp_socket->send(HeartbeatCommand::HEARTBEAT_PARAMETERS_UPDATE,
                            impl_->cached_params_message);
}

// HeartbeatCore::implementation
HeartbeatCore::implementation::implementation(QObject *parent)
    : timers(parent)
    , udp_socket(new HeartbeatSocket(parent))
{
}

// Timers implementation
Timers::Timers(QObject *parent)
    : backup_timer_signal_mapper(new QSignalMapper(parent))
    , heartbeat_timer(new QTimer(parent))
    , primary_dead_timer(new QTimer(parent))
    , primary_waiting_timer(new QTimer(parent))
{
}

void Timers::setInterval(TimerType type, std::chrono::milliseconds ms)
{
    switch(type) {
    case TimerType::HEARTBEAT:
        heartbeat_timer->setInterval(ms);
        break;
    case TimerType::PRIMARY_DEAD:
        primary_dead_timer->setInterval(ms);
        break;
    case TimerType::PRIMARY_WAITING_TIMER:
        primary_waiting_timer->setInterval(ms);
        break;
    case TimerType::BACKUP_DEAD:
        backup_dead_interval = ms;
        break;
    default:
        CHECK(false);
    }
}

void Timers::startTimer(TimerType type, qint32 backup_node_id)
{
    switch(type) {
    case TimerType::HEARTBEAT:
        heartbeat_timer->start();
        break;
    case TimerType::PRIMARY_DEAD:
        primary_dead_timer->start();
        break;
    case TimerType::PRIMARY_WAITING_TIMER:
        primary_waiting_timer->start();
        break;
    case TimerType::BACKUP_DEAD:
    {
        auto backup_timer_it = backup_dead_timers.find(backup_node_id);
        if (backup_dead_timers.end() == backup_timer_it) {
            // add new to container
            auto timer = new QTimer();
            std::unique_ptr<QTimer, decltype(deleter)> timer_ptr(timer, deleter);
            auto emplace_pair = backup_dead_timers.emplace(
                backup_node_id,
                BackupTimerData({ false, std::move(timer_ptr) }));
            // connect to signal mapper slot
            QObject::connect(timer, &QTimer::timeout,
                             backup_timer_signal_mapper,
                             qOverload<>(&QSignalMapper::map));
            backup_timer_signal_mapper->setMapping(timer, backup_node_id);
            backup_timer_it = emplace_pair.first;
        }
        auto& backup_timer_data = (*backup_timer_it).second;
        backup_timer_data.timer->start(backup_dead_interval);
    }
        break;
    default:
        // logical error for TimerType::ALL and for others
        CHECK(false);
    }
}

void Timers::stopTimer(TimerType type, qint32 backup_node_id)
{
    switch(type) {
    case TimerType::HEARTBEAT:
        heartbeat_timer->stop();
        break;
    case TimerType::PRIMARY_DEAD:
        primary_dead_timer->stop();
        break;
    case TimerType::PRIMARY_WAITING_TIMER:
        primary_waiting_timer->stop();
        break;
    case TimerType::BACKUP_DEAD:
        stop_backup_timer(backup_node_id);
        break;
    case TimerType::ALL:
        heartbeat_timer->stop();
        primary_dead_timer->stop();
        primary_waiting_timer->stop();
        stop_backup_timer();
        break;
    default:
        // logical error for TimerType::ALL
        CHECK(false);
    }
}

void Timers::stop_backup_timer(qint32 unique_node_id)
{
    static auto stop_timer = [](BackupTimerData& backup_timer_data)
    {
        backup_timer_data.timer->stop();
        backup_timer_data.is_connection_to_backup_established = false;
    };

    if (DEFAULT_ID == unique_node_id) {
        // stop all timers
        for (auto& pair : backup_dead_timers) {
            auto& backup_timer_data = pair.second;
            stop_timer(backup_timer_data);

            VLOG(3) << "[RecoveryManager] HeartbeatCore - "
                       "All backup timers stopped";
        }
    } else {
        // stop the only one timer
        auto backup_timer_it = backup_dead_timers.find(unique_node_id);
        CHECK(backup_dead_timers.end() != backup_timer_it);
        auto& backup_timer_data = (*backup_timer_it).second;
        stop_timer(backup_timer_data);

        VLOG(3) << "[RecoveryManager] HeartbeatCore - Backup timer with id="
                << unique_node_id << " stopped";
    }
}

#include "heartbeatcore.moc"
