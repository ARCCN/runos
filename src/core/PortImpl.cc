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

#include "PortImpl.hpp"

#include "SwitchImpl.hpp"
#include <runos/core/assert.hpp>

#include <boost/lexical_cast.hpp>
#include <range/v3/view/map.hpp>

#include <unordered_set>

namespace runos {

constexpr uint32_t xid = 30561;

PortImpl::PortImpl(SwitchImplPtr sw, of13::Port& port, QObject* parent)
    : sw_(sw)
    , number_(port.port_no())
    , hw_addr_(port.hw_addr().to_string())
    , name_(port.name())
    , config_(port.config())
    , state_(port.state())
    , advertised_(port.advertised())
    , supported_(port.supported())
    , peer_(port.peer())
    , current_(port.curr())
    , current_speed_(port.curr_speed())
    , max_speed_(port.max_speed())
    , maintenance_(false)
{
    moveToThread(parent->thread());
    setParent(parent);

    // manual curr_speed from devicedb if speed is unknown
    if (not current_speed_) {
        auto speed = sw->property("current_speed_" + std::to_string(number_), 0);
        speed = (speed ? speed : sw->property("current_speed", 0));
        current_speed_ = speed;
        max_speed_ = speed;
    }

    traffic_stats_->emplace((+ForwardingType::Unicast)._to_string(),
                            StatisticsStore<TrafficMeasurement>());
    traffic_stats_->emplace((+ForwardingType::Multicast)._to_string(),
                            StatisticsStore<TrafficMeasurement>());
    traffic_stats_->emplace((+ForwardingType::Broadcast)._to_string(),
                            StatisticsStore<TrafficMeasurement>());

    QObject::connect(this, &PortImpl::linkUp, sw.get(), &Switch::linkUp);
    QObject::connect(this, &PortImpl::linkDown, sw.get(), &Switch::linkDown);
    QObject::connect(this, &PortImpl::maintenanceStart, sw.get(), &Switch::portMaintenanceStart);
    QObject::connect(this, &PortImpl::maintenanceEnd, sw.get(), &Switch::portMaintenanceEnd);
}

void PortImpl::start()
{
    auto self = shared_from_this();
    if (not link_down()) {
        emit linkUp(self);
    }
}

std::vector<uint32_t> PortImpl::queues() const
{
    auto queues = queue_stats_.synchronize();
    return *queues | ranges::view::keys;
}

void PortImpl::set_config(uint32_t config)
{
    boost::upgrade_lock< boost::shared_mutex > rlock(cmutex);
    if (config != config_) {
        boost::upgrade_to_unique_lock< boost::shared_mutex > wlock(rlock);
        config_ = config;
        emit configChanged(shared_from_this());
    }
}

void PortImpl::set_offline()
{
    if (link_down())
        return;

    state_ |= of13::OFPPS_LINK_DOWN;
    emit linkDown(shared_from_this());
}

void PortImpl::process_event(of13::Port& port)
{
    ASSERT(port.port_no() == number());
    ASSERT(port.name() == name());
    if (port.hw_addr().to_string() != hw_addr_)
        LOG(WARNING)<<"port.hw_addr().to_string() != hw_addr_ (port address have changed)";

    set_config(port.config());

    uint32_t mstate;
    boost::unique_lock< boost::shared_mutex > wlock(cmutex);
    {
        mstate = state_ ^ port.state();
        state_ = port.state();

        advertised_ = port.advertised();
        supported_ = port.supported();
        peer_ = port.peer();
        current_ = port.curr();

        current_speed_ = port.curr_speed();
        max_speed_ = port.max_speed();
    }

    auto self = shared_from_this();
    if (mstate & of13::OFPPS_LINK_DOWN) {
        if (link_down()) {
            emit linkDown(self);
        } else {
            emit linkUp(self);
        }
    }
}

void PortImpl::process_event(of13::PortStats stats)
{
    PortMeasurement<uint64_t> m;

    m.rx_packets() = stats.rx_packets();
    m.tx_packets() = stats.tx_packets();
    m.rx_bytes() = stats.rx_bytes();
    m.tx_bytes() = stats.tx_bytes();
    m.rx_dropped() = stats.rx_dropped();
    m.tx_dropped() = stats.tx_dropped();
    m.rx_errors() = stats.rx_errors();
    m.tx_errors() = stats.tx_errors();

    auto duration = std::chrono::seconds(stats.duration_sec())
                  + std::chrono::nanoseconds(stats.duration_nsec());

    //  if switch doesn't support duration params in port stats replies,
    //  we use epoch duration for computing 'speed'
    if (duration.count() == 0) {
        duration = std::chrono::steady_clock::now().time_since_epoch();
    }

    stats_->append(duration, std::move(m));
}

void PortImpl::process_event(std::vector<of13::QueueStats> stats)
{
    auto store = queue_stats_.synchronize();

    std::unordered_set<uint32_t> ids;

    for (auto& s : stats) {
        auto id = s.queue_id();
        QueueMeasurement<uint64_t> m;

        m.tx_bytes() = s.tx_bytes();
        m.tx_packets() = s.tx_packets();
        m.tx_errors() = s.tx_errors();

        using std::chrono::seconds;
        using std::chrono::nanoseconds;

        (*store)[id].append( seconds(s.duration_sec())
                           + nanoseconds(s.duration_nsec())
                           , std::move(m) );

        ids.insert(id);
    }

    // Remove unused queues
    auto next = store->end();
    for (auto it = store->begin(); it != store->end(); it = next) {
        next = it; ++next;
        if (ids.count(it->first) == 0)
            store->erase(it);
    }
}

void PortImpl::process_event(std::vector<of13::FlowStats> traffic_stats)
{
    auto store = traffic_stats_.synchronize();
    CHECK(traffic_stats.size() == 3);

    auto guess_type = [](of13::FlowStats& fs) -> ForwardingType {
        auto match = fs.match();

        of13::EthDst* eth_dst = match.eth_dst();
        if (eth_dst == nullptr) {
            return ForwardingType::Unicast;
        }

        auto eth_dst_value = eth_dst->value().to_string();
        if (eth_dst_value == "ff:ff:ff:ff:ff:ff") {
            return ForwardingType::Broadcast;
        }

        auto masked_value = eth_dst_value.substr(0, 2);
        if (masked_value == "01") {
            return ForwardingType::Multicast;
        }

        CHECK(false);
        return ForwardingType::None;
    };

    for (auto& flow : traffic_stats) {
        TrafficMeasurement<uint64_t> m;

        m.rx_packets() = flow.packet_count();
        m.rx_bytes() = flow.byte_count();

        auto duration = std::chrono::seconds(flow.duration_sec()) +
                        std::chrono::nanoseconds(flow.duration_nsec());

        //  if switch doesn't support duration params in port stats replies,
        //  we use epoch duration for computing 'speed'
        if (duration.count() == 0) {
            duration = std::chrono::steady_clock::now().time_since_epoch();
        }

        ForwardingType traffic_type = guess_type(flow);
        (*store)[traffic_type._to_string()].append(duration, std::move(m));
    }
}

bool PortImpl::disabled() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return config_ & of13::OFPPC_PORT_DOWN;
}

bool PortImpl::no_forward() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return config_ & of13::OFPPC_NO_FWD;
}

bool PortImpl::no_receive() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return config_ & of13::OFPPC_NO_RECV;
}

bool PortImpl::no_packet_in() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return config_ & of13::OFPPC_NO_PACKET_IN;
}

auto PortImpl::modify() -> PortModPtr
{
    return std::make_unique<PortModImpl>(shared_from_this());
}

auto PortImpl::disable(bool b) -> PortModPtr
{
    auto ret = modify();
    ret->disable(b);
    return ret;
}

auto PortImpl::no_forward(bool b) -> PortModPtr
{
    auto ret = modify();
    ret->no_forward(b);
    return ret;
}

auto PortImpl::no_receive(bool b) -> PortModPtr
{
    auto ret = modify();
    ret->no_receive(b);
    return ret;
}

auto PortImpl::no_packet_in(bool b) -> PortModPtr
{
    auto ret = modify();
    ret->no_packet_in(b);
    return ret;
}

PortModImpl::PortModImpl(PortImplPtr port)
    : port(port)
{
    pm.xid(xid);
    pm.port_no(port->number());
    pm.hw_addr(boost::lexical_cast<std::string>(port->hw_addr()));
    pm.advertise(0);
    pm.mask(0);
}

PortModImpl::~PortModImpl()
{
    if (pm.mask() == 0)
        return;

    try {
        port->switch_()->connection()->send(pm);
        port->set_config((port->config_ & ~pm.mask()) | pm.config());
    } catch(...) { }
}

auto PortModImpl::disable(bool b) -> PortModPtr
{
    pm.mask(pm.mask() | of13::OFPPC_PORT_DOWN);
    pm.config(b ? (pm.config() | of13::OFPPC_PORT_DOWN)
                : (pm.config() & ~of13::OFPPC_PORT_DOWN));
    return this;
}

auto PortModImpl::no_forward(bool b) -> PortModPtr
{
    pm.mask(pm.mask() | of13::OFPPC_NO_FWD);
    pm.config(b ? (pm.config() | of13::OFPPC_NO_FWD)
                : (pm.config() & ~of13::OFPPC_NO_FWD));
    return this;
}

auto PortModImpl::no_receive(bool b) -> PortModPtr
{
    pm.mask(pm.mask() | of13::OFPPC_NO_RECV);
    pm.config(b ? (pm.config() | of13::OFPPC_NO_RECV)
                : (pm.config() & ~of13::OFPPC_NO_RECV));
    return this;
}

auto PortModImpl::no_packet_in(bool b) -> PortModPtr
{
    pm.mask(pm.mask() | of13::OFPPC_NO_PACKET_IN);
    pm.config(b ? (pm.config() | of13::OFPPC_NO_PACKET_IN)
                : (pm.config() & ~of13::OFPPC_NO_PACKET_IN));
    return this;
}

} // namespace runos
