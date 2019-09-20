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

// TODO: security: add cookie to LLDP messages

#include "LinkDiscovery.hpp"
#include "LinkDiscoveryDriver.hpp"

#include "LLDP.hpp"
#include "Recovery.hpp"
#include "SwitchManager.hpp"
#include "DatabaseConnector.hpp"
#include "lib/poller.hpp"
#include <runos/core/logging.hpp>

#include <fluid/of13/of13match.hh>

#include <algorithm>
#include <chrono>

namespace runos {

//using namespace boost::endian;
namespace of13 = fluid_msg::of13;

REGISTER_APPLICATION(LinkDiscovery, {"controller", "recovery-manager",
                                     "switch-manager", "switch-ordering",
                                     "database-connector", ""})

void LinkDiscovery::init(Loader *loader, const Config &rootConfig)
{
    qRegisterMetaType<switch_and_port>("switch_and_port");

    /* Read configuration */
    auto config = config_cd(rootConfig, "link-discovery");
    c_poll_interval = config_get(config, "poll-interval", 5);
    queue_id = config_get(config, "queue", -1);

    /* Get dependencies */
    recovery = RecoveryManager::get(loader);
    m_switch_manager = SwitchManager::get(loader);
    db_connector_ = DatabaseConnector::get(loader);
    poller = new Poller(this, c_poll_interval * 1000);

    /* Do logging and save to DB */
    connect(this, &LinkDiscovery::linkDiscovered,
         [this](switch_and_port from, switch_and_port to) {
             save_to_database();
             LOG(INFO) << "[LinkDiscovery] Link discovered - "
                       << from << " <-> " << to;
         });
    connect(this, &LinkDiscovery::linkBroken,
         [this](switch_and_port from, switch_and_port to) {
             save_to_database();
             LOG(INFO) << "[LinkDiscovery] Link broken - "
                       << from << " <-> " << to;
        });

    /* Connect with other applications */
    connect(recovery, &RecoveryManager::signalRecovery,
            this, &LinkDiscovery::load_from_database);

    handler = Controller::get(loader)->register_handler(
        [this](of13::PacketIn& pi, OFConnectionPtr connection) {
            if (not recovery->isPrimary()) return false;

            if (pi.cookie() == ~((uint64_t)0)) {
                VLOG(15) << "LinkDiscovery: Receive PacketIn with cookie = -1";
                if (sizeof(lldp_packet) > pi.data_len()) {
                    return false;
                }
                // Look into packet to check if it is LLDP
                auto lldp(static_cast<lldp_packet*>(pi.data()));
                if (lldp->eth_type != 0x88cc) {
                    VLOG(15) << "LinkDiscovery: PacketIn is not lldp packet";
                    return false;
                }
            }
            else if (pi.cookie() != (1 << 16) + 0x11D0) { 
                return false;
            }
            
            VLOG(15) << "LLDP packet received on " << connection->dpid();

            switch_and_port source;
            auto lldp(static_cast<lldp_packet*>(pi.data()));
            if (lldp->eth_type == 0x8100) { // tagged lldp
                auto tagged_lldp(static_cast<tagged_lldp_packet*>(pi.data()));
                if (sizeof(tagged_lldp_packet) > pi.data_len()
                        or tagged_lldp->dpid_oui != 0x0026e1) {
                    VLOG(15) << "[LinkDiscovery] LLDP error - "
                             <<"Recieved incorrect LLDP on "
                             << connection->dpid()
                             << ":" << pi.match().in_port()->value();
                    return false;
                }

                source.dpid = tagged_lldp->dpid_data;
                source.port = tagged_lldp->port_id_sub_component;
            }
            else { // untagged lldp
                if (sizeof(lldp_packet) > pi.data_len()
                        or lldp->dpid_oui != 0x0026e1) {
                    VLOG(15) << "[LinkDiscovery] LLDP error - "
                             <<"Recieved incorrect LLDP on "
                             << connection->dpid()
                             << ":" << pi.match().in_port()->value();
                    return false;
                }

                source.dpid = lldp->dpid_data;
                source.port = lldp->port_id_sub_component;
            }

            switch_and_port target { connection->dpid(),
                                     pi.match().in_port()->value() };

            VLOG(15) << "LLDP packet received on " << target;

            //handle lldp in libfluid threads
            handleBeacon(source, target);

            return true;
        }, -10);

    SwitchOrderingManager::get(loader)->registerHandler(this, 50);
}

void LinkDiscovery::startUp(Loader *)
{
    // Start LLDP polling
    poller->run();
}

switch_and_port LinkDiscovery::other(switch_and_port sp) const
{
    std::lock_guard<std::mutex> lock(links_mutex);

    auto out_it = m_out_edges.find(sp);
    if (out_it == m_out_edges.end()) {
        return switch_and_port {0, 0};
    }

    auto edge_it = out_it->second;
    switch_and_port target;

    if (edge_it->source == sp)
        target = edge_it->target;
    else
        target = edge_it->source;

    DiscoveredLink link(*edge_it);
    auto link_it = find_link(m_links, link);
    CHECK(link_it != m_links.end());

    return target;
}

void LinkDiscovery::handleBeacon(switch_and_port from, switch_and_port to)
{
    if (m_switch_manager->switch_(from.dpid) == nullptr ||
        m_switch_manager->switch_(to.dpid) == nullptr) {
        LOG(WARNING) << "[LinkDiscovery] LLDP warning - "
                     <<"Arrived LLDP from another RUNOS network";
        return;
    }

    DiscoveredLink link{ from, to,
                         std::chrono::steady_clock::now() +
                         std::chrono::seconds(c_poll_interval * 2) };
    DiscoveredLink reversed{ to, from, link.valid_through };
    bool emit_on_add { false };
    std::vector<std::optional<link_pair>> emit_on_remove;

    { // lock
    std::lock_guard<std::mutex> lock(links_mutex);

    auto link_it     = find_link(m_waiting_links, link);
    auto reversed_it = find_link(m_waiting_links, reversed);

    if (reversed_it != m_waiting_links.end()) {
        // Reversed link exists, add (or update) bidirectional link
        remove_waiting_link(to, from);
        emit_on_add = add_link(link);
    }
    else if (link_it != m_waiting_links.end()) {
        // Link of the same direction
    }
    else {
        // Remove broken links, if any
        emit_on_remove.emplace_back(remove_broken_link(from));
        emit_on_remove.emplace_back(remove_broken_link(to));
    }

    add_waiting_link(link);
    } // unlock

    if (emit_on_add) {
        emit linkDiscovered(link.source, link.target);
    }

    for (const auto& pair : emit_on_remove) {
        if (pair) {
            emit linkBroken(pair->first, pair->second);
        }
    }
}

void LinkDiscovery::clearLinkAt(PortPtr port)
{
    switch_and_port source{port->switch_()->dpid(), port->number()};
    std::optional<link_pair> to_remove;
    VLOG(15) << "clearLinkAt " << source;

    { // lock
    std::lock_guard<std::mutex> lock(links_mutex);
    to_remove = remove_broken_link(source);
    } // unlock

    if (to_remove) {
        emit linkBroken(to_remove->first, to_remove->second);
    }
} 

void LinkDiscovery::polling()
{
    // Backup controller
    if (not recovery->isPrimary()) {
        load_from_database();
        return;
    }

    // Master controller
    auto now = std::chrono::steady_clock::now();

    std::vector<link_pair> links_to_delete;

    {//lock
        std::lock_guard<std::mutex> lock(links_mutex);

        // Remove all expired links
        while (!m_links.empty() && m_links.begin()->valid_through < now) {
            auto top = m_links.begin();
            links_to_delete.push_back(std::make_pair(top->source, top->target));
            CHECK(m_out_edges.erase(top->source) == 1);
            CHECK(m_out_edges.erase(top->target) == 1);
            CHECK(top != m_links.end());
            m_links.erase(top);
        }

        // Remove expired links from waiting list
        while (!m_waiting_links.empty() &&
                m_waiting_links.begin()->valid_through < now) {
            auto top = m_waiting_links.begin();
            CHECK(top != m_waiting_links.end());
            m_waiting_links.erase(top);
        }
    }//unlock

    // emit all signals
    for (auto& link: links_to_delete) {
        emit linkBroken(link.first, link.second);
    }

    // Send LLDP packets to all known ports
    for (SwitchPtr sw : m_switch_manager->switches()) {
        sendLLDP s(*this, sw);
        s.sendLLDPtoPorts();
    }
}

LinkDiscovery::links_set_iterator
LinkDiscovery::find_link(const links_set& set, const DiscoveredLink& link) const
{
    return std::find_if(set.begin(), set.end(), [&](DiscoveredLink l){
        return std::tie(link.source, link.target) == std::tie(l.source, l.target);
    });
}

bool LinkDiscovery::add_link(DiscoveredLink& link)
{
    bool isNew = true;

    if (link.source > link.target)
        std::swap(link.source, link.target);

    auto link_it = find_link(m_links, link);
    if (link_it != m_links.end()) {
        m_links.erase(link_it);
        isNew = false;
    }

    auto it = m_links.insert(m_links.end(), link);
    m_out_edges[link.source] = it;
    m_out_edges[link.target] = it;

    return isNew;
}

LinkDiscovery::link_pair
LinkDiscovery::remove_link(switch_and_port from, switch_and_port to)
{
    if (from > to)
        std::swap(from, to);

    DiscoveredLink link{ from, to, DiscoveredLink::valid_through_t() };

    auto link_it = find_link(m_links, link);

    CHECK(link_it != m_links.end());
    m_links.erase(link_it);
    CHECK(m_out_edges.erase(from) == 1);
    CHECK(m_out_edges.erase(to) == 1);

    return { from, to };
}

std::optional<LinkDiscovery::link_pair>
LinkDiscovery::remove_broken_link(switch_and_port from)
{
    auto out_edge = m_out_edges.find(from);

    if (out_edge == m_out_edges.end()) {
        return {};
    }

    auto old_from = out_edge->second->source;
    auto old_to   = out_edge->second->target;
    auto other_it = m_out_edges.find(old_to);

    CHECK(other_it != m_out_edges.end());
    return remove_link(old_from, old_to);
}

void LinkDiscovery::add_waiting_link(DiscoveredLink link)
{
    auto link_it = find_link(m_waiting_links, link);
    if (link_it != m_waiting_links.end())
        m_waiting_links.erase(link_it);

    m_waiting_links.insert(m_waiting_links.end(), link);
}

void LinkDiscovery::remove_waiting_link(switch_and_port from, switch_and_port to)
{
    DiscoveredLink link{ from, to, DiscoveredLink::valid_through_t() };

    auto link_it = find_link(m_waiting_links, link);
    if (link_it == m_waiting_links.end()) {
        return;
    }

    m_waiting_links.erase(link_it);
}

void LinkDiscovery::switchUp(SwitchPtr sw)
{
    if (recovery->isPrimary()) {
        sw->handle(onSwitchUp(sw));
    } else {
        load_from_database();
    }
}

void LinkDiscovery::linkUp(PortPtr port)
{
    if (recovery->isPrimary()) {
        (port->switch_())->handle(sendLLDP(*this,port));
    }
}

void LinkDiscovery::linkDown(PortPtr port)
{
    if (recovery->isPrimary()) {
        clearLinkAt(port);
    }
}

void LinkDiscovery::save_to_database()
{
    auto jdump = nlohmann::json::array();
    for (const auto& l : links()) {
        jdump.push_back(l.to_json());
    }
    db_connector_->putSValue("link-discovery", "links", jdump.dump());
}

void LinkDiscovery::load_from_database()
{
    std::lock_guard<std::mutex> lock(links_mutex);

    m_links.clear();
    m_waiting_links.clear();
    m_out_edges.clear();

    auto time = std::chrono::steady_clock::now() + 
                std::chrono::seconds(c_poll_interval * 2); 
    auto links = db_connector_->getSValue("link-discovery", "links");
    if (links.empty()) {
        return;
    }

    auto links_json = nlohmann::json::parse(links);
    for (const auto& l_json : links_json) {
        if (not m_switch_manager->switch_(l_json["source_dpid"]) ||
            not m_switch_manager->switch_(l_json["target_dpid"])) {
            continue; // skip unknown switch
        }

        switch_and_port from {l_json["source_dpid"], l_json["source_port"]};
        switch_and_port to   {l_json["target_dpid"], l_json["target_port"]};
        DiscoveredLink link {std::move(from), std::move(to), time};

        add_link(link);
        add_waiting_link(link);
    }
}


LinkDiscovery::~LinkDiscovery()
{
    delete poller;
}

} // namespace runos
