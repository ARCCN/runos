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

#include "HostManager.hh"

#include "Controller.hh"
#include "RestListener.hh"

REGISTER_APPLICATION(HostManager, {"switch-manager", "rest-listener", ""})

struct HostImpl {
    uint64_t id;
    std::string mac;
    IPAddress ip;
    uint64_t switchID;
    uint32_t switchPort;
};

struct HostManagerImpl {
    // mac address -> Host
    std::unordered_map<std::string, Host*> hosts;
};

Host::Host(std::string mac, IPAddress ip)
{
    m = new HostImpl;
    m->mac = mac;
    m->ip = ip;
    m->id = rand()%1000 + 1000;
}

Host::~Host()
{ delete m; }

uint64_t Host::id() const
{ return m->id; }

std::string Host::mac() const
{ return m->mac; }

std::string Host::ip() const
{ return uint32_t_ip_to_string(m->ip.getIPv4()); }

uint64_t Host::switchID() const
{ return m->switchID; }

uint32_t Host::switchPort() const
{ return m->switchPort; }

json11::Json Host::to_json() const
{
    return json11::Json::object {
        {"ID", id_str()},
        {"mac", mac()},
        {"switch_id", uint64_to_string(switchID())},
        {"switch_port", (int)switchPort()}
    };
}

json11::Json Host::formFloodlightJSON()
{
    json11::Json attach = json11::Json::object {
        {"switchDPID", AppObject::uint64_to_string(switchID())},
        {"port", (int)switchPort()},
        {"errorStatus", "null"}
    };

    return json11::Json::object {
        {"entityClass", "DefaultEntityClass"},
        {"mac", mac()},
        {"ipv4", "[]"},
        {"vlan", "[]"},
        {"attachmentPoint", attach},
        {"lastSeen", uint64_to_string(static_cast<uint64_t>(connectedSince()))}
    };
}

void Host::switchID(uint64_t id)
{ m->switchID = id; }

void Host::switchPort(uint32_t port)
{ m->switchPort = port; }

void Host::ip(std::string ip)
{ m->ip = IPAddress(ip); }

HostManager::HostManager()
{
    m = new HostManagerImpl;
}

HostManager::~HostManager()
{ delete m; }

void HostManager::init(Loader *loader, const Config &config)
{
    m_switch_manager = SwitchManager::get(loader);
    auto ctrl = Controller::get(loader);
    ctrl->registerHandler(this);

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
                     this, &HostManager::onSwitchDiscovered);
    QObject::connect(m_switch_manager, &SwitchManager::switchDown,
                     this, &HostManager::onSwitchDown);

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "hosts");
}

void HostManager::onSwitchDiscovered(Switch* dp)
{
    QObject::connect(dp, &Switch::portUp, this, &HostManager::newPort);
}

void HostManager::onSwitchDown(Switch *dp)
{
    delHostForSwitch(dp);
    for (of13::Port port : dp->ports()) {
        auto pos = std::find(switch_macs.begin(), switch_macs.end(), port.hw_addr().to_string());
        if (pos != switch_macs.end())
            switch_macs.erase(pos);
    }
}

void HostManager::addHost(Switch* sw, IPAddress ip, std::string mac, uint32_t port)
{
    std::lock_guard<std::mutex> lk(mutex);

    Host* dev = createHost(mac, ip);
    attachHost(mac, sw->id(), port);
    addEvent(Event::Add, dev);
    dev->connectedSince(time(NULL));
}

Host* HostManager::createHost(std::string mac, IPAddress ip)
{
    Host* dev = new Host(mac, ip);
    m->hosts[mac] = dev;
    emit hostDiscovered(dev);
    return dev;
}

bool HostManager::findMac(std::string mac)
{
    if (m->hosts.count(mac) > 0)
        return true;
    return false;
}

bool HostManager::isSwitch(std::string mac)
{
    for (auto sw_mac : switch_macs) {
        if (sw_mac == mac)
            return true;
    }
    return false;
}

void HostManager::attachHost(std::string mac, uint64_t id, uint32_t port)
{
    m->hosts[mac]->switchID(id);
    m->hosts[mac]->switchPort(port);
}

void HostManager::delHostForSwitch(Switch *dp)
{
    auto itr = m->hosts.begin();
    while (itr != m->hosts.end()) {
        if (itr->second->switchID() == dp->id()) {
            addEvent(Event::Delete, itr->second);
            m->hosts.erase(itr++);
        }
        else
            ++itr;
    }
}

Host* HostManager::getHost(std::string mac)
{
    if (m->hosts.count(mac) > 0)
        return m->hosts[mac];
    else
        return nullptr;
}

Host* HostManager::getHost(IPAddress ip)
{
    for (auto it : m->hosts) {
        if (it.second->ip() == AppObject::uint32_t_ip_to_string(ip.getIPv4()))
            return it.second;
    }
    return nullptr;
}

void HostManager::newPort(Switch *dp, of13::Port port)
{
    switch_macs.push_back(port.hw_addr().to_string());
}

OFMessageHandler::Action HostManager::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    uint16_t eth_type = flow->pkt()->readEthType();
    EthAddress eth_src = flow->pkt()->readEthSrc();
    std::string host_mac = eth_src.to_string();

    IPAddress host_ip("0.0.0.0");
    if (eth_type == 0x0800) {
        host_ip = flow->pkt()->readIPv4Src();
    }
    else if (eth_type == 0x0806) {
        host_ip = flow->pkt()->readARPSPA();
    }

    if (app->isSwitch(host_mac))
        return Continue;

    uint32_t in_port = flow->pkt()->readInPort();
    if (in_port > of13::OFPP_MAX)
        return Continue;

    if (!app->findMac(host_mac)) {
        Switch* sw = app->m_switch_manager->getSwitch(ofconn);
        app->addHost(sw, host_ip, host_mac, in_port);

        LOG(INFO) << "Host discovered. MAC: " << host_mac
                  << ", IP: " << AppObject::uint32_t_ip_to_string(host_ip.getIPv4())
                  << ", Switch ID: " << sw->id() << ", port: " << in_port;
    }
    else {
        Host* h = app->getHost(host_mac);
        std::string s_ip = AppObject::uint32_t_ip_to_string(host_ip.getIPv4());
        if (s_ip != "0.0.0.0") {
            h->ip(s_ip);
        }
    }

    return Continue;
}

bool HostManager::isPrereq(const std::string &name) const
{
    return name == "link-discovery";
}

std::unordered_map<std::string, Host*> HostManager::hosts()
{
    return m->hosts;
}

json11::Json HostManager::handleGET(std::vector<std::string> params, std::string body)
{
    if (params[0] == "hosts") {
        return json11::Json(hosts()).dump();
    }

    return "{}";
}
