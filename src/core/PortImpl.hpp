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
#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <chrono>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/synchronized_value.hpp>
#include <fluid/of13msg.hh>

#include "api/Port.hpp"
#include "StatisticsStore.hpp"

namespace runos {

namespace of13 = fluid_msg::of13;

class SwitchImpl;
using SwitchImplPtr = std::shared_ptr<SwitchImpl>;

class PortImpl;
using PortImplPtr = std::shared_ptr<PortImpl>;

class PortImpl : public EthernetPort
               , public std::enable_shared_from_this<PortImpl>
{
    Q_OBJECT
public:
    explicit PortImpl(SwitchImplPtr, of13::Port&, QObject* parent = 0);
    void start();

    // Associations
    SwitchPtr switch_() const override { return sw_.lock(); }

    // Stats
    Statistics<PortMeasurement> stats() const override
    { return stats_->get(); }

    Statistics<QueueMeasurement> queue_stats(uint32_t qid) const override
    { return queue_stats_->at(qid).get(); }

    Statistics<TrafficMeasurement> traffic_stats(ForwardingType type) const override
    { return traffic_stats_->at(type._to_string()).get(); }

    std::vector<uint32_t> queues() const override;
    
    // Description
    unsigned number() const override { return number_; }
    const ethaddr& hw_addr() const override { return hw_addr_; }
    const std::string& name() const override { return name_; }

    // == Configuration ==
    /* TODO: Set from requirements [prio: medium] */
    /* TODO: Make observable [prio: low] */
    PortModPtr modify() override;
    bool disabled() const override;
    PortModPtr disable(bool) override;
    bool no_forward() const override;
    PortModPtr no_forward(bool) override;
    bool no_receive() const override;
    PortModPtr no_receive(bool) override;
    bool no_packet_in() const override;
    PortModPtr no_packet_in(bool) override;

    // == State ==
    /* TODO: Make observable [prio: high] */
    bool link_down() const override { return state_ & of13::OFPPS_LINK_DOWN; }
    bool blocked() const override { return state_ & of13::OFPPS_BLOCKED; }
    bool live() const override { return state_ & of13::OFPPS_LIVE; }
    bool maintenance() const override { return maintenance_; }
    void set_maintenance(bool b) override {
        if (maintenance_ != b) {
            maintenance_ = b; 
            maintenance_ ? emit maintenanceStart(shared_from_this())
                         : emit maintenanceEnd(shared_from_this());
        }
    }

    // == Ethernet ==
    uint32_t advertised() const override { return advertised_; }
    EthernetPortModPtr advertise(uint32_t features) override
    {
        return nullptr; // unimplemented
    }
    uint32_t supported() const override { return supported_; }
    uint32_t peer() const override { return peer_; }
    uint32_t current() const override { return current_; }
    unsigned current_speed() const override { return current_speed_; }
    unsigned max_speed() const override { return max_speed_; }

    void set_config(uint32_t config);
    void set_offline();

    void process_event(of13::Port& port);
    void process_event(of13::PortStats port_stats);
    void process_event(std::vector<of13::QueueStats> queue_stats);
    void process_event(std::vector<of13::FlowStats> traffic_stats);

protected:
    friend class PortModImpl;
    std::weak_ptr<Switch> sw_;

    unsigned number_;
    ethaddr hw_addr_;
    std::string name_;

    mutable boost::shared_mutex cmutex;
    uint32_t config_;
    uint32_t state_;

    uint32_t advertised_;
    uint32_t supported_;
    uint32_t peer_;
    uint32_t current_;
    unsigned current_speed_;
    unsigned max_speed_;

    bool maintenance_;

    boost::synchronized_value<
        StatisticsStore<PortMeasurement>
    > stats_;

    boost::synchronized_value<
        std::unordered_map<uint32_t, StatisticsStore<QueueMeasurement>>
    > queue_stats_;

    boost::synchronized_value<
        std::unordered_map<std::string, StatisticsStore<TrafficMeasurement>>
    > traffic_stats_;
};

class PortModImpl : public PortMod<mod_reuse_trait> {
public:
    explicit PortModImpl(PortImplPtr port);

    PortModPtr disable(bool) override;
    PortModPtr no_forward(bool) override;
    PortModPtr no_receive(bool) override;
    PortModPtr no_packet_in(bool) override;

    ~PortModImpl();
private:
    PortImplPtr port;
    of13::PortMod pm;
};

} // namespace runos
