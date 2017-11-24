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

#include "maple/Runtime.hh"
#include "oxm/field_set.hh"
#include "types/exception.hh"

#include "OFMsgUnion.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"
#include "PacketParser.hh"
#include "FluidOXMAdapter.hh"


using namespace std::placeholders;
using namespace runos;
using namespace fluid_base;
using maple::TraceTree;

REGISTER_APPLICATION(Controller, {""})

typedef std::vector< std::pair<std::string, PacketMissHandler> >
    PacketMissPipeline;
typedef std::vector< std::pair<std::string, PacketMissHandlerFactory> >
    PacketMissPipelineFactory;

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

struct ModTrackingPacket final : public PacketProxy {
    oxm::field_set m_mods;
public:
    explicit ModTrackingPacket(Packet& pkt)
        : PacketProxy(pkt)
    { }

    void modify(const oxm::field<> patch) override
    {
        pkt.modify(patch);
        m_mods.modify(patch);
    }

    const oxm::field_set& mods() const
    { return m_mods; }
};

// Enable protected members
struct DecisionImpl : Decision
{
    DecisionImpl() = default;
    const Base& base() const
    { return Decision::base(); }

    const DecisionData& data() const
    { return m_data; }
};

struct unhandled_packet: runtime_error
{ };

class FlowImpl final : public Flow
                     , public maple::Flow
{
    SwitchConnectionPtr conn;

    uint16_t m_priority;
    oxm::field_set m_match;
    uint8_t m_table{0};
    uint32_t m_in_port;
    Decision m_decision {DecisionImpl()};
    oxm::field_set m_mods;

    bool m_packet_in {false};
    int expect_remowed{0};
    uint32_t m_xid {0};
    uint32_t m_buffer_id {OFP_NO_BUFFER};

    // arrived packetIn take care of this data
    // FlowImpl shouldn't free it
    void* m_packet_data {nullptr};
    size_t m_data_len {0};

    std::function<Action *()>m_flood;

    void setState(State new_state)
    {
        m_state = new_state;
        emit stateChanged(new_state, cookie());
    }

   class DecisionCompiler : public boost::static_visitor<void> {
        ActionList& ret;
        std::function<Action *()> flood;

    public:
        explicit DecisionCompiler(ActionList& ret, std::function<Action*()> flood)
            : ret(ret), flood(flood)
        { }

        void operator()(const Decision::Undefined&) const
        {
            RUNOS_THROW(unhandled_packet());
        }

        void operator()(const Decision::Drop&) const
        { }

        void operator()(const Decision::Unicast& u) const
        {
            ret.add_action(new of13::OutputAction(u.port, 0));
        }

        void operator()(const Decision::Multicast& m) const
        {
            for (uint32_t port : m.ports) {
                ret.add_action(new of13::OutputAction(port, 0));
            }
        }

        void operator()(const Decision::Broadcast& b) const
        {
            ret.add_action(flood());
        }

        void operator()(const Decision::Inspect& i) const
        {
            ret.add_action(new of13::OutputAction(of13::OFPP_CONTROLLER,
                                                  i.send_bytes_len));
        }
    };

    ActionList actions() const
    {
        ActionList ret;

        for (const oxm::field<>& f : m_mods) {
            ret.add_action(new of13::SetFieldAction(new FluidOXMAdapter(f)));
        }

        boost::apply_visitor(DecisionCompiler(ret, m_flood), m_decision.data());
        return ret;
    }

    void install()
    {
        using std::chrono::duration_cast;
        using std::chrono::seconds;

        if (state() == State::Evicted && not m_packet_in)
            return;

        if (state() != State::Active && not m_packet_in)
            return;

        if (state() != State::Egg ){
            // becouse maple require deleting flow-tables before installing, we will
            // wait flow-remowed message from switch
            //TODO : handle this in MapleBackand
            expect_remowed++;
        }

        if (m_decision.idle_timeout() <= Decision::duration::zero()) {
            if (m_packet_in) {
                of13::PacketOut po;
                po.xid(m_xid);
                po.buffer_id(m_buffer_id);
                po.actions(actions());
                po.in_port(m_in_port);

                // if switch not buffering packets
                if (m_buffer_id == OFP_NO_BUFFER && m_packet_data != nullptr){
                    po.data(m_packet_data, m_data_len);
                }

                conn->send(po);
            }
        } else {

            // if we have data and switch not buffering it
            if (m_packet_in &&
                m_buffer_id == OFP_NO_BUFFER &&
                m_packet_data != nullptr) {
                of13::PacketOut po;
                po.xid(m_xid);
                po.buffer_id(OFP_NO_BUFFER);
                po.actions(actions());
                po.in_port(m_in_port);
                po.data(m_packet_data, m_data_len);
                conn->send(po);
            }
            of13::FlowMod fm;

            fm.command(of13::OFPFC_ADD);
            fm.xid(m_xid);
            fm.buffer_id(m_buffer_id);

            fm.table_id(m_table);
            fm.priority(m_priority);
            fm.cookie(cookie());
            fm.match(make_of_match(m_match));

            auto ito = m_decision.idle_timeout();
            auto hto = m_decision.hard_timeout();
            long long ito_seconds = duration_cast<seconds>(ito).count();
            long long hto_seconds = duration_cast<seconds>(hto).count();

            if (ito == Decision::duration::max())
                fm.idle_timeout(0);
            else
                fm.idle_timeout(std::min(ito_seconds, 65535LL));

            if (hto == Decision::duration::max())
                fm.hard_timeout(0);
            else
                fm.hard_timeout(std::min(hto_seconds, 65535LL));

            fm.flags( of13::OFPFF_CHECK_OVERLAP |
                      of13::OFPFF_SEND_FLOW_REM );

            of13::ApplyActions applyActions;
            applyActions.actions(actions());
            fm.add_instruction(applyActions);

            conn->send(fm);
        }

        m_packet_in = false;
        m_xid = 0;
        m_buffer_id = OFP_NO_BUFFER;
        m_packet_data = nullptr;
        m_data_len = 0;
        if (state() != State::Active)
            setState(State::Active);
        m_in_port = of13::OFPP_CONTROLLER;
    }

public:
    explicit FlowImpl(SwitchConnectionPtr conn, uint8_t table,
                       std::function<Action *()> flood)
        : conn(conn), m_table(table), m_flood(flood)
    { }

    void mods(oxm::field_set mod)
    {
        BOOST_ASSERT(state() != State::Active);
        m_mods = std::move(mod);
    }

    void decision(Decision d)
    {
        //BOOST_ASSERT(state() != State::Active);
        m_decision = std::move(d);
    }

    // Transitions
    void packet_in(of13::PacketIn& pi)
    {
        //BOOST_ASSERT(state() == State::Egg ||
        //             state() == State::Evicted ||
        //             state() == State::Idle ||
        //             (state() == State::Active &&
        //                boost::get<Decision::Inspect>(&m_decision.data()))
        //           );

        m_packet_in = true;
        m_xid = pi.xid();
        m_buffer_id = pi.buffer_id();
        m_in_port = pi.match().in_port()->value();
        m_packet_data = pi.data();
        m_data_len = pi.data_len();
    }

    void flow_removed(of13::FlowRemoved& fr)
    {
        switch (fr.reason()) {
        case of13::OFPRR_DELETE:
            if (expect_remowed == 1){
                expect_remowed--;
                return; // we waited this message becouse maple deleted flow-tables before
            }
            expect_remowed = -1;
        case of13::OFPRR_METER_DELETE:
            setState(State::Evicted);
            VLOG(30) << "Evicted Flow";
            break;
        case of13::OFPRR_IDLE_TIMEOUT:
            setState(State::Idle);
            VLOG(30) << "Deleted flow by idle timeout";
            break;
        case of13::OFPRR_HARD_TIMEOUT:
            setState(State::Expired);
            VLOG(30) << "Deted flow by hard timeout";
            break;
        }
    }

    void packet_out(uint16_t priority, oxm::field_set match)
    {
        m_priority = priority;
        m_match = std::move(match);
        install();
    }

    void wakeup()
    {
       // BOOST_ASSERT( state() == State::Idle
       //     || state() == State::Evicted
       //     || disposable() );
        install();
    }
    bool disposable(){
        return m_decision.idle_timeout() <= Decision::duration::zero();
    }
};

typedef std::shared_ptr<FlowImpl> FlowImplPtr;
typedef std::weak_ptr<FlowImpl> FlowImplWeakPtr;

class MapleBackend : public maple::Backend {
    SwitchConnectionPtr conn;
    uint8_t table{0};

    static FlowImplPtr flow_cast(maple::FlowPtr flow)
    {
        FlowImplPtr ret
            = std::dynamic_pointer_cast<FlowImpl>(flow);
        BOOST_ASSERT(ret);
        return ret;
    }

public:
    explicit MapleBackend(SwitchConnectionPtr conn, uint8_t table)
        : conn(conn), table(table)
    { }

    virtual void install(unsigned priority,
                         oxm::field_set const& match,
                         maple::FlowPtr flow_) override
    {
        auto flow = flow_cast(flow_);
        DVLOG(20) << "Installing prio=" << priority
                  << ", match={" << match << "}"
                  << " => cookie = " << std::setbase(16) << flow->cookie() << " on switch " << conn->dpid();
        flow->packet_out(priority, match);
    }

    void remove(oxm::field_set const& match) override
    {
        DVLOG(20) << "Removing flows matching {" << match << "}" << " on switch " << conn->dpid();

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);

        fm.table_id(table);
        fm.cookie(Flow::cookie_space().first);
        fm.cookie_mask(Flow::cookie_space().second);
        fm.match(make_of_match(match));

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void remove(unsigned priority,
                oxm::field_set const& match) override
    {
        DVLOG(20) << "Removing flows matching prio=" << priority
                  << " with " << match;

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE_STRICT);

        fm.table_id(table);
        fm.cookie(Flow::cookie_space().first);
        fm.cookie_mask(Flow::cookie_space().second);
        fm.match(make_of_match(match));
        fm.priority(priority);

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void remove(maple::FlowPtr flow_) override
    {
        auto flow = flow_cast(flow_);
        DVLOG(20) << "Removing flow with cookie=" << flow->cookie();

        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);

        fm.table_id(table);
        fm.cookie(flow->cookie());
        fm.cookie_mask(uint64_t(-1));

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        conn->send(fm);
    }

    void barrier() override
    {
        conn->send(of13::BarrierRequest());
    }
};

typedef boost::error_info< struct tag_pi_handler, std::string >
    errinfo_packetin_handler;

struct SwitchScope {
    SwitchConnectionImplPtr connection;
    FlowImplPtr miss;
    MapleBackend backend;
    maple::Runtime<DecisionImpl, FlowImpl> runtime;
    PacketMissPipeline pipeline;
    std::unordered_map<uint64_t, FlowImplPtr> flows;
    uint8_t handler_table;
    std::function <Action*()> flood;

    SwitchScope(PacketMissPipelineFactory const& pipeline_factory,
                OFConnection* ofconn,
                uint64_t dpid,
                uint8_t handler_table,
                FloodImplementation _flood)
        : connection{new SwitchConnectionImpl{ofconn, dpid}}
        , miss{new FlowImpl(connection, handler_table, std::bind(_flood, 0))}
        , backend{connection, handler_table}
        , runtime{std::bind(&SwitchScope::process, this, _1, _2), backend, miss}
        , handler_table{handler_table}
        , flood(std::bind(_flood, dpid))
    {
        clearTables();
        miss->decision( DecisionImpl{}.inspect(128) ); // TODO: unhardcode

        for (auto &factory : pipeline_factory) {
            pipeline.emplace_back(factory.first,
                                  std::move(factory.second(connection)));
        }
        for (uint8_t i = 0; i < handler_table; i++){
            installGoto(i);
        }
        installTableMissRule();
    }

    void reinit(){
        clearTables();
        for (uint8_t i = 0; i < handler_table; i++){
            installGoto(i);
        }
        installTableMissRule();
    }

    DecisionImpl process(Packet& pkt, FlowImplPtr flow) const
    {
        DecisionImpl ret = DecisionImpl{};
        for (auto& handler: pipeline) {
            try {
                ret = (DecisionImpl&&)(handler.second(pkt, flow, ret));
                if (ret.base().return_)
                    return ret;
            } catch (boost::exception & e) {
                e << errinfo_packetin_handler(handler.first);
                throw;
            }
        }
        return ret;
    }

    bool isTableMiss(of13::PacketIn& pi) const
    {
        if (pi.reason() == of13::OFPR_NO_MATCH)
            return true;
        if (pi.reason() == of13::OFPR_ACTION && pi.cookie() == miss->cookie())
            return true;
        return false;
    }

    void clearTables()
    {
        backend.barrier();
        of13::FlowMod fm;
        fm.command(of13::OFPFC_DELETE);

        fm.table_id(of13::OFPTT_ALL);
        fm.cookie(0x0);
        fm.cookie_mask(0x0);

        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        connection->send(fm);
    }

    /* install in table `table` goto on `table + 1` */
    void installGoto(uint8_t table)
    {
        backend.barrier();
        of13::FlowMod fm;
        fm.command(of13::OFPFC_ADD);
        fm.priority(0);
        fm.cookie(0);
        fm.idle_timeout(0);
        fm.hard_timeout(0);
        fm.buffer_id(OFP_NO_BUFFER);
        fm.table_id(table);
        fm.flags( of13::OFPFF_CHECK_OVERLAP |
                  of13::OFPFF_SEND_FLOW_REM );
        of13::ApplyActions act;
        of13::GoToTable go_to_table(table + 1);
        fm.add_instruction(go_to_table);
        connection->send(fm);
    }

    void installTableMissRule()
    {
        backend.remove(oxm::field_set{});
        backend.barrier();
        of13::FlowMod tableMiss;
        tableMiss.command(of13::OFPFC_ADD);
        tableMiss.priority(0);
        tableMiss.cookie(0);
        tableMiss.buffer_id(OFP_NO_BUFFER);
        tableMiss.idle_timeout(0);
        tableMiss.hard_timeout(0);
        tableMiss.table_id(handler_table);
        tableMiss.flags( of13::OFPFF_CHECK_OVERLAP |
                  of13::OFPFF_SEND_FLOW_REM );
        of13::ApplyActions act;
        of13::OutputAction out(of13::OFPP_CONTROLLER, 128); //TODO : unhardcore
        act.add_action(out);
        tableMiss.add_instruction(act);
        connection->send(tableMiss);
    }
    void processPacketIn(of13::PacketIn& pi);
    void processFlowRemoved(of13::FlowRemoved& fr);
};

class ControllerImpl : public OFServer {
    const uint32_t min_xid = 0xfff;
    Controller &app;

public:
    bool started{false};
    bool cbench;
    uint8_t handler_table{0};
    Config config;

    std::unordered_map<std::string, PacketMissHandlerFactory>
        handlers;
    PacketMissPipelineFactory pipeline_factory;
    bool isFloodImplementation{false};
    FloodImplementation flood;
    std::unordered_map<uint64_t, SwitchScope> switches;

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

        SwitchScope *ctx = reinterpret_cast<SwitchScope *>(ofconn->get_application_data());

        if (ctx == nullptr && type != of13::OFPT_FEATURES_REPLY) {
            LOG(WARNING) << "Switch send message before feature reply";
            OFMsg::free_buffer(static_cast<uint8_t *>(data));
            return;
        }

        try {
            OFMsgUnion msg(type, data, len);

            switch (type) {
            case of13::OFPT_PACKET_IN:
                ctx->processPacketIn(msg.packetIn);
                break;
            case of13::OFPT_FEATURES_REPLY:
                ctx = createSwitchScope(ofconn, msg.featuresReply.datapath_id());
                ofconn->set_application_data(ctx);
                emit app.switchUp(ctx->connection, msg.featuresReply);
                break;
            case of13::OFPT_PORT_STATUS:
                if (ctx == nullptr) break;
                emit app.portStatus(ctx->connection, msg.portStatus);
                break;
            case of13::OFPT_FLOW_REMOVED:
                ctx->processFlowRemoved(msg.flowRemoved);
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
        auto ctx = reinterpret_cast<SwitchScope*>(ofconn->get_application_data());

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
                ctx->runtime.invalidate();
                ctx->flows.clear();
            }
        }

        else if (type == OFConnection::EVENT_DEAD) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed due to inactivity";
            if (ctx) {
                ofconn->set_application_data(nullptr);
                emit app.switchDown(ctx->connection);
                ctx->connection->replace(nullptr);
                ctx->runtime.invalidate();
                ctx->flows.clear();
            }
        }
    }

    SwitchScope *createSwitchScope(OFConnection *ofconn, uint64_t dpid)
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
                                  std::forward_as_tuple(pipeline_factory,
                                                        ofconn,
                                                        dpid,
                                                        handler_table,
                                                        flood))
                         .first;
            return &it->second;
        }

        ret:
        SwitchScope *ctx = &it->second;
        if (ctx->connection->alive()) {
            LOG(ERROR) << "Overwriting switch scope on active connection";
        }
        ctx->connection->replace(ofconn);
        ctx->reinit();

        return ctx;
    }

    void invalidateTraceTree()
    {
        LOG(INFO) << "Invalidation all Trace Trees";
        for (auto i =  switches.begin(); i != switches.end(); i++){
            i->second.runtime.invalidate();
            i->second.backend.remove(oxm::field_set{});
        }
    }
};

void SwitchScope::processPacketIn(of13::PacketIn& pi)
{
    DVLOG(10) << "Packet-in on switch " << connection->dpid()
              << (isTableMiss(pi) ? " (miss)" : " (inspect)");

    // Serializes to/from raw buffer
    PacketParser pkt { pi };
    // Find flow in the trace tree
    std::shared_ptr<FlowImpl> flow = runtime(pkt);

    DVLOG(30) << "flow cookie is : " << std::setbase(16) << flow->cookie() << " packet cookie : " << pi.cookie();
    // Delete flow if it doesn't found or expired
    if (flow == miss || flow->state() == Flow::State::Expired) {
        DVLOG(30) << "hit on  mapple-barrier rule. Cookie : " << std::setbase(16) << miss->cookie();
        flow = std::make_shared<FlowImpl>(connection, handler_table, flood);
        flows[flow->cookie()] = flow;
    }
    flow->packet_in(pi);

    switch (flow->state()) {
        case Flow::State::Egg:
        case Flow::State::Idle:
        {
            ModTrackingPacket mpkt {pkt};
            runtime.augment(mpkt, flow);
            flow->mods( std::move(mpkt.mods()) );
            if (flow->disposable()){
            // Sometimes we don't need to create a new flow on the switch.
            // So, reply to the packet-in using packet-out message.
                flow->packet_out(1, mpkt.mods() );
                flows.erase(flow->cookie());
            } else {
                runtime.commit();
            }
        }
        break;

        case Flow::State::Evicted:
            flow->wakeup();
        break;

        case Flow::State::Expired:
            BOOST_ASSERT(false);
        break;

        case Flow::State::Active:
        {
            DLOG_IF(ERROR, isTableMiss(pi))
                << "Table-miss on active non-inspect flow, "
                << "cookie = " << std::setbase(16) << flow->cookie()
                << ", reason = " << unsigned(pi.reason()) << " disposable : " << flow->disposable();
            // BOOST_ASSERT(not isTableMiss(pi));
            if (not isTableMiss(pi)){
                flow->decision(process(pkt, flow));
            } else {
                flow->wakeup();
            }
            // Maybe this packet arrived on switch when maple reload table, but may be from remowed flows
            // TODO: implement FSM of flow
            // TODO: make that packets arrived Only when reloading table */
        }
        break;
    }
}

void SwitchScope::processFlowRemoved(of13::FlowRemoved& fr)
{
    DVLOG(30) << "Flow-remowed message handled on  switch : " << connection->dpid();
    auto it = flows.find( fr.cookie() );
    if (it == flows.end())
        return;
    auto flow = it->second;

    flow->flow_removed(fr);
    if (flow->state() == Flow::State::Expired)
        flows.erase(it);
}

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
}

void Controller::startUp(Loader*)
{
    auto config = impl->config;

    LOG(INFO) << "Registered packet-in handlers: ";
    for (const auto& handler : impl->handlers) {
        LOG(INFO) << "\t" << handler.first;
    }
    LOG(INFO) << ".";

    const auto& names = config.at("pipeline").array_items();
    // TODO: check dups
    for (const auto& name_token : names) {
        // TODO: warn if doesn't exists
        auto name = name_token.string_value();
        impl->pipeline_factory.emplace_back(name, impl->handlers.at(name));
    }
    // TODO: print unused handlers
    if (not impl->isFloodImplementation){
        LOG(INFO) << "using default flood implemetation";
        impl->flood = [](uint32_t){
            return new of13::OutputAction(of13::OFPP_FLOOD, 0);
        };
    }
    impl->start(/* block: */ false);
    impl->started = true;
    impl->cbench = config_get(impl->config, "cbench", false);
}

void Controller::registerHandler(const char* name,
                                 PacketMissHandlerFactory factory)
{
    if (impl->started) {
        LOG(ERROR) << "Registering handler after startup";
        return;
    }

    VLOG(10) << "Registring flow processor " << name;
    impl->handlers[std::string(name)] = factory;
}

void Controller::registerFlood(FloodImplementation _flood)
{
    if (impl->started) {
        LOG(ERROR) << "Register flood implemetation after startup";
        return;
    }
    VLOG(10) << "Register flood implemetation";
    //TODO : mutax setup
    if (impl->isFloodImplementation){
        LOG(ERROR) << "trying register more than one flood implemetation";
        return;
    }
    impl->isFloodImplementation = true;
    impl->flood = _flood;
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

uint8_t Controller::handler_table() const
{ return impl->handler_table; }

uint8_t Controller::reserveTable()
{ return impl->handler_table++; }

void Controller::invalidateTraceTree()
{ impl->invalidateTraceTree(); }

Controller::~Controller() = default;
