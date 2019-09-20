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

#include "SwitchFwd.hpp"
#include "Statistics.hpp"
#include "../lib/mod_trait.hpp"
#include "../lib/ethaddr.hpp"
#include "../lib/better_enum.hpp"
#include <runos/core/safe_ptr.hpp>

#include <QtCore>

#include <string>
#include <memory>
#include <cstdint>

namespace runos {

BETTER_ENUM(ForwardingType, short, None,
                                   Unicast,
                                   Broadcast,
                                   Multicast);

class Port;
class EthernetPort;
using PortPtr = safe::shared_ptr<Port>;
using UnsafePortPtr = std::shared_ptr<Port>;
using EthernetPortPtr = safe::shared_ptr<EthernetPort>;
using UnsafeEthernetPortPtr = std::shared_ptr<EthernetPort>;

template<class Trait>
struct PortMod {
    using PortModPtr
        = typename Trait::template return_type<runos::PortMod>;

    virtual PortModPtr disable(bool) = 0;
    virtual PortModPtr no_receive(bool) = 0;
    virtual PortModPtr no_forward(bool) = 0;
    virtual PortModPtr no_packet_in(bool) = 0;

    virtual ~PortMod() = default;
};

class Port : public QObject
           , public virtual PortMod<mod_create_trait>
{
    Q_OBJECT
public:
    using super = PortMod<mod_create_trait>;

    explicit Port(QObject* parent = 0)
        : QObject(parent)
    { }

    virtual SwitchPtr switch_() const = 0;
    virtual Statistics<PortMeasurement> stats() const = 0;
    
    // Queues
    virtual Statistics<QueueMeasurement> queue_stats(uint32_t qid) const = 0;
    virtual std::vector<uint32_t> queues() const = 0;

    // Traffic stats
    virtual Statistics<TrafficMeasurement> traffic_stats(ForwardingType type) const = 0;

    // Description
    virtual unsigned number() const = 0;
    virtual const ethaddr& hw_addr() const = 0;
    virtual const std::string& name() const = 0;

    // == Configuration ==
    /* TODO: Set from requirements [prio: medium] */
    /* TODO: Make observable [prio: low] */
    virtual PortModPtr modify() = 0;
    virtual bool disabled() const = 0; // getter
    using super::disable; // setter
    virtual bool no_forward() const = 0; // getter
    using super::no_forward; // setter
    virtual bool no_receive() const = 0;
    using super::no_receive; // setter
    virtual bool no_packet_in() const = 0; // getter
    using super::no_packet_in; // setter

    // == State ==
    /* TODO: Make observable [prio: high] */
    virtual bool link_down() const = 0;
    virtual bool blocked() const = 0;
    virtual bool live() const = 0;
    virtual bool maintenance() const = 0; // getter
    virtual void set_maintenance(bool b) = 0; // setter

signals:
    void configChanged(PortPtr);
    void stateChanged(PortPtr);
    void linkUp(PortPtr);
    void linkDown(PortPtr);
    void maintenanceStart(PortPtr);
    void maintenanceEnd(PortPtr);
};

template<class Trait>
struct EthernetPortMod : virtual PortMod<Trait>
{
    using EthernetPortModPtr
        = typename Trait::template return_type<runos::EthernetPortMod>;

    virtual EthernetPortModPtr advertise(uint32_t features) = 0;
};

class EthernetPort : public Port
                   , public EthernetPortMod<mod_create_trait>
{
    Q_OBJECT
public:
    virtual uint32_t advertised() const = 0;
    virtual uint32_t supported() const = 0;
    virtual uint32_t peer() const = 0;
    virtual uint32_t current() const = 0;

    virtual unsigned current_speed() const = 0;
    virtual unsigned max_speed() const = 0;
};

} // namespace runos
