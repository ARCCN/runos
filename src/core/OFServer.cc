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

#include "OFServer.hpp"
#include "DpidChecker.hpp"

#include "lib/qt_executor.hpp"
#include "OFMessage.hpp"
#include "OFAgentImpl.hpp"

#include <runos/core/logging.hpp>
#include <runos/core/assert.hpp>
#include <runos/core/catch_all.hpp>
#include <runos/core/future.hpp>

#include <fluid/OFServer.hh>
#include <fluid/OFConnection.hh>
#include <fluid/TLS.hh>
#include <fluid/of13msg.hh>

#include <boost/assert.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/endian/arithmetic.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <tuple>
#include <iterator>
#include <utility>
#include <functional>
#include <list>
#include <thread>

// for msg_limiter
#include <chrono>
#include <queue>

namespace runos {

using namespace boost::endian;

static constexpr std::chrono::milliseconds PRINT_LOGGING_TIMEOUT_MSEC(10000); // ms

REGISTER_APPLICATION(OFServer, {"dpid-checker", ""})

typedef fluid_base::OFConnection FluidConnection;
namespace of13 = fluid_msg::of13;

namespace limiter {
    using valid_through_t =
        std::chrono::time_point<std::chrono::steady_clock>;
    using t_container =
        std::queue<valid_through_t>;
};

struct message_limiter {
    message_limiter() = delete;
    message_limiter(bool enabled, int max)
        : enabled(enabled), max_pps(max)
        {
            if (max <= 0) this->enabled = false;
        }

    bool enabled;
    unsigned int max_pps;
    std::unordered_map<int, limiter::t_container> limits;
};

struct fluid_conn_data {
    uint64_t dpid;

    static fluid_conn_data* get(FluidConnection* conn)
    {
        return reinterpret_cast<fluid_conn_data*>
            (conn->get_application_data());
    }
};

template<class Dispatcher>
class BroadcastSignal {
    using HandlerBase = typename Dispatcher::HandlerBase;
public:
    void connect(std::shared_ptr<HandlerBase> handler)
    {
        boost::unique_lock< boost::shared_mutex > lock(mutex);
        handlers_.push_back(handler);
    }

    //template<class Message, class... Args>
    //void dispatch(Message& msg, Args&&... args)
    //{
    //    boost::shared_lock< boost::shared_mutex > lock(mutex);

    //    for (auto& weak_handler : handlers_) {
    //        if (auto handler = weak_handler.lock()) {
    //            handler->dispatch(msg, std::forward<Args>(args)...);
    //        }
    //    }
    //}

    template<class... Args>
    void dispatch(typename Dispatcher::Dispatchable& dispatchable,
                  Args&&... args)
    {
        boost::shared_lock< boost::shared_mutex > lock(mutex);

        for (auto& weak_handler : handlers_) {
            if (auto handler = weak_handler.lock()) {
                catch_all_and_log([&]() {
                    dispatchable.dispatch(*handler,
                                          std::forward<Args>(args)...);
                });
            }
        }
    }

    void gc(); // TODO: delete invalid weak_ptr's from list
private:
    boost::shared_mutex mutex;
    std::list< std::weak_ptr<HandlerBase> > handlers_;
};

class OFConnectionImpl final : public OFConnection
                             , public std::enable_shared_from_this<OFConnectionImpl>
{
public:
    using OFConnection::ReceiveDispatch;

    explicit OFConnectionImpl(FluidConnection* fluid_conn, uint64_t dpid)
        : fluid_conn_(fluid_conn)
        , dpid_(dpid)
        , rx_of_packets_(0)
        , tx_of_packets_(0)
        , pkt_in_of_packets_(0)
    { }

    uint64_t dpid() const override
    {
        return dpid_;
    }

    std::string peer_address() const override
    {
        return fluid_conn_ ? fluid_conn_->get_peer_address() : std::string();
    }

    void fluid_conn(FluidConnection* fluidconn)
    {
        fluid_conn_ = fluidconn;
    }

    FluidConnection* fluid_conn() const
    {
        return fluid_conn_;
    }

    OFAgentPtr agent() const override
    {
        // Aliasing constructor
        return OFAgentImplPtr(shared_from_this(), const_cast<OFAgentImpl*>(&agent_));
    }

    bool alive() const override
    {
        if (not fluid_conn_)
        {
            return false;
        }
        return //fluid_conn_->is_alive() &&
               fluid_conn_->get_state() == FluidConnection::STATE_RUNNING;
    }

    void set_start_time() override
    {
        conn_start_time_ = std::chrono::system_clock::now();
    }

    void reset_stats() override
    {
        rx_of_packets_ = 0;
        tx_of_packets_ = 0;
        pkt_in_of_packets_ = 0;
    }

    std::chrono::system_clock::time_point get_start_time() const override
    {
        return conn_start_time_;
    }

    uint64_t get_rx_packets() const override
    {
        return rx_of_packets_;
    }

    uint64_t get_tx_packets() const override
    {
        return tx_of_packets_;
    }

    uint64_t get_pkt_in_packets() const override
    {
        return pkt_in_of_packets_;
    }

    void packet_in_counter() override
    {
        pkt_in_of_packets_++;
    }

    uint8_t protocol_version() const override
    {
        return fluid_conn_ ? fluid_conn_->get_version() : 0;
    }

    void send(const fluid_msg::OFMsg& cmsg) override
    {
        auto& msg = const_cast<fluid_msg::OFMsg&>(cmsg);
        if (not alive())
            return;

        auto dispatchable = make_dispatchable<SendHookDispatch>(msg);
        send_hook_sig_.dispatch(*dispatchable);

        auto deleter = &fluid_msg::OFMsg::free_buffer;
        std::unique_ptr<uint8_t[], decltype(deleter)> buf
            { msg.pack(), deleter };

        //std::thread([this,len = msg.length(), buf = std::move(buf)] {
        //LOG(ERROR)<<"-|- OFServer send("<<msg.length()<<")";
        fluid_conn_->send(buf.get(), msg.length());
        //LOG(ERROR)<<"--- OFServer OK("<<msg.length()<<")";
        //}).detach();
        tx_of_packets_++;
    }

    void send(void* msg, size_t size)
    {
        fluid_conn_->send(msg, size);
        tx_of_packets_++;

    }

    void on_receive(typename ReceiveDispatch::Dispatchable& dispatchable)
    {
        receive_sig_.dispatch(dispatchable);
        rx_of_packets_++;
    }

    void close() override
    {
        if (fluid_conn_) {
            fluid_conn_->close();
            fluid_conn_ = nullptr;
            tx_of_packets_ = 0;
            rx_of_packets_ = 0;
            pkt_in_of_packets_ = 0;
        }
    }

    void send_hook(SendHookHandlerPtr handler) override
    {
        send_hook_sig_.connect(handler);
    }

    void receive(ReceiveHandlerPtr handler) override
    {
        receive_sig_.connect(handler);
    }

private:
    FluidConnection* fluid_conn_;
    uint64_t dpid_;

    std::chrono::system_clock::time_point conn_start_time_;
    uint64_t rx_of_packets_;
    uint64_t tx_of_packets_;
    uint64_t pkt_in_of_packets_;

    BroadcastSignal< SendHookDispatch > send_hook_sig_;
    BroadcastSignal< ReceiveDispatch > receive_sig_;

    // Keep this field at the end because
    // OFAgentImpl ctor uses OFConnection, so it must
    // be at good state at this point.
    OFAgentImpl agent_{this};
};

typedef std::shared_ptr<OFConnectionImpl>
    OFConnectionImplPtr;
typedef std::weak_ptr<OFConnectionImpl>
    OFConnectionImplWeakPtr;

struct OFServer::implementation : fluid_base::OFServer
{
    typedef std::unordered_map<uint64_t, shared_future<OFConnectionImplPtr>>
        ConnectionMap;

    runos::OFServer& app;

    boost::inline_executor inline_executor;
    qt_executor executor;
    message_limiter limiter;

    class DpidChecker* dpid_checker;

    std::unordered_map<uint64_t, promise<OFConnectionImplPtr>>
        connection_promises;
    std::unordered_map<uint64_t, shared_future<OFConnectionImplPtr>>
        connection_futures;

    mutable boost::shared_mutex connections_mutex;
    std::unordered_map<uint64_t, OFConnectionImplPtr>
        connections;

    QTimer* defer_log_timer;
    std::unordered_map<uint64_t,uint64_t> connection_msgs_before_feature_reply;
    std::chrono::system_clock::time_point ctrl_start_time_;

    shared_future<OFConnectionImplPtr> get_connection_future(uint64_t dpid);

    implementation(runos::OFServer& app,
                   class DpidChecker * checker,
                   const char *address,
                   const int port,
                   const int nthreads = 4,
                   const bool secure = false,
                   const bool limits = false,
                   const int max_pps = 500,
                   const class fluid_base::OFServerSettings ofsc
                        = fluid_base::OFServerSettings())
            : fluid_base::OFServer(address, port, nthreads, secure, ofsc)
            , app(app)
            , executor(&app)
            , limiter(limits, max_pps)
            , dpid_checker(checker)
            , defer_log_timer(new QTimer(&app))
    {
        // log attempt connection
        defer_log_timer->setInterval(PRINT_LOGGING_TIMEOUT_MSEC);
        auto defer_log_timeout_lambda = [this]() {
            for (auto pair : connection_msgs_before_feature_reply) {
                LOG(INFO) << "[OFServer] Switch with dpid=" << pair.first
                    << " sent " << pair.second
                    << " messages before FEATURES_REPLY during the last "
                    << PRINT_LOGGING_TIMEOUT_MSEC.count()<< " ms";
            }
            connection_msgs_before_feature_reply.clear();
            if (defer_log_timer->isActive()) {
                QMetaObject::invokeMethod(defer_log_timer,
                                          "stop",
                                          Qt::QueuedConnection);
            }
        };
        QObject::connect(defer_log_timer,
                         &QTimer::timeout,
                         defer_log_timeout_lambda);

        auto close_connection_lambda = [this](uint64_t dpid) {
            async(executor, [this, dpid]() {
                auto conn = connections.find(dpid);
                if (connections.end() != conn) {
                    (*conn).second->close();
                    emit this->app.connectionDown(conn->second);
                    LOG(WARNING) << "[OFServer] Switch with dpid=" << dpid
                        << " has been removed from registered role list."
                        << " Connection is closed.";
                }
            });
        };
        QObject::connect(dpid_checker,
                         &DpidChecker::switchUnregistered,
                         close_connection_lambda);
    }

    OFConnectionImplPtr get_connection(FluidConnection *conn);

    void message_callback(FluidConnection *fluid_conn,
                          uint8_t type, void* data_, size_t len) override;

    void connection_callback(FluidConnection *conn,
                             FluidConnection::Event type) override;

    void print_error(of13::Error &msg, OFConnectionImplPtr conn);
    std::string flow_mod_failed_descr(uint16_t error_code);
    std::string group_mod_failed_descr(uint16_t error_code);
    std::string meter_mod_failed_descr(uint16_t error_code);
};

std::chrono::system_clock::time_point OFServer::get_start_time() const
{
    return impl->ctrl_start_time_;
}

uint64_t OFServer::get_rx_openflow_packets() const
{
    uint64_t ret = 0;

    for (const auto& conn : connections()) {
        ret += conn->get_rx_packets();
    }

    return ret;
}

uint64_t OFServer::get_tx_openflow_packets() const
{
    uint64_t ret = 0;

    for (const auto& conn : connections()) {
        ret += conn->get_tx_packets();
    }

    return ret;
}

uint64_t OFServer::get_pkt_in_openflow_packets() const
{
    uint64_t ret = 0;

    for (const auto& conn : connections()) {
        ret += conn->get_pkt_in_packets();
    }

    return ret;
}

shared_future<OFConnectionImplPtr>
OFServer::implementation::get_connection_future(uint64_t dpid)
{
    auto it = connection_futures.find(dpid);
    if (it == connection_futures.end()) {
        CHECK(connection_promises.count(dpid) == 0);

        promise<OFConnectionImplPtr> p;
        auto f = p.get_future().share();

        connection_promises.emplace(dpid, std::move(p));
        connection_futures.emplace(dpid, f);

        return f;
    } else {
        return it->second;
    }
}

OFConnectionImplPtr
OFServer::implementation::get_connection(FluidConnection *conn)
{
    if (auto conn_data = fluid_conn_data::get(conn)) {
        auto dpid = conn_data->dpid;

        boost::upgrade_lock< boost::shared_mutex > rlock{connections_mutex};
        auto it = connections.find(dpid);
        auto ret = (it != connections.end()) ? it->second : nullptr;

        if (ret) {
            // Switch has been seen before

            rlock.unlock();
            CHECK(ret->dpid() == dpid);

            if (ret->fluid_conn() != conn) {
                if (ret->alive()) {
                    LOG(ERROR) << "Duplicate dpid (" << dpid << ")"
                        << " detected between connections "
                        << ret->fluid_conn()->get_id()
                        << " and " << conn->get_id();
                    conn->close();
                } else {
                    ret->fluid_conn(conn);
                    ret->set_start_time();
                    emit app.connectionUp(ret);
                }
            }
        } else {
            // Create new OFConnection
            ret = std::make_shared<OFConnectionImpl>(conn, dpid);
            {
                boost::upgrade_to_unique_lock< boost::shared_mutex > wlock{rlock};
                connections.emplace(dpid, ret);
            }
            rlock.unlock();

            emit app.switchDiscovered(ret);
            ret->set_start_time();
            emit app.connectionUp(ret);

            async(executor, [dpid,ret,this]() {
                auto it = connection_promises.find(dpid);
                if (it != connection_promises.end()) {
                    it->second.set_value(ret);
                    connection_promises.erase(dpid);
                } else {
                    auto f = make_ready_future(ret).share();
                    connection_futures.emplace(dpid, std::move(f));
                }
            });
        }

        return ret;
    } else {
        // feature-reply hasn't been received yet
        return nullptr;
    }
}

void
OFServer::implementation::message_callback(FluidConnection *fluid_conn,
                                           uint8_t type,
                                           void* data_,
                                           size_t len)
{ catch_all_and_log([&]() {
    auto deleter = [this](void* ptr){ free_data(ptr); };
    std::unique_ptr<void, decltype(deleter)> data {data_, deleter};

    struct exthdr {
        big_uint8_t version;
        big_uint8_t type;
        big_uint16_t length;
        big_uint32_t xid;
        big_uint16_t mpart_type;
    };

    uint16_t mpart = 0xffff;
    if (len >= sizeof(exthdr)) {
        auto hdr = reinterpret_cast<exthdr*>(data_);
        mpart = hdr->mpart_type;
        CHECK(type == hdr->type);
    }

    auto dispatchable
        = make_dispatchable<OFConnectionImpl::ReceiveDispatch>(type, mpart);
    auto& msg
        = dynamic_cast<fluid_msg::OFMsg&>(*dispatchable);

    if (msg.unpack((uint8_t*) data_) != 0) {
        LOG(WARNING) << "[OFServer] message_callback - Malformed "
            "message received from connection " << fluid_conn->get_id();
        return;
    }

    if (type == of13::OFPT_FEATURES_REPLY) {
        auto& fr = dynamic_cast<of13::FeaturesReply&>(msg);
        auto dpid = fr.datapath_id();

        if (not dpid_checker->isRegistered(dpid)) {
            auto it = connection_msgs_before_feature_reply.find(dpid);
            if (connection_msgs_before_feature_reply.end() == it) {
                // print log for dpid for the first time
                LOG(ERROR) << "[OFServer] message_callback - Role of switch "
                    "with dpid=" << dpid << " is undefined. Connection with id="
                    << fluid_conn->get_id() << " dropped";
                // initialize connection attempts
                connection_msgs_before_feature_reply.insert(std::make_pair(dpid, 1));
            } else {
                // increase connection attempts
                it->second++;
            }
            if (not defer_log_timer->isActive()) {
                QMetaObject::invokeMethod(defer_log_timer, "start", Qt::QueuedConnection);
            }
            fluid_conn->close();
            return;
        }

        if (auto conn_data = fluid_conn_data::get(fluid_conn)) {
            CHECK(conn_data->dpid == dpid);
        } else {
            if (limiter.enabled && limiter.limits.count(fluid_conn->get_id()) == 0)
                limiter.limits.emplace(fluid_conn->get_id(), limiter::t_container());
            fluid_conn->set_application_data(new fluid_conn_data {dpid});
            LOG(INFO) << "Connection id=" << fluid_conn->get_id()
                      << " ends on switch dpid=" << dpid;
        }
    }

    // print verbose message for flow mod error, group mod error and meter mod error
    try {
        if ((of13::OFPT_ERROR == type) && (msg.xid() < OFAgentImpl::get_minimal_xid())) {
            auto& error_msg = dynamic_cast<of13::Error&>(msg);
            this->print_error(error_msg, get_connection(fluid_conn));
        }
    } catch(const std::bad_cast& e) {
        LOG(ERROR) << "[OFServer] Error message received - Bad cast message to of13::Error. What="
                   << e.what();
        throw;
    }

    // Is used for limiting OFMsg/sec from switches
    if (limiter.enabled) {
        auto& limits = limiter.limits[fluid_conn->get_id()];
        auto now = std::chrono::steady_clock::now();

        // clear outdated message's timepoints (older than 1 sec)
        while (not limits.empty() &&
               now - limits.front() > std::chrono::seconds(1))
            limits.pop();

        // drop exceeded packet (when timepoint's container is full)
        if (limits.size() > limiter.max_pps) {
            VLOG(6) << "Drop message (" << (unsigned) msg.type() << ") "
                       << "from connection id=" << fluid_conn->get_id();
            return;
        }

        // add timepoint to container
        limits.push(now);
    }

    if (auto conn = get_connection(fluid_conn)) {
        // TODO: catch exceptions inside signal
        conn->on_receive(*dispatchable);
        if (type == of13::OFPT_PACKET_IN) {
            conn->packet_in_counter();
        }
    }
} ); }

void
OFServer::implementation::connection_callback(FluidConnection *conn,
                                              FluidConnection::Event type)
{
    switch (type) {
    case FluidConnection::EVENT_STARTED:
        VLOG(3) << "Connection id=" << conn->get_id() << " from "
                << conn->get_peer_address() << " started";
        conn->set_application_data(nullptr);
    break;
    case FluidConnection::EVENT_ESTABLISHED:
        VLOG(3) << "Connection id=" << conn->get_id() << " from "
                << conn->get_peer_address() <<  " established";
    break;
    case FluidConnection::EVENT_FAILED_NEGOTIATION:
        VLOG(3) << "Connection id=" << conn->get_id() << " from "
                << conn->get_peer_address() << ": failed version negotiation";
    break;
    case FluidConnection::EVENT_CLOSED:
        VLOG(3) << "Connection id=" << conn->get_id() << " from "
                << conn->get_peer_address() << " closed by the user";
        if (auto ofconn = get_connection(conn)) {
            emit app.connectionDown(ofconn);
        }
    break;
    case FluidConnection::EVENT_DEAD:
        VLOG(3) << "Connection id=" << conn->get_id() << " from "
                << conn->get_peer_address() << " closed due to inactivity";
        if (auto ofconn = get_connection(conn)) {
            emit app.connectionDown(ofconn);
        }
    break;
    }
}

OFServer::OFServer() = default;
OFServer::~OFServer() = default;

/* Application interface */
void OFServer::init(Loader* loader, const Config& rootConfig)
{
    const Config& config = config_cd(rootConfig, "of-server");

    // If secure mode
    if(config_get(config, "secure", false))
    {
        LOG(WARNING)<<"    TLS mode - ON";
        LOG(WARNING)<<"    Controller public key  = "<<config_get(config, "ctl-cert", "").c_str();
        LOG(WARNING)<<"    Controller private key = "<<config_get(config, "ctl-privkey", "").c_str();
        LOG(WARNING)<<"    Switch certificate     = "<<config_get(config, "cacert", "").c_str();

        // Init controller private/public key and switch certificate
        fluid_base::libfluid_tls_init(config_get(config, "ctl-cert", "").c_str(),
                                      config_get(config, "ctl-privkey", "").c_str(),
                                      config_get(config, "cacert", "").c_str());
    }

    impl.reset(new implementation{
            *this,
            DpidChecker::get(loader),
            config_get(config, "address", "0.0.0.0").c_str(),
            config_get(config, "port", 6653),
            config_get(config, "nthreads", 4),
            config_get(config, "secure", false),
            config_get(config, "limiter", false),
            config_get(config, "max_pps", 500),
            fluid_base::OFServerSettings()
                    .supported_version(of13::OFP_VERSION)
                    .keep_data_ownership(false)
                    .echo_interval(config_get(config, "echo-interval", 5))
                    .echo_attempts(config_get(config, "echo-attempts", 3))
                    .liveness_check(config_get(config, "liveness-check", true))
    });
}

void OFServer::startUp(Loader*)
{
    impl->start(/* block: */ false);
    impl->ctrl_start_time_ = std::chrono::system_clock::now();
}

future<OFConnectionPtr> OFServer::connection(uint64_t dpid) const
{
    {
        boost::shared_lock< boost::shared_mutex > rlock(impl->connections_mutex);
        auto& connections = impl->connections;
        auto it = connections.find(dpid);
        if (it != connections.end()) {
            auto ofconn = std::static_pointer_cast<OFConnection>(it->second);
            return make_ready_future(std::move(ofconn));
        }
    }

    return async(impl->executor,
        [=]() {
            return impl->get_connection_future(dpid).then(impl->inline_executor,
                [=](shared_future<OFConnectionImplPtr> conn) -> OFConnectionPtr {
                    return conn.get(); // downcast
            });
        }).get();
}

future<OFAgentPtr> OFServer::agent(uint64_t dpid) const
{
    return connection(dpid).then([this](future<OFConnectionPtr> conn) {
        return conn.get()->agent();
    });
}

std::vector<OFConnectionPtr> OFServer::connections() const
{
    boost::shared_lock< boost::shared_mutex > rlock(impl->connections_mutex);
    std::vector<OFConnectionPtr> ret;
    boost::copy(impl->connections | boost::adaptors::map_values,
                std::back_inserter(ret));
    return ret;
}

std::string OFServer::implementation::flow_mod_failed_descr(uint16_t error_code)
{
    std::string error_description;
    switch(error_code) {
        case of13::OFPFMFC_UNKNOWN:
            error_description = "Unspecified error";
            break;
        case of13::OFPFMFC_TABLE_FULL:
            error_description = "Flow not added because table was full";
            break;
        case of13::OFPFMFC_BAD_TABLE_ID:
            error_description = "Table does not exist";
            break;
        case of13::OFPFMFC_OVERLAP:
            error_description = "Attempted to add overlapping "
                                "flow with CHECK_OVERLAP flag set";
            break;
        case of13::OFPFMFC_EPERM:
            error_description = "Permissions error";
            break;
        case of13::OFPFMFC_BAD_TIMEOUT:
            error_description = "Flow not added because of "
                                "unsupported idle/hard timeout";
            break;
        case of13::OFPFMFC_BAD_COMMAND:
            error_description = "Unsupported or unknown command";
            break;
        case of13::OFPFMFC_BAD_FLAGS:
            error_description = "Unsupported or unknown flags";
            break;
        default:
            error_description = "Undefined error code=" + error_code;
            break;
    }
    return error_description;
}

std::string OFServer::implementation::group_mod_failed_descr(uint16_t error_code)
{
    std::string error_description;
    switch (error_code) {
        case of13::OFPGMFC_GROUP_EXISTS:
            error_description = "Group not added because a group ADD "
                                "attempted to replace an already-present group";
            break;
        case of13::OFPGMFC_INVALID_GROUP:
            error_description = "Invalid group";
            break;
        case of13::OFPGMFC_OUT_OF_GROUPS:
            error_description = "The group table is full";
            break;
        case of13::OFPGMFC_OUT_OF_BUCKETS:
            error_description = "The maximum number of action buckets "
                                "for a group has been exceeded";
            break;
        case of13::OFPGMFC_CHAINING_UNSUPPORTED:
            error_description = "Switch does not support "
                                "groups that forward to groups";
            break;
        case of13::OFPGMFC_WATCH_UNSUPPORTED:
            error_description = "This group cannot watch the "
                                "watch_port or watch_group specified";
            break;
        case of13::OFPGMFC_LOOP:
            error_description = "Group entry would cause a loop";
            break;
        case of13::OFPGMFC_UNKNOWN_GROUP:
            error_description = "Group not modified because a group MODIFY "
                                "attempted to modify a non-existent group";
            break;
        case of13::OFPGMFC_CHAINED_GROUP:
            error_description = "Group not deleted because another "
                                "group is forwarding to it";
            break;
        case of13::OFPGMFC_BAD_TYPE:
            error_description = "Unsupported or unknown group type";
            break;
        case of13::OFPGMFC_BAD_COMMAND:
            error_description = "Unsupported or unknown command";
            break;
        case of13::OFPGMFC_BAD_BUCKET:
            error_description = "Error in bucket";
            break;
        case of13::OFPGMFC_BAD_WATCH:
            error_description = "Error in watch port/group";
            break;
        case of13::OFPGMFC_EPERM:
            error_description = "Permissions error";
            break;
        default:
            error_description = "Undefined error code=" + error_code;
            break;
    }
    return error_description;
}

std::string OFServer::implementation::meter_mod_failed_descr(uint16_t error_code)
{
    std::string error_description;
    switch(error_code) {
        case of13::OFPMMFC_UNKNOWN:
            error_description = "Unspecified error";
            break;
        case of13::OFPMMFC_METER_EXISTS:
            error_description = "Meter not added because a Meter ADD "
                                "attempted to replace an existing Meter";
            break;
        case of13::OFPMMFC_INVALID_METER:
            error_description = "Meter not added because "
                                "Meter specified is invalid";
            break;
        case of13::OFPMMFC_UNKNOWN_METER:
            error_description = "Meter not modified because a Meter MODIFY "
                                "attempted to modify a non-existent Meter";
            break;
        case of13::OFPMMFC_BAD_COMMAND:
            error_description = "Unsupported or unknown command";
            break;
        case of13::OFPMMFC_BAD_FLAGS:
            error_description = "Flag configuration unsupported";
            break;
        case of13::OFPMMFC_BAD_RATE:
            error_description = "Rate unsupported";
            break;
        case of13::OFPMMFC_BAD_BURST:
            error_description = "Burst size unsupported";
            break;
        case of13::OFPMMFC_BAD_BAND:
            error_description = "Band unsupported";
            break;
        case of13::OFPMMFC_BAD_BAND_VALUE:
            error_description = "Band value unsupported";
            break;
        case of13::OFPMMFC_OUT_OF_METERS:
            error_description = "No more meters available";
            break;
        case of13::OFPMMFC_OUT_OF_BANDS:
            error_description = "The maximum number of properties "
                                "for a meter has been exceeded";
            break;
        default:
            error_description = "Undefined error code=" + error_code;
            break;
    }
    return error_description;
}

void OFServer::implementation::print_error(of13::Error& error_msg, OFConnectionImplPtr conn)
{
    std::string error_type_str;
    std::string error_description_str;
    const auto error_type = error_msg.err_type();
    const auto error_code = error_msg.code();
    std::string switch_description = conn
            ? "from switch with dpid=" + std::to_string(conn->dpid())
            : "from undefined switch";

    switch (error_type) {
        case of13::OFPET_FLOW_MOD_FAILED:
            error_type_str = "FLOW_MOD_FAILED";
            error_description_str = this->flow_mod_failed_descr(error_code);
            break;
        case of13::OFPET_GROUP_MOD_FAILED:
            error_type_str = "GROUP_MOD_FAILED";
            error_description_str = this->group_mod_failed_descr(error_code);
            break;
        case of13::OFPET_METER_MOD_FAILED:
            error_type_str = "METER_MOD_FAILED";
            error_description_str = this->meter_mod_failed_descr(error_code);
            break;
        default:
            LOG(ERROR) << "[OFServer] Error message received "
                       << switch_description << ". Type=" << error_type
                       << ", description=" << error_code;
            return;
    }
    LOG(ERROR) << "[OFServer] Error message received " << switch_description
               << ". Type=" << error_type_str
               << ", description=" << error_description_str;
}

} // namespace runos
