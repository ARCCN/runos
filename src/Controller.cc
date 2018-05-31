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

#include "Controller.hh"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <sstream>
#include <memory>
#include <functional>

#include <boost/assert.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/get.hpp>
#include <fluid/OFServer.hh>

#include "types/exception.hh"

#include "OFMsgUnion.hh"
#include "SwitchConnection.hh"


using namespace std::placeholders;
using namespace runos;
using namespace fluid_base;

REGISTER_APPLICATION(Controller, {""})

class SwitchConnectionImpl : public SwitchConnection {
public:
    SwitchConnectionImpl(OFConnection* ofconn_, uint64_t dpid)
        : SwitchConnection(ofconn_, dpid)
    { }

    void replace(OFConnection* ofconn_)
    { m_ofconn = ofconn_; }
};

typedef std::shared_ptr<SwitchConnectionImpl> SwitchConnectionImplPtr;
typedef std::weak_ptr<SwitchConnectionImpl> SwitchConnectionImplWeakPtr;

struct SwitchBase {
    SwitchConnectionImplPtr connection;
    uint8_t max_table;

public:
    SwitchBase(OFConnection* ofconn,
            uint64_t dpid,
            uint8_t max_table)
        : connection{ new SwitchConnectionImpl{ofconn, dpid} },
        max_table(max_table)
    {
        clearTables();
        for (uint8_t i = 0; i < max_table; ++i){
            installGoto(i);
        }
        installTableMiss(max_table);
    }

    void reinit() {
        clearTables();
        for (uint8_t i = 0; i < max_table; i++) {
            installGoto(i);
        }
        installTableMiss(max_table);
    }

private:

    void barrier()
    {
        connection->send(of13::BarrierRequest());
    }

    void clearTables()
    {
        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);
        fm.table_id(of13::OFPTT_ALL);
        fm.cookie(0x0);
        fm.cookie_mask(0x0);
        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        connection->send(fm);
    }

    void installGoto(uint8_t table)
    {
        barrier();
        of13::FlowMod fm;
        fm.command(of13::OFPFC_ADD);
        fm.buffer_id(OFP_NO_BUFFER);
        fm.priority(0);
        fm.cookie(0);
        fm.idle_timeout(0);
        fm.hard_timeout(0);
        fm.table_id(table);
        fm.flags(of13::OFPFF_CHECK_OVERLAP | of13::OFPFF_SEND_FLOW_REM);
        of13::GoToTable go_to_table(table + 1);
        fm.add_instruction(go_to_table);

        connection->send(fm);
    }

    void installTableMiss(uint8_t table)
    {
        barrier();
        of13::FlowMod fm;
        fm.command(of13::OFPFC_ADD);
        fm.buffer_id(OFP_NO_BUFFER);
        fm.priority(0);
        fm.cookie(0);
        fm.idle_timeout(0);
        fm.hard_timeout(0);
        fm.table_id(table);
        fm.flags(of13::OFPFF_CHECK_OVERLAP | of13::OFPFF_SEND_FLOW_REM);
        of13::ApplyActions act;
        of13::OutputAction out(of13::OFPP_CONTROLLER, 128); // TODO : unhardcore
        act.add_action(out);
        fm.add_instruction(act);

        connection->send(fm);
    }

};


class ControllerImpl : public OFServer {
    const uint32_t min_xid = 0xfff;
    Controller &app;

public:
    bool started{false};
    bool cbench;
    Config config;
    uint8_t max_table;

    std::unordered_map<uint8_t, CommonHandlers*> handlers;
    std::unordered_map<uint64_t, SwitchBase> switches;

    // OFResponse
    std::vector<OFTransaction*> static_ofresponse;
    // Make sure that we don't intersect with libfluid_base
    uint32_t min_session_xid{min_xid};
    //uint32_t last_xid;

    ControllerImpl(Controller &_app,
            const char *address,
            const int port,
            const int nthreads = 4,
            const bool secure = false,
            const class OFServerSettings ofsc = OFServerSettings())
            : OFServer(address, port, nthreads, secure, ofsc),
              app(_app)
              // last_xid(min_xid)
    { }

    void message_callback(OFConnection *ofconn, uint8_t type, void *data, size_t len) override
    {
        if (cbench && type == of13::OFPT_PACKET_IN) {
            OFMsg pi(static_cast<uint8_t*>(data));
            of13::PacketOut po;
            po.xid(pi.xid());
            uint8_t* buffer;
            buffer = po.pack();
            ofconn->send(buffer, po.length());
            OFMsg::free_buffer(buffer);
            free_data(data);
            return;
        }


        SwitchBase *ctx = reinterpret_cast<SwitchBase *>(ofconn->get_application_data());

        auto it = handlers.find(type);
        if( it != handlers.end()){
            it->second->apply(data, ctx->connection);
        }


        if (ctx == nullptr && type != of13::OFPT_FEATURES_REPLY) {
            LOG(WARNING) << "Switch send message before feature reply";
            OFMsg::free_buffer(static_cast<uint8_t *>(data));
            return;
        }

        try {
            OFMsgUnion msg(type, data, len);

            switch (type) {
            case of13::OFPT_FEATURES_REPLY:
                ctx = createSwitchBase(ofconn, msg.featuresReply.datapath_id());
                ofconn->set_application_data(ctx);
                emit app.switchUp(ctx->connection, msg.featuresReply);
                break;
            case of13::OFPT_PORT_STATUS:
                if (ctx == nullptr) break;
                emit app.portStatus(ctx->connection, msg.portStatus);
                break;
            case of13::OFPT_FLOW_REMOVED:
                emit app.flowRemoved(ctx->connection, msg.flowRemoved);
                break;
            default: {
               uint32_t xid = msg.base()->xid();
                if (xid < min_xid)
                    break;

                OFTransaction *transaction = nullptr;
                if (xid < min_session_xid) {
                    transaction = static_ofresponse[xid - min_xid];
                } else {
                    // TODO: session ofresponse's
                }

                if (transaction) {
                    auto msg_copy = std::make_shared<OFMsgUnion>(type, data, len);

                    if (type == of13::OFPT_ERROR) {
                        emit transaction->error(ctx->connection, msg_copy);
                    } else {
                        emit transaction->response(ctx->connection, msg_copy);
                    }
                }
            }
            }
        } catch (const OFMsgParseError &e) {
            LOG(WARNING) << "Malformed message received from connection " << ofconn->get_id();
        } catch (const OFMsgUnhandledType &e) {
            LOG(WARNING) << "Unhandled message type " << e.msg_type()
                    << " received from connection " << ofconn->get_id();
        } catch (const std::exception &e) {
            LOG(ERROR) << "Unhandled exception: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Unhandled exception";
        }

        free_data(data);
    }

    void connection_callback(OFConnection *ofconn, OFConnection::Event type) override
    {
        auto ctx = reinterpret_cast<SwitchBase*>(ofconn->get_application_data());

        if (type == OFConnection::EVENT_STARTED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " started";
            ofconn->set_application_data(nullptr);
        }

        else if (type == OFConnection::EVENT_ESTABLISHED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " established";
        }

        else if (type == OFConnection::EVENT_FAILED_NEGOTIATION) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << ": failed version negotiation";
        }

        else if (type == OFConnection::EVENT_CLOSED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed by the user";
            if (ctx) {
                ofconn->set_application_data(nullptr);
                emit app.switchDown(ctx->connection);
                ctx->connection->replace(nullptr);
           }
        }

        else if (type == OFConnection::EVENT_DEAD) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed due to inactivity";
            if (ctx) {
                ofconn->set_application_data(nullptr);
                emit app.switchDown(ctx->connection);
                ctx->connection->replace(nullptr);
           }
        }
    }
private:
    SwitchBase *createSwitchBase(OFConnection *ofconn, uint64_t dpid)
    {

        auto it = switches.find(dpid);
        if (it != switches.end())
            goto ret;
        {
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock(mutex);

            it = switches.find(dpid);
            if (it != switches.end())
                goto ret;

            it = switches.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(dpid),
                                  std::forward_as_tuple(ofconn,
                                                        dpid,
                                                        max_table))
                          .first;
            return &it->second;
        }

        ret:
        SwitchBase *ctx = &it->second;
        if (ctx->connection->alive()) {
            LOG(ERROR) << "Overwriting switchscope on active connection";
        }
        ctx->connection->replace(ofconn);
        ctx->reinit();
        return ctx;
    }

};

/* Application interface */
void Controller::init(Loader*, const Config& rootConfig)
{
    const Config& config = config_cd(rootConfig, "controller");
    impl.reset(new ControllerImpl{
            *this,
            config_get(config, "address", "0.0.0.0").c_str(),
            config_get(config, "port", 6653),
            config_get(config, "nthreads", 4),
            config_get(config, "secure", false),
            OFServerSettings()
                    .supported_version(of13::OFP_VERSION)
                    .keep_data_ownership(false)
                    .echo_interval(config_get(config, "echo_interval", 15))
                    .liveness_check(config_get(config, "liveness_check", true))
    });
    impl->config = config;
    impl->max_table = config_get(config, "tables.max_table", 0);
}

void Controller::startUp(Loader*)
{
    impl->start(/* block: */ false);
    impl->started = true;
    impl->cbench = config_get(impl->config, "cbench", false);
}

void Controller::__register_handler__(uint8_t t, CommonHandlers* h)
{
    if (impl->started) {
        LOG(ERROR) << "Register handler after startup";
    }
    impl->handlers.emplace(t, h);
}

OFTransaction* Controller::registerStaticTransaction(Application *caller)
{
    if (impl->started) {
        LOG(ERROR) << "Registering static xid after startup";
        return 0;
    }

    uint32_t xid = impl->min_session_xid++;

    OFTransaction* ret = new OFTransaction(xid, caller);
    impl->static_ofresponse.push_back(ret);

    QObject::connect(ret, &QObject::destroyed, [xid, this]() {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        impl->static_ofresponse[xid] = 0;
    });

    return ret;
}

uint8_t Controller::getTable(const char* name) const
{
    auto config = config_cd(impl->config, "tables");
    return config_get(config, name, 0);
}

uint8_t Controller::maxTable() const
{
    return impl->max_table;
}

Controller::~Controller() = default;
