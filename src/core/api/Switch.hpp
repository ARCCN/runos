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

#include "../lib/ethaddr.hpp"
#include "../lib/mod_trait.hpp"

#include "SwitchFwd.hpp"
#include "Port.hpp"
#include "OFConnection.hpp"
#include "OFDriver.hpp"

#include <runos/core/exception.hpp>

#include <QtCore>

#include <any>
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

namespace runos {

struct insufficient_capabilities : exception_root
                                 , runtime_error_tag
{ };

template<class Trait>
struct SwitchMod {
    using SwitchModPtr
        = typename Trait::template return_type<runos::SwitchMod>;

    virtual SwitchModPtr miss_send_len(unsigned) = 0;
    virtual SwitchModPtr fragment_policy(unsigned) = 0;

    virtual ~SwitchMod() = default;
};

class Switch : public QObject
             , public SwitchMod<mod_create_trait>
{
Q_OBJECT
public:
    using super = SwitchMod<mod_create_trait>;

    explicit Switch(QObject* parent = 0)
        : QObject(parent)
    { }

    virtual OFConnectionPtr connection() const = 0;

    // == Features ==
    virtual uint64_t dpid() const = 0;
    virtual unsigned nbuffers() const = 0;
    virtual unsigned ntables() const = 0;
    virtual uint32_t capabilites() const = 0;
    virtual std::any const& property(std::string_view name) const = 0;
    // TODO: using templates
    virtual int64_t  property(std::string_view name, int64_t def_val) const = 0;
    virtual uint64_t property(std::string_view name, uint64_t def_val) const = 0;
    virtual uint32_t property(std::string_view name, uint32_t def_val) const = 0;
    virtual uint16_t property(std::string_view name, uint16_t def_val) const = 0;
    virtual uint8_t  property(std::string_view name, uint8_t def_val) const = 0;
    virtual int      property(std::string_view name, int def_val) const = 0;
    virtual bool     property(std::string_view name, bool def_val) const = 0;
    virtual std::string property(std::string_view name, std::string def_val) const = 0;

    // == Description ==
    virtual const std::string& manufacturer() const = 0;
    virtual const std::string& hardware() const = 0;
    virtual const std::string& software() const = 0;
    virtual const std::string& serial_number() const = 0;
    virtual const std::string& description() const = 0;
    virtual const std::string& aux_address() const = 0;

    // == Configuration ==
    // TODO: Set from requirements [prio: low]

    virtual SwitchModPtr modify() = 0;
    virtual unsigned miss_send_len() const = 0;
    using super::miss_send_len;
    virtual unsigned fragment_policy() const = 0;
    using super::fragment_policy;
    virtual bool maintenance() const = 0;
    virtual void set_maintenance(bool b) = 0;

    // == Ports ==
    virtual safe::shared_ptr<Port> port(unsigned port_no) const = 0;
    virtual safe::shared_ptr<Port> port(const ethaddr& hw_addr) const = 0;
    // TODO: Make observable [prio: high]
    virtual std::vector<PortPtr> ports() const = 0;

    // == Tables ==
    struct Tables {
        static const uint8_t no_table {255};
        uint8_t statistics    {no_table};
        uint8_t ep_statistics {no_table};

        uint8_t admission  {0};
        uint8_t mirroring  {1};
        uint8_t classifier {2};
        uint8_t forwarding {3};
        uint8_t learning   {4};
        uint8_t output     {5};
    } tables;

    // == Specific handling for different OF pipelines (drivers) ==
    virtual void handle(const drivers::Handler& h) const = 0;

signals:
    void portAdded(PortPtr);
    void portDeleted(PortPtr);
    void linkUp(PortPtr);
    void linkDown(PortPtr);
    void switchUp(SwitchPtr);
    void switchDown(SwitchPtr);
    void configUpdated(SwitchPtr);

    void portMaintenanceStart(PortPtr);
    void portMaintenanceEnd(PortPtr);
    void switchMaintenanceStart(SwitchPtr);
    void switchMaintenanceEnd(SwitchPtr);
};

} // namespace runos
