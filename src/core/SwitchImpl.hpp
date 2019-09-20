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

#include <runos/core/future-decl.hpp>
#include "lib/qt_executor.hpp"
#include "api/Switch.hpp"
#include "PortImpl.hpp"

#include <runos/DeviceDb.hpp>

#include <fluid/of13msg.hh>
#include <boost/thread/shared_mutex.hpp>

#include <memory>
#include <map>

namespace runos {

namespace of13 = fluid_msg::of13;

class SwitchImpl;
using SwitchImplPtr = std::shared_ptr<SwitchImpl>;

class SwitchImpl : public Switch
                 , public std::enable_shared_from_this<SwitchImpl>
{
    Q_OBJECT
public:
    explicit SwitchImpl(of13::FeaturesReply& fr,
                        Rc<DeviceDb> propdb,
                        OFConnectionPtr conn,
                        QObject* parent = 0);

    OFConnectionPtr connection() const override { return conn_; }

    uint64_t dpid() const override { return dpid_; }
    unsigned nbuffers() const override { return nbuffers_; }
    unsigned ntables() const override { return ntables_; }
    uint32_t capabilites() const override { return capabilites_; }
    std::any const& property(std::string_view name) const override;

    int64_t  property(std::string_view name, int64_t def_val) const override;
    uint64_t property(std::string_view name, uint64_t def_val) const override;
    uint32_t property(std::string_view name, uint32_t def_val) const override;
    uint16_t property(std::string_view name, uint16_t def_val) const override;
    uint8_t  property(std::string_view name, uint8_t def_val) const override;
    int      property(std::string_view name, int def_val) const override;
    bool     property(std::string_view name, bool def_val) const override;
    std::string property(std::string_view name, std::string def_val) const override;

    const std::string& manufacturer() const override { return manufacturer_; }
    const std::string& hardware() const override { return hardware_; }
    const std::string& software() const override { return software_; }
    const std::string& serial_number() const override { return serial_number_; }
    const std::string& description() const override { return description_; }
    const std::string& aux_address() const override { return aux_address_; }

    unsigned miss_send_len() const override;
    unsigned fragment_policy() const override;

    SwitchModPtr modify() override;
    SwitchModPtr miss_send_len(unsigned len) override;
    SwitchModPtr fragment_policy(unsigned policy) override;

    bool maintenance() const override { return maintenance_; }
    void set_maintenance(bool b) override {
        if (maintenance_ != b) {
            maintenance_ = b;
            maintenance_ ? emit switchMaintenanceStart(shared_from_this())
                         : emit switchMaintenanceEnd(shared_from_this());
        }
    }

    safe::shared_ptr<Port> port(unsigned port_no) const override;
    safe::shared_ptr<Port> port(ethaddr const& hw_addr) const override;

    std::vector<PortPtr> ports() const override;

    void process_event(of13::PortStatus ps);

    // Warning: requries external locking
    std::map<unsigned, PortImplPtr>::iterator delete_port(unsigned no);
    // Warning: requries external locking
    void add_port(of13::Port port);
    // Warning: requries external locking
    void modify_port(of13::Port port);

    void set_up();
    void set_down();

    void handle(const drivers::Handler& h) const override { m_driver->apply(h); }

protected:
    void process_event(std::vector<of13::Port> vports);
    safe::shared_ptr<PortImpl> port_impl(unsigned port_no) const;
    void update_props();
    void init_tables();
    void init_aux_address();
    void updateStats();
    void loadDriver();
    void timerEvent(QTimerEvent*) override;

private:
    friend class SwitchModImpl;

    OFConnectionPtr conn_;
    Rc<DeviceDb> propdb_;

    bool is_up {false};

    uint64_t dpid_;
    unsigned nbuffers_;
    unsigned ntables_;
    uint32_t capabilites_;
    std::map<std::string_view, std::any> property_;

    mutable boost::shared_mutex cmutex;
    uint16_t miss_send_len_;
    uint16_t flags_;
    bool maintenance_;

    std::string manufacturer_;
    std::string hardware_;
    std::string software_;
    std::string serial_number_;
    std::string description_;
    std::string aux_address_;

    mutable boost::shared_mutex pmutex;
    std::map<unsigned, PortImplPtr> ports_;

    mutable qt_executor executor{this};
    drivers::DefaultDriver* m_driver;
};

} // namespace runos
