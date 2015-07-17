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

#include "Switch.hh"

#include <unordered_map>

REGISTER_APPLICATION(SwitchManager, {"controller", "rest-listener", ""})

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
    r = new SwitchManagerRest("Switch Manager", "switch.html");
    r->m = this;
    r->makeEventApp();
}

SwitchManager::~SwitchManager()
{
    delete m;
    delete r;
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

    QObject::connect(this, &SwitchManager::switchDiscovered,
                     r, &SwitchManagerRest::onSwitchDiscovered);
    QObject::connect(this, &SwitchManager::switchDown,
                     r, &SwitchManagerRest::onSwitchDown);

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

    RestListener::get(loader)->newListener("switch-manager", r);
}

void SwitchManager::onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr)
{
    int conn_id = ofconn->get_id();
    QWriteLocker locker(&m->switch_lock);

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
        locker.unlock();

        emit switchDiscovered(m->switch_by_conn[conn_id] = &it->second);
    } else {
        m->switch_by_conn[conn_id] = &it->second;
        locker.unlock();

        it->second.setUp(ofconn, fr);
    }

    it->second.requestPortDescriptions();
}

void SwitchManager::onSwitchDown(OFConnection* ofconn)
{
    if (!ofconn)
        return;
    auto it = m->switch_by_conn.find(ofconn->get_id());
    if (it == m->switch_by_conn.end())
        return;

    Switch* dp = it->second;
    dp->setDown();

    emit switchDown(dp);
}

void SwitchManager::onPortStatus(OFConnection* ofconn, of13::PortStatus ps)
{
    // don't acquire lock because operation lives in qt loop
    m->switch_by_conn.at(ofconn->get_id())->portStatus(ps);
}

void SwitchManagerRest::onSwitchDiscovered(Switch *dp) {
    addEvent(Event::Add, dp);
    dp->connectedSince(time(NULL));
}

void SwitchManagerRest::onSwitchDown(Switch *dp) {
    addEvent(Event::Delete, dp);
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
        locker.unlock();

        emit portUp(this, port);
        break;
    }
    case of13::OFPPR_DELETE: {
        if (m->port.erase(port_no) != 1) {
            LOG(WARNING) << "Datapath " << idstr() << " signals about removed port "
                    << port_no << ", but it is not exists before";
        }

        DVLOG(2) << "Deleted port " << idstr() << ':' << port_no;
        locker.unlock();

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
        locker.unlock();

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

json11::Json Switch::to_json() const {
    return json11::Json::object {
        {"ID", id_str()},
        {"DPID", AppObject::uint64_to_string(id())}
    };
}

json11::Json Switch::to_floodlight_json() const {
    std::vector<json11::Json> ports_vec;
    for (of13::Port port : ports()) {
        json11::Json json_port = json11::Json::object {
            {"portNumber", (int)port.port_no()},
            {"hardwareAddress", port.hw_addr().to_string()},
            {"name", port.name()},
            {"config", (int)port.config()},
            {"state", (int)port.state()},
            {"currentFeatures", (int)port.curr()},
            {"advertisedFeatures", (int)port.advertised()},
            {"supportedFeatures", (int)port.supported()},
            {"peerFeatures", (int)port.peer()}
        };
        ports_vec.push_back(json_port);
    }

    return json11::Json::object {
        {"harole", "MASTER"},
        {"ports", json11::Json(ports_vec)},
        {"buffers", (int)nbuffers()},
        {"description", "NONE"},
        {"capabilities", (int)capabilites()},
        {"inetAddress", "NONE"},
        {"connectedSince", uint64_to_string(static_cast<uint64_t>(connectedSince()))},
        {"dpid", uint64_to_string(id())},
        {"actions", "NONE"},
        {"attributes", "NONE"}
    };
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

OFConnection* Switch::ofconn() const
{
    return m->conn;
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

std::string SwitchManagerRest::handle(std::vector<std::string> params)
{
    if (params[0] != "GET")
        return "{ \"error\": \"error\" }";
    if (params[2] == "switches") {
        if (params[3] == "all") {
            return json11::Json(m->switches()).dump();
        }
    }

    //Floodlight
    if (params[2] == "core" &&
            params[3] == "controller" &&
            params[4] == "switches" &&
            params[5] == "json") {

        std::vector<json11::Json> res;
        auto sw = m->switches();
        for (auto it = sw.begin(); it != sw.end(); it++) {
            res.push_back((*it)->to_floodlight_json());
        }
        return json11::Json(res).dump();

    }

    return "{}";
}

int SwitchManagerRest::count_objects()
{
    return m->switches().size();
}
