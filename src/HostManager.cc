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

#include <boost/lexical_cast.hpp>
#include <fluid/util/ipaddr.hh>

#include "api/Packet.hh"
#include "api/PacketMissHandler.hh"
#include "api/TraceablePacket.hh"
#include "Controller.hh"
#include "Flow.hh"
#include "oxm/openflow_basic.hh"
#include "RestListener.hh"
#include "SwitchConnection.hh"

REGISTER_APPLICATION(HostManager, {"switch-manager", "rest-listener", ""})

struct HostImpl {
    uint64_t id;
    std::string mac;
    IPv4Addr ip;
    uint64_t switchID;
    uint32_t switchPort;
};

struct HostManagerImpl {
    // mac address -> Host
    std::unordered_map<std::string, Host*> hosts;
};

Host::Host(std::string mac, IPv4Addr ip)
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
{ return boost::lexical_cast<std::string>(m->ip); }

uint64_t Host::switchID() const
{ return m->switchID; }

uint32_t Host::switchPort() const
{ return m->switchPort; }

json11::Json Host::to_json() const
{
    return json11::Json::object {
        {"ID", id_str()},
        {"mac", mac()},
        {"switch_id", boost::lexical_cast<std::string>(switchID())},
        {"switch_port", (int)switchPort()}
    };
}

json11::Json Host::formFloodlightJSON()
{
    json11::Json attach = json11::Json::object {
        {"switchDPID", boost::lexical_cast<std::string>(switchID())},
        {"port", (int)switchPort()},
        {"errorStatus", "null"}
    };

    return json11::Json::object {
        {"entityClass", "DefaultEntityClass"},
        {"mac", mac()},
        {"ipv4", "[]"},
        {"vlan", "[]"},
        {"attachmentPoint", attach},
        {"lastSeen", boost::lexical_cast<std::string>(static_cast<uint64_t>(connectedSince()))}
    };
}

void Host::switchID(uint64_t id)
{ m->switchID = id; }

void Host::switchPort(uint32_t port)
{ m->switchPort = port; }

void Host::ip(std::string ip)
{ m->ip = IPv4Addr(ip); }

void Host::ip(IPv4Addr ip)
{ m->ip = IPv4Addr(ip); }

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

    ctrl->registerHandler("host-manager",
        [this](SwitchConnectionPtr connection) {
            const auto ofb_in_port = oxm::in_port();
            const auto ofb_eth_type = oxm::eth_type();
            const auto ofb_eth_src = oxm::eth_src();
            const auto ofb_arp_spa = oxm::arp_spa();
            const auto ofb_ipv4_src = oxm::ipv4_src();

            return [=](Packet& pkt, FlowPtr, Decision decision) {
                auto tpkt = packet_cast<TraceablePacket>(pkt);

                ethaddr eth_src = pkt.load(ofb_eth_src);
                std::string host_mac = boost::lexical_cast<std::string>(eth_src);

                IPv4Addr host_ip("0.0.0.0");
                if (pkt.test(ofb_eth_type == 0x0800)) {
                    host_ip = tpkt.watch(ofb_ipv4_src);
                } else if (pkt.test(ofb_eth_type == 0x0806)) {
                    host_ip = IPv4Addr(tpkt.watch(ofb_arp_spa));
                }

                if (isSwitch(host_mac))
                    return decision;

                uint32_t in_port = tpkt.watch(ofb_in_port);
                if (in_port > of13::OFPP_MAX)
                    return decision;

                if (not findMac(host_mac)) {
                    Switch* sw = m_switch_manager->getSwitch(connection->dpid());
                    addHost(sw, host_ip, host_mac, in_port);

                    LOG(INFO) << "Host discovered. MAC: " << host_mac
                              << ", IP: " << host_ip
                              << ", Switch ID: " << sw->id() << ", port: " << in_port;
                } else {
                    Host* h = getHost(host_mac);
                    if (host_ip != "0.0.0.0") {
                        h->ip(host_ip);
                    }
                }

                return decision;
            };
        }
    );

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

void HostManager::addHost(Switch* sw, IPv4Addr ip, std::string mac, uint32_t port)
{
    std::lock_guard<std::mutex> lk(mutex);

    Host* dev = createHost(mac, ip);
    attachHost(mac, sw->id(), port);
    addEvent(Event::Add, dev);
    dev->connectedSince(time(NULL));
    emit hostDiscovered(dev);
}

Host* HostManager::createHost(std::string mac, IPv4Addr ip)
{
    Host* dev = new Host(mac, ip);
    m->hosts[mac] = dev;
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

Host* HostManager::getHost(IPv4Addr ip)
{
    auto string_ip = boost::lexical_cast<std::string>(ip);
    for (auto it : m->hosts) {
        if (it.second->ip() == string_ip)
            return it.second;
    }
    return nullptr;
}

void HostManager::newPort(Switch *, of13::Port port)
{
    switch_macs.push_back(port.hw_addr().to_string());
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
