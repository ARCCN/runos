#include <algorithm>
#include <unordered_map>
#include <vector>
#include <list>
#include <mutex>
#include <thread>

#include <fluid/OFServer.hh>
#include "AppProvider.hh"
#include "Controller.hh"
#include "Flow.hh"

REGISTER_APPLICATION(Controller, "controller", {""})

struct Pipeline {
    Pipeline(size_t n = 1) : last_cookie(1) { }
    std::list<OFMessageHandler*> processors;
    int last_cookie;
};

class ControllerImpl : public OFServer
{
    Controller* app;

    const uint32_t min_xid = 0xff;

public:
    bool started;
    // Pipeline
    std::unordered_map< std::thread::id, Pipeline > pipeline;
    std::vector< OFMessageHandlerFactory* > pipeline_factory;
    // OFResponse
    std::vector<OFTransaction*> static_ofresponse;
    uint32_t min_session_xid; // Make sure that we don't intersect with libfluid_base
    //uint32_t last_xid;

    ControllerImpl(Controller* _app,
               const char* address,
               const int port,
               const int nthreads = 4,
               const bool secure = false,
               const struct OFServerSettings ofsc = OFServerSettings())
            : OFServer(address, port, nthreads, secure, ofsc),
              app(_app), started(false),
              min_session_xid(min_xid) //, last_xid(min_xid)
    { }

    void message_callback(OFConnection* ofconn, uint8_t type, void* data, size_t len) override
    {
        auto conn = *reinterpret_cast< shared_ptr<OFConnection>* >(ofconn->get_application_data());
        OFMsg* msg;

        shared_ptr<of13::PacketIn> pi;
        shared_ptr<of13::Error>    error;
        of13::FeaturesReply  fr;
        of13::PortStatus     ps;

        // Construct message object
        switch (type) {
        case of13::OFPT_PACKET_IN:
            pi = std::make_shared<of13::PacketIn>();
            msg = pi.get();
            break;
        case of13::OFPT_FEATURES_REPLY:
            msg = &fr;
            break;
        case of13::OFPT_PORT_STATUS:
            msg = &ps;
            break;
        case of13::OFPT_MULTIPART_REPLY:
            msg = new of13::MultipartReply();
            break;
        case of13::OFPT_BARRIER_REPLY:
            msg = new of13::BarrierReply();
            break;
        case of13::OFPT_ERROR:
            error = std::make_shared<of13::Error>();
            msg = error.get();
            break;
        default:
            LOG(WARNING) << "Unhandled message type " << type
                    <<" received from connection " << ofconn->get_id();
            return;
        }

        // Parse data
        if (msg->unpack(static_cast<uint8_t*>(data)) != 0) {
            LOG(WARNING) << "Malformed message received from connection " << ofconn->get_id();
        }
        if (type != of13::OFPT_MULTIPART_REPLY) {
            free_data(data);
            data = nullptr;
        }

        // Send it to handlers
        switch (type) {
        case of13::OFPT_PACKET_IN:
            if (pi->reason() == of13::OFPR_NO_MATCH) {
                processTableMiss(conn, pi);
            } else {
                LOG(ERROR) << "TODO: to-controller packet-ins"; // TODO
            }
            break;
        case of13::OFPT_FEATURES_REPLY:
            emit app->switchUp(conn, fr);
            break;
        case of13::OFPT_PORT_STATUS:
            emit app->portStatus(conn, ps);
            break;
        default: {
            uint32_t xid = msg->xid();
            OFTransaction *transaction;

            if (xid < min_xid)
                break;

            if (xid < min_session_xid) {
                transaction = static_ofresponse[xid - min_xid];
            } else {
                // TODO: session ofresponse's
            }

            if (transaction) {
                if (type == of13::OFPT_ERROR) {
                    emit transaction->error(conn, error);
                } else {
                    emit transaction->response(conn, msg, static_cast<uint8_t*>(data));
                }
            }
        }
        }
    }

    void connection_callback(OFConnection* ofconn, OFConnection::Event type) override
    {
        auto conn = reinterpret_cast< shared_ptr<OFConnection>* >(ofconn->get_application_data());

        if (type == OFConnection::EVENT_STARTED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " started";
            ofconn->set_application_data(new shared_ptr<OFConnection>(ofconn));
        }

        else if (type == OFConnection::EVENT_ESTABLISHED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " established";
        }

        else if (type == OFConnection::EVENT_FAILED_NEGOTIATION) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << ": failed version negotiation";
        }

        else if (type == OFConnection::EVENT_CLOSED) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed by the user";
            emit app->switchDown(*conn);
            /*delete conn;*/ ofconn->set_application_data(nullptr);
        }

        else if (type == OFConnection::EVENT_DEAD) {
            LOG(INFO) << "Connection id=" << ofconn->get_id() << " closed due to inactivity";
            emit app->switchDown(*conn);
            /*delete conn;*/ ofconn->set_application_data(nullptr);
        }
    }

    void sortPipeline()
    {
        std::sort(pipeline_factory.begin(), pipeline_factory.end(),
                [] (const OFMessageHandlerFactory* a, const OFMessageHandlerFactory* b) -> bool {
                    bool a_before_b = b->isPrereq(a->orderingName());
                    bool a_after_b  = b->isPostreq(a->orderingName());
                    bool b_before_a = a->isPrereq(b->orderingName());
                    bool b_after_a  = a->isPostreq(b->orderingName());

                    if ( ( a_before_b && a_after_b  ) ||
                            ( b_before_a && b_after_a  ) ||
                            ( a_before_b && b_before_a ) ||
                            ( a_after_b  && b_after_a  ))
                    {
                        LOG(FATAL) << "Wrong pipeline order constraints on "
                                << a->orderingName() << " and " << b->orderingName();
                    }

                    return a_before_b || b_after_a;
                }
        );

        DLOG(INFO) << "Flow processors registered: ";
        for (auto& factory : pipeline_factory)
            DLOG(INFO) << "   " << factory->orderingName();
    }

    Pipeline& getPipeline(std::thread::id tid)
    {
        auto it = pipeline.find(tid);
        if (it != pipeline.end())
            return it->second;

        DLOG(INFO) << "Creating new pipeline with " << pipeline_factory.size()
                << " flow processor(s)";
        Pipeline new_pipeline(pipeline_factory.size());
        for (auto& factory : pipeline_factory) {
            new_pipeline.processors.push_back(factory->makeOFMessageHandler());
        }

        return pipeline[tid] = new_pipeline;
    }

    void processTableMiss(shared_ptr<OFConnection> ofconn, shared_ptr<of13::PacketIn> pi)
    {
        DLOG(INFO) << "Table miss on connection id=" << ofconn->get_id();

        Pipeline& pipeline = getPipeline(std::this_thread::get_id());
        Flow* flow = new Flow(pi);

        for (const auto& processor : pipeline.processors) {
            DLOG(INFO) << " processing step...";
            if (processor->processMiss(ofconn,flow) == OFMessageHandler::Stop)
                break;
        }

        DLOG(INFO) << "Installing rule to the switch";
        of13::FlowMod fm = flow->compile();

        fm.cookie(((uint64_t)ofconn->get_id() << 32ULL) + pipeline.last_cookie++);
        fm.cookie_mask(0);
        fm.command(of13::OFPFC_ADD);

        fm.idle_timeout(20); // FIXME: move to config
        fm.hard_timeout(0); // FIXME move to config

        fm.buffer_id(pi->buffer_id());
        fm.xid(pi->xid());

        // FIXME: use config
        fm.flags(fm.flags() | of13::OFPFF_CHECK_OVERLAP);

        uint8_t* buffer = fm.pack();
        ofconn->send(buffer, fm.length());
        flow->setInstalled(fm.cookie());

        OFMsg::free_buffer(buffer);
    }

};

/* Application interface */
Controller::Controller()
    : impl(nullptr)
{ }

void Controller::init(AppProvider*, const Config& rootConfig)
{
    qRegisterMetaType<of13::PortStatus>();
    qRegisterMetaType<of13::FeaturesReply>();
    qRegisterMetaType< shared_ptr<of13::Error> >();
    qRegisterMetaType< shared_ptr<OFConnection> >();

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
}

Controller::~Controller() {
    delete impl;
}

void Controller::startUp(AppProvider*)
{
    impl->sortPipeline();
    impl->start(/* block: */ false);
    impl->started = true;
}

void Controller::registerHandler(OFMessageHandlerFactory *factory)
{
    if (impl->started) {
        LOG(ERROR) << "Registering handler after startup";
        return;
    }
    DLOG(INFO) << "Registring flow processor " << factory->orderingName();
    impl->pipeline_factory.push_back(factory);
}

OFTransaction* Controller::registerStaticTransaction()
{
    if (impl->started) {
        LOG(ERROR) << "Registering static xid after startup";
        return 0;
    }

    uint32_t xid = impl->min_session_xid++;
    OFTransaction* ofr = new OFTransaction(xid, this);
    impl->static_ofresponse.push_back(ofr);

    QObject::connect(ofr, &QObject::destroyed, [xid, this]() {
        std::mutex mutex;
        mutex.lock();
        impl->static_ofresponse[xid] = 0;
        mutex.unlock();
    });

    return ofr;
}
