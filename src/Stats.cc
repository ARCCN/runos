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

#include "Stats.hh"

#include <boost/lexical_cast.hpp>

#include "SwitchConnection.hh"
#include "Controller.hh"
#include "RestListener.hh"

#define SHOW(a) {#a, std::to_string(stats1->a())}


REGISTER_APPLICATION(SwitchStats, {"switch-manager", "controller", "rest-listener", ""})

port_packets_bytes::port_packets_bytes(of13::PortStats stats): stats(stats) {}

port_packets_bytes::port_packets_bytes(): stats{} {}

SwitchPortStats::SwitchPortStats(Switch *_sw): sw(_sw) { }

SwitchPortStats::SwitchPortStats(): sw(nullptr) { }

json11::Json port_packets_bytes::to_json() const
{
    auto stats1 = const_cast<of13::PortStats *>(&stats);
    return json11::Json::object {
            SHOW(tx_dropped),
            SHOW(rx_packets),
            SHOW(rx_crc_err),
            SHOW(tx_bytes),
            SHOW(rx_dropped),
            SHOW(port_no),
            SHOW(rx_over_err),
            SHOW(rx_frame_err),
            SHOW(rx_bytes),
            SHOW(tx_errors),
            SHOW(duration_nsec),
            SHOW(collisions),
            SHOW(duration_sec),
            SHOW(rx_errors),
            SHOW(tx_packets)
    };
}

uint64_t port_packets_bytes::id() const
{
    auto stats1 = const_cast<of13::PortStats *>(&stats);
    return static_cast<uint64_t>(stats1->port_no());
}

std::vector<port_packets_bytes> SwitchPortStats::to_vector()
{
    std::vector<port_packets_bytes> vec;
    for (auto it : port_stats) {
        vec.emplace_back(it.second);
    }
    return vec;
}

void SwitchStats::init(Loader *loader, const Config& rootConfig)
{
    /* Initialize members */
    m_timer = new QTimer(this);

    /* Read configuration */
    auto config = config_cd(rootConfig, "switch-stats");
    c_poll_interval = config_get(config, "poll-interval", 15);

    /* Get dependencies */
    m_switch_manager = SwitchManager::get(loader);

    pdescr = Controller::get(loader)->registerStaticTransaction(this);
    QObject::connect(pdescr, &OFTransaction::response,
                     this, &SwitchStats::portStatsArrived);

    QObject::connect(pdescr, &OFTransaction::error,
    [](SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> msg) {
        of13::Error& error = msg->error;
        LOG(ERROR) << "Switch reports error for OFPT_MULTIPART_REQUEST: "
            << "type " << (int) error.type() << " code " << error.code();
    });

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
                     this, &SwitchStats::newSwitch);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(pollTimeout()));

    RestListener::get(loader)->registerRestHandler(this);
    // port/<dpid>/[all, <port_id>]
    acceptPath(Method::GET, "port/[0-9]+/(all|[0-9]+)");
}

void SwitchStats::startUp(Loader* provider)
{
    m_timer->start(c_poll_interval * 1000);
}

void SwitchStats::newSwitch(Switch *sw)
{
    SwitchPortStats newswitch(sw);
    all_switches_stats.insert(std::pair<uint64_t, SwitchPortStats>(sw->id(), newswitch));
}

void SwitchStats::portStatsArrived(SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> reply)
{
    auto type = reply->base()->type();
    if (type != of13::OFPT_MULTIPART_REPLY) {
        LOG(ERROR) << "Unexpected response of type " << type
                << " received, expected OFPT_MULTIPART_REPLY";
        return;
    }

    of13::MultipartReplyPortStats stats = reply->multipartReplyPortStats;

    std::vector<of13::PortStats> s = stats.port_stats();
    for (auto& i : s)
    {
        uint64_t sw_id = conn->dpid();
        // check and count bytes per second
        port_packets_bytes newstat{i};

        // find switch in old data
        SwitchPortStats& sps = all_switches_stats.at(sw_id);

        try {
            // find port in old data
            sps.getElem(i.port_no()) = newstat;
        }
        catch (const std::out_of_range&) {
            // no old data for this port, speed is not calculated
            sps.insertElem(std::pair<uint32_t, port_packets_bytes>(i.port_no(), newstat));
        }
    }
}

void SwitchStats::pollTimeout()
{
    auto switches = m_switch_manager->switches();
    for (auto sw : switches)
    {
        of13::MultipartRequestPortStats req;
        req.flags(0);
        req.port_no(of13::OFPP_ANY);
        if (all_switches_stats.count(sw->id()))
            pdescr->request(sw->connection(), req);
    }
}

json11::Json SwitchStats::handleGET(std::vector<std::string> params, std::string body)
{
    uint64_t dpid = std::stoull(params[1]);
    if (params[2] == "all") {
        return json11::Json::object{std::make_pair(std::to_string(dpid),
                                                   all_switches_stats[dpid].to_vector())};
    }
    int port = atoi(params[2].c_str());
    return json11::Json::object{std::make_pair(std::to_string(dpid),
                                               json11::Json::array{all_switches_stats[dpid].port_stats[port]})};
}

