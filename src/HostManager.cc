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

REGISTER_APPLICATION(HostManager, {"switch-manager", "rest-listener", ""})

struct HostImpl {
    uint64_t id;
    std::string mac;
    uint64_t switchID;
    uint32_t switchPort;
};

struct HostManagerImpl {
    // mac address -> Host
    std::unordered_map<std::string, Host*> hosts;
};

Host::Host(std::string mac)
{
    m = new HostImpl;
    m->mac = mac;
    m->id = rand()%1000 + 1000;
}

Host::~Host()
{ delete m; }

uint64_t Host::id() const
{ return m->id; }

std::string Host::mac() const
{ return m->mac; }

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

HostManager::HostManager()
{
    m = new HostManagerImpl;
    r = new HostManagerRest("Host Manager", "none");
    r->m = this;
    r->makeEventApp();
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

    RestListener::get(loader)->newListener("host-manager", r);
}

void HostManager::onSwitchDiscovered(Switch* dp)
{
    QObject::connect(dp, &Switch::portUp, this, &HostManager::newPort);
}

Host* HostManager::addHost(std::string mac)
{
    Host* dev = new Host(mac);
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

Host* HostManager::getHost(std::string mac)
{ return m->hosts[mac]; }

void HostManager::newPort(Switch *dp, of13::Port port)
{
    switch_macs.push_back(port.hw_addr().to_string());
}

OFMessageHandler::Action HostManager::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    EthAddress eth_src = flow->pkt()->readEthSrc();
    std::string eth_mac = eth_src.to_string();
    if (app->isSwitch(eth_mac))
        return Continue;

    uint32_t in_port = flow->pkt()->readInPort();
    if (in_port > of13::OFPP_MAX)
        return Continue;

    if (!app->findMac(eth_mac)) {
        std::mutex mutex;

        Switch* sw = app->m_switch_manager->getSwitch(ofconn);

        mutex.lock();
        Host* dev = app->addHost(eth_mac);
        app->attachHost(eth_mac, sw->id(), in_port);

        app->r->addEvent(Event::Add, dev);
        dev->connectedSince(time(NULL));
        mutex.unlock();

        LOG(INFO) << "Host discovered. MAC: " << eth_src.to_string()
                  << ", Switch ID: " << sw->id() << ", port: " << in_port;
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

std::string HostManagerRest::handle(std::vector<std::string> params)
{
    if (params[0] != "GET")
        return "{ \"error\": \"error\" }";
    if (params[2] == "hosts") {
        return json11::Json(m->hosts()).dump();
    }
    if (params[2] == "f_hosts") {
        std::vector<json11::Json> res;
        for (auto it : m->hosts()) {
            res.push_back(it.second->formFloodlightJSON());
        }
        return json11::Json(res).dump();
    }

    return "{}";
}

int HostManagerRest::count_objects()
{
    return m->hosts().size();
}
