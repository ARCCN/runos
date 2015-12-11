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
#include <list>
#include <mutex>
#include <thread>
#include <sstream>
#include <memory>

#include <fluid/OFServer.hh>

#include "TraceTree.hh"
#include "Flow.hh"
#include "Packet.hh"
#include "OFMsgUnion.hh"

REGISTER_APPLICATION(Controller, {""})

typedef std::list< std::unique_ptr<OFMessageHandler> > HandlerPipeline;

// TODO: Can we implement similar SwitchStorage?
class SwitchScope {
public:
    TraceTree trace_tree;
    OFConnection* ofconn;
    HandlerPipeline pipeline;

    void processTableMiss(of13::PacketIn& pi);
    void processFlowRemoved(Flow* flow, uint8_t reason);
};

class ControllerImpl : public OFServer {
    const uint32_t min_xid = 0xff;
    Controller *app;

public:
    bool started;
    bool cbench;
    Config config;
    std::vector<OFMessageHandlerFactory *> pipeline_factory;
    std::unordered_map<uint64_t, SwitchScope> switch_scope;

    // OFResponse
    std::vector<OFTransaction *> static_ofresponse;
    uint32_t min_session_xid; // Make sure that we don't intersect with libfluid_base
    //uint32_t last_xid;

    ControllerImpl(Controller *_app,
            const char *address,
            const int port,
            const int nthreads = 4,
            const bool secure = false,
            const class OFServerSettings ofsc = OFServerSettings())
            : OFServer(address, port, nthreads, secure, ofsc),
              app(_app), started(false),
              min_session_xid(min_xid) // last_xid(min_xid)
    { }

    ~ControllerImpl()
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

        SwitchScope *ctx = reinterpret_cast<SwitchScope *>(ofconn->get_application_data());
        Flow* flow;

        if (ctx == nullptr && type != of13::OFPT_FEATURES_REPLY) {
            LOG(ERROR) << "Switch send message before feature reply";
            OFMsg::free_buffer(static_cast<uint8_t *>(data));
            return;
        }

        try {
            OFMsgUnion msg(type, data, len);

            switch (type) {
            case of13::OFPT_PACKET_IN:
                if (TraceTree::isTableMiss(msg.packetIn)) {
                    ctx->processTableMiss(msg.packetIn);
                } else {
                    LOG(ERROR) << "TODO: to-controller packet-ins"; // TODO
                }
                break;
            case of13::OFPT_FEATURES_REPLY:
                ctx = createSwitchScope(ofconn, msg.featuresReply.datapath_id());
                ofconn->set_application_data(ctx);
                emit app->switchUp(ctx->ofconn, msg.featuresReply);
                break;
            case of13::OFPT_PORT_STATUS:
                if (ctx == nullptr) break;
                emit app->portStatus(ctx->ofconn, msg.portStatus);
                break;
            case of13::OFPT_FLOW_REMOVED:
                if (ctx == nullptr) break;
                if (msg.flowRemoved.reason() == of13::OFPRR_DELETE) break;
                flow = ctx->trace_tree.find(msg.flowRemoved.cookie());
                ctx->processFlowRemoved(flow, msg.flowRemoved.reason());
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
                        emit transaction->error(ctx->ofconn, msg_copy);
                    } else {
                        emit transaction->response(ctx->ofconn, msg_copy);
                    }
                }
            }
            }
        } catch (const OFMsgParseError &e) {
            LOG(WARNING) << "Malformed message received from connection " << ofconn->get_id();
        } catch (const OFMsgUnhandledType &e) {
            LOG(WARNING) << "Unhandled message type " << e.msg_type()
                    << " received from connection " << ofconn->get_id();
        }

        free_data(data);
    }

    void connection_callback(OFConnection *ofconn, OFConnection::Event type) override
    {
        SwitchScope *ctx = reinterpret_cast<SwitchScope *>(ofconn->get_application_data());

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
                emit app->switchDown(ctx->ofconn);
                ctx->ofconn = nullptr;
                ctx->trace_tree.clear();
            }
        }

        else if (type == OFConnection::EVENT_DEAD) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed due to inactivity";
            if (ctx) {
                ofconn->set_application_data(nullptr);
                emit app->switchDown(ctx->ofconn);
                ctx->ofconn = nullptr;
                ctx->trace_tree.clear();
            }
        }
    }

    void sortPipeline()
    {
        for(size_t i = 0; i < pipeline_factory.size(); ++i) {
            for(size_t j = 1; j < pipeline_factory.size(); ++j) {
                const auto a = pipeline_factory[j - 1];
                const auto b = pipeline_factory[j];
                bool a_before_b = b->isPrereq(a->orderingName());
                bool a_after_b = b->isPostreq(a->orderingName());
                bool b_before_a = a->isPrereq(b->orderingName());
                bool b_after_a = a->isPostreq(b->orderingName());

                if ((a_before_b && a_after_b) ||
                       (b_before_a && b_after_a) ||
                       (a_before_b && b_before_a) ||
                       (a_after_b && b_after_a)) {
                           LOG(FATAL) << "Wrong pipeline ordering constraints between "
                               << a->orderingName() << " and " << b->orderingName();
                }

                if(!(a_before_b || b_after_a)) {
                    std::swap(pipeline_factory[j - 1], pipeline_factory[j]);
                }
            }
        }
/*
        std::sort(pipeline_factory.begin(), pipeline_factory.end(),
                  [](const OFMessageHandlerFactory *a, const OFMessageHandlerFactory *b) -> bool {
                      bool a_before_b = b->isPrereq(a->orderingName());
                      bool a_after_b = b->isPostreq(a->orderingName());
                      bool b_before_a = a->isPrereq(b->orderingName());
                      bool b_after_a = a->isPostreq(b->orderingName());

                      if ((a_before_b && a_after_b) ||
                              (b_before_a && b_after_a) ||
                              (a_before_b && b_before_a) ||
                              (a_after_b && b_after_a)) {
                          LOG(FATAL) << "Wrong pipeline ordering constraints between "
                                  << a->orderingName() << " and " << b->orderingName();
                      }

                      return a_before_b || b_after_a;
                  }
        );
*/
        LOG(INFO) << "Flow processors registered: ";
        for (auto &factory : pipeline_factory)
            LOG(INFO) << "  * " << factory->orderingName();
    }

    SwitchScope *createSwitchScope(OFConnection *ofconn, uint64_t dpid)
    {
        static std::mutex mutex;

        auto it = switch_scope.find(dpid);
        if (it != switch_scope.end())
            goto ret;

        mutex.lock();
        it = switch_scope.find(dpid);
        if (it != switch_scope.end()) {
            mutex.unlock();
            goto ret;
        }

        it = switch_scope.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(dpid),
                                  std::forward_as_tuple()).first;

        {
            auto& swctx = it->second;
            // Create pipeline
            for (auto &factory : pipeline_factory) {
                swctx.pipeline.push_back(std::move(factory->makeOFMessageHandler()));
            }
            swctx.trace_tree.cleanFlowTable(ofconn);
        }
        mutex.unlock();

        ret:
        SwitchScope *ctx = &it->second;
        if (ctx->ofconn != nullptr) {
            LOG(ERROR) << "Overwriting switch scope on active connection";
        }
        ctx->ofconn = ofconn;

        return ctx;
    }
};

void SwitchScope::processTableMiss(of13::PacketIn& pi)
{
    auto conn_id = ofconn->get_id();
    auto pkt     = new Packet(pi);
    auto leaf    = trace_tree.find(pkt);

    DVLOG(10) << "Table miss on connection id=" << conn_id;

    if (leaf == nullptr) {
        // The flow is not in the trace tree.
        // This means that it is new or outdated (deleted by hard timeout).

        // Create new Flow object and pass it through handlers.
        // Handlers will define:
        //  1) Flow space: which fields are used to make decision.
        //  2) Decision: forward to port, modify fields, drop.
        //  3) Timeout: how many time decision is valid.
        Flow* flow = new Flow(pkt);
        for (auto& handler : pipeline) {
            if (handler->processMiss(ofconn, flow) == OFMessageHandler::Stop)
                break;
        }

        if (flow->flags() & Flow::Disposable) {
            // Sometimes we don't need to create a new flow on the switch.
            // So, reply to the packet-in using packet-out message.
            DVLOG(9) << "Sending packet-out";

            of13::PacketOut po;
            po.xid(pi.xid());
            po.buffer_id(pi.buffer_id());
            if (pi.buffer_id() == OFP_NO_BUFFER)
                po.data(pi.data(), pi.data_len());
            flow->initPacketOut(&po);

            uint8_t* buffer = po.pack();
            ofconn->send(buffer, po.length());
            OFMsg::free_buffer(buffer);

            flow->deleteLater();
        } else {
            // In other cases we need to add newly created Flow into the
            // trace tree and rebuild it [TODO: incrementaly].
            DVLOG(5) << "Rebuilding flow table on conn = " << conn_id;

            static const struct Barrier {
                uint8_t* data;
                size_t len;
                Barrier() {
                    of13::BarrierRequest br;
                    data = br.pack();
                    len = br.length();
                }
                ~Barrier() { OFMsg::free_buffer(data); }
            } barrier;

            // Initialize flow mod
            of13::FlowMod* fm = new of13::FlowMod();
            fm->xid(pi.xid());
            fm->buffer_id(pi.buffer_id());
            if (pi.buffer_id() == OFP_NO_BUFFER) {
                of13::PacketOut po;
                po.xid(pi.xid());
                po.buffer_id(OFP_NO_BUFFER);
                po.data(pi.data(), pi.data_len());
                flow->initPacketOut(&po);

                uint8_t* buffer = po.pack();
                ofconn->send(buffer, po.length());
                OFMsg::free_buffer(buffer);
            }
            fm->command(of13::OFPFC_ADD);
            flow->setFlags(Flow::TrackFlowRemoval);
            flow->initFlowMod(fm);

            // Add new leaf to the trace tree
            trace_tree.augment(flow, fm);
            if (VLOG_IS_ON(10)) {
                std::stringstream ss;
                trace_tree.dump(ss);
                VLOG(10) << "Current trace tree on connection " << ofconn->get_id() << ": "
                    << std::endl << ss.str();
            }

            // Rebuild flow table from scratch.
            // We will do incremental update in the future.
            trace_tree.cleanFlowTable(ofconn);
            ofconn->send(barrier.data, barrier.len);
            unsigned rules = trace_tree.buildFlowTable(ofconn);

            DVLOG(5) << rules << " rules generated for switch on conn = " << conn_id;

            // FIXME: Can flow be expired and free'd at this point?
            flow->setLive();
        }

    } else {
        // Flow removed from the switch by idle timeout, but
        // still valid (by hard timeout). Reinstall it without
        // touching handlers.

        DVLOG(5) << "Reinstalling rule from the trace tree";
        // Flow removed by idleTimeout but still actual
        of13::FlowMod* fm = leaf->fm;
        fm->xid(pi.xid());
        fm->buffer_id(pi.buffer_id());
        leaf->flow->initFlowMod(fm);

        uint8_t* buffer = fm->pack();
        ofconn->send(buffer, fm->length());
        OFMsg::free_buffer(buffer);

        leaf->flow->setLive();
    }
}

void SwitchScope::processFlowRemoved(Flow *flow, uint8_t reason)
{
    if (flow) {
        if (reason == of13::OFPRR_HARD_TIMEOUT)
            flow->setDestroy();
        if (reason == of13::OFPRR_IDLE_TIMEOUT)
            flow->setShadow();
    }
}

/* Application interface */
Controller::Controller()
    : impl(nullptr)
{ }

void Controller::init(Loader*, const Config& rootConfig)
{

    auto config = config_cd(rootConfig, "controller");

    impl = new ControllerImpl(
            this,
            config_get(config, "address", "0.0.0.0").c_str(),
            config_get(config, "port", 6653),
            config_get(config, "nthreads", 4),
            config_get(config, "secure", false),
            OFServerSettings()
                    .supported_version(of13::OFP_VERSION)
                    .keep_data_ownership(false)
                    .echo_interval(config_get(config, "echo_interval", 15))
                    .liveness_check(config_get(config, "liveness_check", true))
    );
    impl->config = config;
}

Controller::~Controller() {
    delete impl;
}

void Controller::startUp(Loader*)
{
    impl->sortPipeline();
    impl->start(/* block: */ false);
    impl->started = true;
    impl->cbench = config_get(impl->config, "cbench", false);
}

void Controller::registerHandler(OFMessageHandlerFactory *factory)
{
    if (impl->started) {
        LOG(ERROR) << "Registering handler after startup";
        return;
    }
    VLOG(10) << "Registring flow processor " << factory->orderingName();
    impl->pipeline_factory.push_back(factory);
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
        mutex.lock();
        impl->static_ofresponse[xid] = 0;
        mutex.unlock();
    });

    return ret;
}

TraceTree* Controller::getTraceTree(uint64_t dpid)
{
    return &impl->switch_scope[dpid].trace_tree;
}
