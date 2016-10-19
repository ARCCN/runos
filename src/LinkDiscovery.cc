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

// TODO: security: add cookie to LLDP messages
// TODO: test on unstable links (broken by timeout)

#include "LinkDiscovery.hh"

#include <cstdint>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include "LLDP.hh"

#include "api/PacketMissHandler.hh"
#include "api/Packet.hh"
#include "api/SerializablePacket.hh"
#include "api/TraceablePacket.hh"
#include "oxm/openflow_basic.hh"
#include "SwitchConnection.hh"
#include "Flow.hh"

using namespace boost::endian;
using namespace runos;

REGISTER_APPLICATION(LinkDiscovery, {"switch-manager", "controller", ""})

big_uint16_t lldp_tlv_header(big_uint16_t type, big_uint16_t length)
{
    return (type << big_uint16_t(9)) | length;
}

struct lldp_packet {
    big_uint48_t dst_mac {0x0180c200000eULL};
    big_uint48_t src_mac;
    big_uint16_t eth_type {LLDP_ETH_TYPE};

    big_uint16_t chassis_id_header {lldp_tlv_header(LLDP_CHASSIS_ID_TLV, 7)};
    big_uint8_t chassis_id_sub {LLDP_CHASSIS_ID_SUB_MAC};
    big_uint48_t chassis_id_sub_mac;

    big_uint16_t port_id_header {lldp_tlv_header(LLDP_PORT_ID_TLV, 5)};
    big_uint8_t port_id_sub {LLDP_PORT_ID_SUB_COMPONENT};
    big_uint32_t port_id_sub_component;

    big_uint16_t ttl_header {lldp_tlv_header(LLDP_TTL_TLV, 2)};
    big_uint16_t ttl_seconds;

    big_uint16_t dpid_header {lldp_tlv_header(127, 12)};
    big_uint24_t dpid_oui {0x0026e1}; // OpenFlow
    big_uint8_t  dpid_sub {0};
    big_uint64_t dpid_data;

    big_uint16_t end {0};
};
static_assert(sizeof(lldp_packet) == 50, "Unexpected alignment");

void LinkDiscovery::init(Loader *loader, const Config &rootConfig)
{
    qRegisterMetaType<switch_and_port>();

    /* Read configuration */
    auto config = config_cd(rootConfig, "link-discovery");
    c_poll_interval = config_get(config, "poll-interval", 120);

    /* Get dependencies */
    ctrl  = Controller::get(loader);
    m_switch_manager     = SwitchManager::get(loader);

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
         [this](Switch* dp) {
             QObject::connect(dp, &Switch::portUp, this, &LinkDiscovery::portUp);
             QObject::connect(dp, &Switch::portDown, this, &LinkDiscovery::portDown);
             QObject::connect(dp, &Switch::portModified, this, &LinkDiscovery::portModified);
         });

    /* Do logging */
    QObject::connect(this, &LinkDiscovery::linkDiscovered,
         [](switch_and_port from, switch_and_port to) {
             LOG(INFO) << "Link discovered: "
                     << from.dpid << ':' << from.port << " -> "
                     << to.dpid << ':' << to.port;
         });
    QObject::connect(this, &LinkDiscovery::linkBroken,
         [](switch_and_port from, switch_and_port to) {
             LOG(INFO) << "Link broken: "
                     << from.dpid << ':' << from.port << " -> "
                     << to.dpid << ':' << to.port;

        });

    /* Connect with other applications */
    ctrl->registerHandler("link-discovery",
        [this](SwitchConnectionPtr connection) {
            const auto ofb_in_port = oxm::in_port();
            const auto ofb_eth_type = oxm::eth_type();

            return [=](Packet& pkt, FlowPtr, Decision decision) {
                if (not pkt.test(ofb_eth_type == LLDP_ETH_TYPE))
                    return decision;

                lldp_packet lldp;
                auto written = packet_cast<SerializablePacket&>(pkt)
                              //.ethernet()
                              .serialize_to(sizeof lldp, &lldp);
                if (written < sizeof lldp) {
                    LOG(ERROR) << "LLDP packet is too small";
                    return decision.idle_timeout(std::chrono::seconds::zero())
                        .drop()// TODO : inspect(sizeof(lldp_packet))
                                   .return_();
                }

                switch_and_port source
                    = { lldp.dpid_data, lldp.port_id_sub_component };
                switch_and_port target
                    = { target.dpid = connection->dpid(),
                        packet_cast<TraceablePacket>(pkt).watch(ofb_in_port) };

                DVLOG(5) << "LLDP packet received on "
                    << target.dpid << ':' << target.port;

                if (source > target)
                    std::swap(source, target);

                QMetaObject::invokeMethod(this, "handleBeacon",
                                          Qt::QueuedConnection,
                                          Q_ARG(switch_and_port, source),
                                          Q_ARG(switch_and_port, target));
                return decision.idle_timeout(std::chrono::seconds::zero())
                    .drop()// TODO : inspect(sizeof(lldp_packet))
                                   .return_();
            };
        });
}

void LinkDiscovery::startUp(Loader *)
{
    // Start LLDP polling
    startTimer(c_poll_interval * 1000);
}

void LinkDiscovery::portUp(Switch *dp, of13::Port port)
{
    // Don't discover reserved ports
    if (port.port_no() > of13::OFPP_MAX)
        return;

    if (!(port.state() & of13::OFPPS_LINK_DOWN)) {
        // Send first packet immediately
        sendLLDP(dp, port);
    }
}

void LinkDiscovery::portModified(Switch* dp, of13::Port port, of13::Port old_port)
{
    bool live = !(port.state() & of13::OFPPS_LINK_DOWN);
    bool old_live = !(old_port.state() & of13::OFPPS_LINK_DOWN);

    if (live && !old_live) {
        // Send first packet immediately
        sendLLDP(dp, port);
    } else if (!live && old_live) {
        // Remove discovered link if it exists
        clearLinkAt(switch_and_port{dp->id(), port.port_no()});
    }
}

void LinkDiscovery::portDown(Switch *dp, uint32_t port_no)
{
    clearLinkAt(switch_and_port{dp->id(), port_no});
}

void LinkDiscovery::sendLLDP(Switch *dp, of13::Port port)
{
    lldp_packet lldp;
    uint8_t* mac = port.hw_addr().get_data();
    lldp.src_mac = ((uint64_t) mac[0] << 40ULL) |
                   ((uint64_t) mac[1] << 32ULL) |
                   ((uint64_t) mac[2] << 24ULL) |
                   ((uint64_t) mac[3] << 16ULL) |
                   ((uint64_t) mac[4] << 8ULL) |
                    (uint64_t) mac[5];

    // lower 48 bits of datapath id represents switch MAC address
    lldp.chassis_id_sub_mac = dp->id();
    lldp.port_id_sub_component = port.port_no();
    lldp.ttl_seconds = c_poll_interval;
    lldp.dpid_data   = dp->id();

    VLOG(5) << "Sending LLDP packet to " << port.name();
    of13::PacketOut po;
    of13::OutputAction action(port.port_no(), of13::OFPCML_NO_BUFFER);
    po.buffer_id(OFP_NO_BUFFER);
    po.data(&lldp, sizeof lldp);
    po.add_action(action);

    // Send packet 3 times to prevent drops
    dp->connection()->send(po);
    dp->connection()->send(po);
    dp->connection()->send(po);
}

void LinkDiscovery::handleBeacon(switch_and_port from, switch_and_port to)
{
    DiscoveredLink link{ from, to,
                         std::chrono::steady_clock::now() +
                            std::chrono::seconds(c_poll_interval * 2) };
    bool isNew = true;

    // Remove older edge
    auto out_it = m_out_edges.find(from);
    if (out_it != m_out_edges.end()) {
        m_links.erase(out_it->second);
        m_out_edges.erase(from);
        m_out_edges.erase(to);
        isNew = false;
    }

    // Insert new
    auto it = m_links.insert(link).first;
    m_out_edges[from] = it;
    m_out_edges[to] = it;

    if (isNew) {
        emit linkDiscovered(from, to);
    }
}

void LinkDiscovery::clearLinkAt(const switch_and_port &source)
{
    VLOG(5) << "clearLinkAt " << source.dpid << ':' << source.port;

    switch_and_port target;
    auto out_edges_it = m_out_edges.find(source);
    if (out_edges_it == m_out_edges.end())
        return;
    auto edge_it = out_edges_it->second;

    if (edge_it->source == source) {
        target = edge_it->target;
    } else {
        assert(edge_it->target == source);
        target = edge_it->source;
    }

    m_links.erase(edge_it);
    CHECK(m_out_edges.erase(source) == 1);
    CHECK(m_out_edges.erase(target) == 1);

    if (source < target)
        emit linkBroken(source, target);
    else
        emit linkBroken(target, source);
    ctrl->invalidateTraceTree(); // TODO : smart invalidation
}

void LinkDiscovery::timerEvent(QTimerEvent*)
{
    auto now = std::chrono::steady_clock::now();

    // Remove all expired links
    while (!m_links.empty() && m_links.begin()->valid_through < now) {
        auto top = m_links.begin();
        assert(m_out_edges.erase(top->source) == 1);
        assert(m_out_edges.erase(top->target) == 1);
        emit linkBroken(top->source, top->target);
        m_links.erase(top);
    }

    // Send LLDP packets to all known links
    for (Switch* sw : m_switch_manager->switches()) {
        for (of13::Port &port : sw->ports()) {
            if (port.port_no() > of13::OFPP_MAX)
                continue;
            sendLLDP(sw, port);
        }
    }
}
