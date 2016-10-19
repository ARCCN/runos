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

REGISTER_APPLICATION(SwitchStats, {"switch-manager", "controller", "rest-listener", ""})

port_packets_bytes::port_packets_bytes(uint32_t pn, uint64_t tp, uint64_t rp, uint64_t tb, uint64_t rb):
    port_no(pn),
    tx_packets(tp),
    rx_packets(rp),
    tx_bytes(tb),
    rx_bytes(rb),
    tx_byte_speed(0),
    rx_byte_speed(0) { }

port_packets_bytes::port_packets_bytes():
    port_no(0),
    tx_packets(0),
    rx_packets(0),
    tx_bytes(0),
    rx_bytes(0),
    tx_byte_speed(0),
    rx_byte_speed(0) { }

SwitchPortStats::SwitchPortStats(Switch *_sw): sw(_sw) { }

SwitchPortStats::SwitchPortStats(): sw(nullptr) { }

json11::Json port_packets_bytes::to_json() const
{
    return json11::Json::object {
        {"port_no", (int)port_no},
        {"tx_packets", boost::lexical_cast<std::string>(tx_packets)},
        {"rx_packets", boost::lexical_cast<std::string>(rx_packets)},
        {"tx_bytes", boost::lexical_cast<std::string>(tx_bytes)},
        {"rx_bytes", boost::lexical_cast<std::string>(rx_bytes)},
        {"tx_byte_speed", boost::lexical_cast<std::string>(tx_byte_speed)},
        {"rx_byte_speed", boost::lexical_cast<std::string>(rx_byte_speed)}
    };
}

uint64_t port_packets_bytes::id() const
{
    return static_cast<uint64_t>(port_no);
}

std::vector<port_packets_bytes> SwitchPortStats::getPPB_vec()
{
    std::vector<port_packets_bytes> vec;
    for (auto it : port_stats) {
        vec.push_back(it.second);
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
    acceptPath(Method::GET, "port_info/[0-9]+/all");
    acceptPath(Method::GET, "port_info/[0-9]+/[0-9]+");
}

void SwitchStats::startUp(Loader* provider)
{
    m_timer->start(c_poll_interval * 1000);
}

void SwitchStats::newSwitch(Switch *sw)
{
    SwitchPortStats newswitch(sw);
    switch_stats.insert(std::pair<uint64_t, SwitchPortStats>(sw->id(), newswitch));
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
    float tx_byte_speed = 0;
    float rx_byte_speed = 0;
    for (auto& i : s)
    {
        uint64_t sw_id = conn->dpid();
        uint32_t port_no = i.port_no();
        uint64_t tx_packets = i.tx_packets();
        uint64_t rx_packets = i.rx_packets();
        uint64_t tx_bytes = i.tx_bytes();
        uint64_t rx_bytes = i.rx_bytes();
        // check and count bytes per second
        port_packets_bytes newstat(port_no, tx_packets, rx_packets, tx_bytes, rx_bytes);

        // find switch in old data
        SwitchPortStats& sps = switch_stats.at(sw_id);
        try {
            // find port in old data
            port_packets_bytes& ppb = sps.getElem(port_no);
            tx_byte_speed = float((tx_bytes - ppb.tx_bytes)) / c_poll_interval;
            rx_byte_speed = float((rx_bytes - ppb.rx_bytes)) / c_poll_interval;
            ppb = newstat;
            ppb.tx_byte_speed = tx_byte_speed;
            ppb.rx_byte_speed = rx_byte_speed;
        }
        catch (const std::out_of_range&) {
            // no old data for this port, speed is not calculated
            sps.insertElem(std::pair<uint32_t, port_packets_bytes>(port_no, newstat));
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
        if (switch_stats.count(sw->id()))
            pdescr->request(sw->connection(), req);
    }
}

json11::Json SwitchStats::handleGET(std::vector<std::string> params, std::string body)
{
    uint64_t id = std::stoull(params[1]);
    if (params[2] == "all") {
        return json11::Json(switch_stats[id].getPPB_vec());
    }
    else {
        int port = atoi(params[2].c_str());
        return json11::Json(switch_stats[id].port_stats[port]);
    }
    return "{}";
}

