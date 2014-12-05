#include <unordered_map>

#include "Switch.hh"

REGISTER_APPLICATION(SwitchManager, {"controller", ""})

struct SwitchImpl {
    OFConnection* conn;
    SwitchManager* mgr;

    uint64_t         id;
    uint32_t         nbuffers;
    uint8_t          ntables;
    uint32_t         capabilities;

    QReadWriteLock port_lock;
    std::unordered_map<uint32_t, of13::Port>
            port;
};

struct SwitchManagerImpl {
    Controller* controller;
    OFTransaction* pdescr;

    QReadWriteLock switch_lock;
    std::unordered_map<int, Switch*> switch_by_conn;
    std::unordered_map<uint64_t, Switch> switches;
};

SwitchManager::SwitchManager()
{
    m = new SwitchManagerImpl;
}

SwitchManager::~SwitchManager()
{
    delete m;
}

Switch* SwitchManager::getSwitch(OFConnection* ofconn) const
{
    try {
        QReadLocker locker(&m->switch_lock);
        return m->switch_by_conn.at(ofconn->get_id());
    } catch (const std::out_of_range& e) {
        return nullptr;
    }
}

Switch* SwitchManager::getSwitch(uint64_t dpid)
{
    try {
        return &m->switches.at(dpid);
    } catch (const std::out_of_range& e) {
        return nullptr;
    }
}

void SwitchManager::init(Loader* loader, const Config& config)
{
    auto controller = m->controller = Controller::get(loader);

    QObject::connect(controller, &Controller::switchUp,
                     this, &SwitchManager::onSwitchUp);
    QObject::connect(controller, &Controller::switchDown,
                     this, &SwitchManager::onSwitchDown);
    QObject::connect(controller, &Controller::portStatus,
                     this, &SwitchManager::onPortStatus);

    m->pdescr = controller->registerStaticTransaction(this);
    QObject::connect(m->pdescr, &OFTransaction::response,
                     this, &SwitchManager::onPortDescriptions);
    QObject::connect(m->pdescr, &OFTransaction::error,
    [](OFConnection* conn, std::shared_ptr<OFMsgUnion> msg) {
        of13::Error& error = msg->error;
        LOG(ERROR) << "Switch reports error for OFPT_MULTIPART_REQUEST: "
            << "type " << (int) error.type() << " code " << error.code();
        // Send request again
        conn->send(error.data(), error.data_len());
    });
}

void SwitchManager::onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr)
{
    QWriteLocker locker(&m->switch_lock);

    int conn_id = ofconn->get_id();

    if (m->switch_by_conn.find(conn_id) != m->switch_by_conn.end()) {
        LOG(WARNING) << "Unexpected FeaturesReply received";
        return;
    }

    auto it = m->switches.find(fr.datapath_id());

    if (it == m->switches.end()) {
        it = m->switches.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(fr.datapath_id()),
                std::forward_as_tuple(this, ofconn, fr)
        ).first;
        emit switchDiscovered(m->switch_by_conn[conn_id] = &it->second);
    } else {
        m->switch_by_conn[conn_id] = &it->second;
        it->second.setUp(ofconn, fr);
    }

}

void SwitchManager::onSwitchDown(OFConnection* ofconn)
{
    auto it = m->switch_by_conn.find(ofconn->get_id());
    if (it == m->switch_by_conn.end())
        return;

    Switch* dp = it->second;
    dp->setDown();
}

void SwitchManager::onPortStatus(OFConnection* ofconn, of13::PortStatus ps)
{
    // don't acquire lock because operation lives in qt loop
    m->switch_by_conn.at(ofconn->get_id())->portStatus(ps);
}

void SwitchManager::onPortDescriptions(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> reply)
{
    // don't acquire lock because operation lives in qt loop
    auto type = reply->base()->type();
    if (type != of13::OFPT_MULTIPART_REPLY) {
        LOG(ERROR) << "Unexpected response of type " << type
                << " received, expected OFPT_MULTIPART_REPLY";
        return;
    }

    m->switch_by_conn.at(ofconn->get_id())->portDescArrived(reply->multipartReplyPortDescription);
}

std::vector<Switch*> SwitchManager::switches()
{
    QReadLocker locker(&m->switch_lock);

    std::vector<Switch*> ret;
    ret.reserve(m->switches.size());

    for (auto& pair : m->switches) {
        OFConnection* conn = pair.second.m->conn;

        if (conn->get_state() == OFConnection::STATE_RUNNING)
            ret.push_back(&pair.second);
    }
    return ret;
}

/* ==== Switch ===== */

Switch::Switch(SwitchManager* mgr,
               OFConnection* conn,
               of13::FeaturesReply fr)
 : m(new SwitchImpl)
{
    m->mgr = mgr;
    m->conn = conn;
    m->id = fr.datapath_id();
    m->nbuffers = fr.n_buffers();
    m->ntables = fr.n_tables();
    m->capabilities = fr.capabilities();
    requestPortDescriptions();
}

Switch::~Switch()
{
    delete m;
}

std::string Switch::idstr() const
{
    std::ostringstream ss;
    ss << std::setw(16) << std::hex << std::setfill('0') << id();
    return ss.str();
}

void Switch::setUp(OFConnection* conn, of13::FeaturesReply fr)
{
    auto conn_state = m->conn->get_state();
    if (conn_state != OFConnection::STATE_DOWN &&
        conn_state != OFConnection::STATE_FAILED)
    {
        LOG(WARNING) << "Setting switch up, but it isn't down";
    }

    m->conn = conn;
    m->id   = fr.datapath_id();
    m->nbuffers = fr.n_buffers();
    m->ntables  = fr.n_tables();
    m->capabilities = fr.capabilities();

    emit up(this);
    requestPortDescriptions();
}

void Switch::portStatus(of13::PortStatus ps)
{
    QWriteLocker locker(&m->port_lock);

    of13::Port port = ps.desc();
    uint32_t port_no = port.port_no();

    switch (ps.reason()) {
    case of13::OFPPR_ADD: {
        bool newPort = m->port.insert(std::make_pair(port_no, port)).second;
        if (!newPort) {
            LOG(WARNING) << "Datapath " << idstr() << " signals about new port "
                    << port_no << ", but it is already exists before";
        }

        DVLOG(2) << "Created port " << idstr() << ':' << port_no;
        emit portUp(this, port);
        break;
    }
    case of13::OFPPR_DELETE: {
        if (m->port.erase(port_no) != 1) {
            LOG(WARNING) << "Datapath " << idstr() << " signals about removed port "
                    << port_no << ", but it is not exists before";
        }

        DVLOG(2) << "Deleted port " << idstr() << ':' << port_no;
        emit portDown(this, port_no);
        break;
    }
    case of13::OFPPR_MODIFY: {
        auto it = m->port.find(port_no);
        if (it == m->port.end()) {
            LOG(WARNING) << "Datapath " << idstr() << " signals that port " << port_no <<
                    "changed, but it doesn't exists before";
        }
        auto old_port = it->second;
        it->second = port;

        DVLOG(2) << "Modified port " << idstr() << ':' << port_no;
        emit portModified(this, port, old_port);
        break;
    }
    }
}

void Switch::setDown()
{
    QWriteLocker locker(&m->port_lock);

    for (auto& portPair : m->port) {
        emit portDown(this, portPair.first);
    }
    m->port.clear();
    emit down(this);
}

void Switch::requestPortDescriptions()
{
    of13::MultipartRequestPortDescription req;
    req.flags(0); // no more requests will follow
    m->mgr->m->pdescr->request(m->conn, &req);
}

void Switch::portDescArrived(of13::MultipartReplyPortDescription &desc)
{
    for (auto& port : desc.ports()) {
        // FIXME: what if receive portStatus (delete) message before port descriptions?
        if (m->port.find(port.port_no()) == m->port.end()) {
            m->port[port.port_no()] = port;
            emit portUp(this, port);
        }

        DVLOG(1) << "PortDesc received (dpid=" << idstr() << ", port=" << port.port_no() << ") --> "
                << port.name() << "(" << port.hw_addr().to_string() << ") {"
                << "curr_speed=" << port.curr_speed() << ", max_speed=" << port.max_speed() << "}";
    }
}

void Switch::send(OFMsg *msg)
{
    uint8_t* data = msg->pack();
    m->conn->send(data, msg->length());
    OFMsg::free_buffer(data);
}

uint64_t Switch::id() const
{
    return m->id;
}

uint32_t Switch::nbuffers() const
{
    return m->nbuffers;
}

uint8_t Switch::ntables() const
{
    return m->ntables;
}

uint32_t Switch::capabilites() const
{
    return m->capabilities;
}

of13::Port Switch::port(uint32_t port_no) const
{
    QReadLocker locker(&m->port_lock);
    return m->port.at(port_no);
}

std::vector<of13::Port> Switch::ports() const
{
    QReadLocker locker(&m->port_lock);

    std::vector<of13::Port> ret(m->port.size());
    std::transform(m->port.begin(), m->port.end(), ret.begin(),
                   [](const std::pair<uint32_t, of13::Port>& p) {
                       return p.second;
                   });
    return ret;
}
