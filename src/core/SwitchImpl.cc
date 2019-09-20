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

#include "SwitchImpl.hpp"

#include "PortImpl.hpp"
#include "api/OFAgent.hpp"

#include <runos/core/assert.hpp>
#include <runos/core/throw.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/catch_all.hpp>

//#include <range/v3/all.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/group_by.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/action/sort.hpp>

#include <boost/lexical_cast.hpp>
#include <iterator> // back_inserter
#include <utility> // move
#include <chrono>
#include <functional>

namespace runos {

SwitchImpl::SwitchImpl(of13::FeaturesReply& fr,
                       Rc<DeviceDb> propdb,
                       OFConnectionPtr conn,
                       QObject* parent)
    : conn_{conn}, propdb_{propdb},
      maintenance_{false}
{
    dpid_ = fr.datapath_id();
    nbuffers_ = fr.n_buffers();
    ntables_ = fr.n_tables();
    capabilites_ = fr.capabilities();

    moveToThread(parent->thread());
    setParent(parent);

    async(executor, [this]() {
        constexpr static auto stats_update_interval
            = std::chrono::seconds(2);
        startTimer(std::chrono::milliseconds(stats_update_interval).count());
    });

    set_up();
}

std::any const& SwitchImpl::property(std::string_view name) const {
    static std::any none;
    auto it = property_.find(name);
    return it != property_.end() ? it->second : none;
}

int64_t SwitchImpl::property(std::string_view name, int64_t def_val) const {
    auto it = property_.find(name);
    return it != property_.end() ? std::any_cast<int64_t>(it->second) : def_val;
}

uint64_t SwitchImpl::property(std::string_view name, uint64_t def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
               ? static_cast<uint64_t>(std::any_cast<int64_t>(it->second))
               : def_val;
}

uint32_t SwitchImpl::property(std::string_view name, uint32_t def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
               ? static_cast<uint32_t>(std::any_cast<int64_t>(it->second))
               : def_val;
}

uint16_t SwitchImpl::property(std::string_view name, uint16_t def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
               ? static_cast<uint16_t>(std::any_cast<int64_t>(it->second))
               : def_val;
}

uint8_t SwitchImpl::property(std::string_view name, uint8_t def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
               ? static_cast<uint8_t>(std::any_cast<int64_t>(it->second))
               : def_val;
}

int SwitchImpl::property(std::string_view name, int def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
               ? static_cast<int>(std::any_cast<int64_t>(it->second))
               : def_val;
}

bool SwitchImpl::property(std::string_view name, bool def_val) const {
    auto it = property_.find(name);
    return it != property_.end() ? std::any_cast<bool>(it->second) : def_val;
}

std::string SwitchImpl::property(std::string_view name, std::string def_val) const {
    auto it = property_.find(name);
    return it != property_.end()
        ? boost::lexical_cast<std::string>(std::any_cast<std::string_view>(it->second))
        : def_val;
}

unsigned SwitchImpl::miss_send_len() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return miss_send_len_;
}

unsigned SwitchImpl::fragment_policy() const
{
    boost::shared_lock< boost::shared_mutex > lock(cmutex);
    return flags_ & of13::OFPC_FRAG_MASK;
}

auto SwitchImpl::miss_send_len(unsigned len) -> SwitchModPtr
{
    auto ret = modify();
    ret->miss_send_len(len);
    return ret;
}

auto SwitchImpl::fragment_policy(unsigned policy) -> SwitchModPtr
{
    auto ret = modify();
    ret->fragment_policy(policy);
    return ret;
}

safe::shared_ptr<Port> SwitchImpl::port(unsigned port_no) const
{
    return port_impl(port_no);
}

safe::shared_ptr<PortImpl> SwitchImpl::port_impl(unsigned port_no) const
{
    boost::shared_lock< boost::shared_mutex > rlock(pmutex);
    auto port_data = ports_.find(port_no);
    return (port_data != ports_.end()) ? port_data->second : nullptr;
}

safe::shared_ptr<Port> SwitchImpl::port(const ethaddr& hw_addr) const
{
    boost::shared_lock< boost::shared_mutex > rlock(pmutex);
    using value_type = std::pair<unsigned, PortImplPtr>;
    auto hw_addr_equal = [&](const value_type& p)
                         { return hw_addr == p.second->hw_addr(); };
    auto it = ranges::find_if( ports_, hw_addr_equal );

    return (it != ports_.end()) ? it->second : nullptr;
}

std::vector<PortPtr> SwitchImpl::ports() const
{
    boost::shared_lock< boost::shared_mutex > rlock(pmutex);
    return ports_ | ranges::view::values;
}

// Warning: requries external locking
std::map<unsigned, PortImplPtr>::iterator SwitchImpl::delete_port(unsigned no)
{
    auto it = ports_.find(no);
    auto port = it->second;
    CHECK(it != ports_.end());
    port->set_offline();
    auto ret = ports_.erase(it);
    emit portDeleted(port);
    return ret;
}

// Warning: requries external locking
void SwitchImpl::add_port(of13::Port ofport)
{
    auto self = shared_from_this();
    auto port = std::make_shared<PortImpl>(self, ofport, this);
    auto ret = ports_.emplace(ofport.port_no(), port);
    CHECK(ret.second); // inserted
    emit portAdded(port);
    port->start();
}

// Warning: requries external locking
void SwitchImpl::modify_port(of13::Port port)
{
    ports_.at(port.port_no())->process_event(port);
}

void SwitchImpl::process_event(std::vector<of13::Port> vports)
{
    using namespace ranges;

    // Prepare
    auto get_port_no = std::mem_fn<uint32_t()>(&of13::Port::port_no);
    std::map<unsigned, of13::Port> ports
        = view::zip(vports | view::transform(get_port_no), vports | view::all);

    boost::unique_lock< boost::shared_mutex > lock(cmutex);

    // 1. Find old port that is not listed, delete them
    for (auto it = ports_.begin(); it != ports_.end(); ) {
        auto port_no = it->first;
        if (ports.count(port_no) == 0) {
            it = delete_port(port_no);
            continue;
        } else {
            ethaddr old_hw = ports_.at(port_no)->hw_addr();
            ethaddr new_hw = ports.at(port_no).hw_addr().to_string();
            if (old_hw != new_hw) {
                it = delete_port(port_no);
                continue;
            }
        }
        ++it;
    }

    // 2. Find new ports and add them
    // 3. Find existing ports and modify their properties
    RANGES_FOR(auto& port, ports | view::values) {
        if (ports_.count(port.port_no()) > 0) {
            modify_port(port);
        } else {
            add_port(port);
        }
    }
}

void SwitchImpl::process_event(of13::PortStatus ps)
{
    boost::unique_lock< boost::shared_mutex > wlock(pmutex);

    of13::Port port = ps.desc();
    uint32_t port_no = port.port_no();

    switch (ps.reason()) {
        case of13::OFPPR_ADD: add_port(port); break;
        case of13::OFPPR_DELETE: delete_port(port_no); break;
        case of13::OFPPR_MODIFY: modify_port(port); break;
    }
}

void SwitchImpl::update_props()
{
    auto props = propdb_->query()
                         .dpid(dpid_)
                         .manufacturer(manufacturer_)
                         .hwVersion(hardware_)
                         .swVersion(software_)
                         .serialNum(serial_number_)
                         .description(description_)
                         .execute();

    property_.clear();
    for (auto& prop : props) {
        property_.insert(std::move(prop));
    }
    
}

void SwitchImpl::init_tables()
{
    tables.statistics = property("tables.statistics", tables.statistics);
    tables.ep_statistics = property("tables.ep_statistics", tables.ep_statistics);
    tables.admission = property("tables.admission", tables.admission);
    tables.mirroring = property("tables.mirroring", tables.mirroring);
    tables.classifier = property("tables.classifier", tables.classifier);
    tables.forwarding = property("tables.forwarding", tables.forwarding);
    tables.learning = property("tables.learning", tables.learning);
    tables.output = property("tables.output", tables.output);
}

void SwitchImpl::init_aux_address()
{
    std::string peer = conn_->peer_address();
    std::string ip = peer.substr(0, peer.find(":"));
    auto it = property_.find("aux_address");
    aux_address_ = it != property_.end()
                    ? boost::lexical_cast<std::string>(std::any_cast<std::string_view>(it->second))
                    : ip;
}

void SwitchImpl::set_up()
{
    auto agent = connection()->agent();

    try {
        auto f1 = agent->request_config().then(executor,
            [this](future<ofp::switch_config> fconfig) {
                auto config = fconfig.get();
                miss_send_len_ = config.miss_send_len;
                flags_ = config.flags;
            });

        auto f2 = agent->request_switch_desc().then(executor,
            [this](future<fluid_msg::SwitchDesc> fdesc) {
                auto desc = fdesc.get();
                manufacturer_ = desc.mfr_desc();
                hardware_ = desc.hw_desc();
                software_ = desc.sw_desc();
                serial_number_ = desc.serial_num();
                description_ = desc.dp_desc();
                update_props();
                init_tables();
                init_aux_address();
                loadDriver();
            });

        auto f3 = agent->request_port_desc().then(executor,
            [this](future<OFAgent::sequence<of13::Port>> fpdesc) {
                auto pdesc = fpdesc.get();
                process_event(std::move(pdesc));
            });

        when_all(std::move(f1), std::move(f2), std::move(f3)).then(executor,
            [this](auto f) {
                auto t = f.get();
                std::get<0>(t).get();
                std::get<1>(t).get();
                std::get<2>(t).get();
                is_up = true;
                emit this->switchUp(this->shared_from_this());
            });
    } catch (const OFAgent::request_error& e) {
        LOG(ERROR) << "[SwitchImpl] - " << e.what();
    }
}

void SwitchImpl::set_down()
{
    is_up = false;

    boost::shared_lock< boost::shared_mutex > rlock(pmutex);
    for (auto p : ports_) {
        p.second->set_offline();
    }
    emit switchDown(shared_from_this());
}

class SwitchModImpl : public SwitchMod<mod_reuse_trait> {
public:
    explicit SwitchModImpl(SwitchImplPtr sw);

    SwitchModPtr miss_send_len(unsigned len) override;
    SwitchModPtr fragment_policy(unsigned policy) override;

    ~SwitchModImpl();
private:
    SwitchImplPtr sw;
    ofp::switch_config scmsg;
};

SwitchModImpl::SwitchModImpl(SwitchImplPtr sw)
    : sw(sw)
{
    scmsg.miss_send_len = sw->miss_send_len();
    scmsg.flags = sw->fragment_policy();
}

auto SwitchModImpl::miss_send_len(unsigned len) -> SwitchModPtr 
{
    THROW_IF(len > 0xffff, invalid_argument(), "miss_send_len is too big");
    scmsg.miss_send_len = len;
    return this;
} 

auto SwitchModImpl::fragment_policy(unsigned policy) -> SwitchModPtr 
{
    THROW_IF(policy & ~of13::OFPC_FRAG_MASK, invalid_argument(), "Bad fragment policy");
    if (not (sw->capabilites() & of13::OFPC_IP_REASM) &&
            (policy & of13::OFPC_FRAG_REASM))
    {
        THROW(insufficient_capabilities(), "Switch doesn't support IP reassembly");
    }
    if (policy & of13::OFPC_FRAG_MASK) {
        scmsg.flags |= of13::OFPC_FRAG_MASK;
    } else {
        scmsg.flags &= ~of13::OFPC_FRAG_MASK;
    }
    return this;
}

SwitchModImpl::~SwitchModImpl()
{
    boost::upgrade_lock< boost::shared_mutex > lock(sw->cmutex);

    if (sw->miss_send_len() == scmsg.miss_send_len &&
        sw->fragment_policy() == (scmsg.flags & of13::OFPC_FRAG_MASK))
    {
        return;
    }

    try {
        sw->connection()->agent()->set_config(scmsg);
        boost::upgrade_to_unique_lock< boost::shared_mutex > wlock(lock);
        sw->miss_send_len_ = scmsg.miss_send_len;
        sw->flags_ = scmsg.flags;
        emit sw->configUpdated(sw);
    } catch(...) { }
}

auto SwitchImpl::modify() -> SwitchModPtr
{
    return std::make_unique<SwitchModImpl>(shared_from_this());
}

void SwitchImpl::timerEvent(QTimerEvent*)
{
    updateStats();
}

void SwitchImpl::updateStats()
{
    VLOG(10) << "updateStats()";
    auto self = shared_from_this();

    if (not connection()->alive()) {
        VLOG(3) << "Request to offline switch";
        return;
    }

    try {
        auto agent = connection()->agent();

        agent->request_port_stats().then(executor,
            [self](future<OFAgent::sequence<of13::PortStats>> stats) {
                VLOG(10) << "Entering port stats continuation";

                for (auto& ps : stats.get()) try {
                    if (self->property("local_port", of13::OFPP_LOCAL) != ps.port_no())
                        self->port_impl(ps.port_no())->process_event(ps);
                } catch (bad_pointer_access& ex) {
                    LOG(WARNING) << "Can't find port " << ps.port_no();
                }
            });

        agent->request_queue_stats().then(executor,
            [self](future<OFAgent::sequence<of13::QueueStats>> stats) {
                VLOG(10) << "Entering queue stats continuation";

                using Stats = of13::QueueStats;
                using namespace ranges;

                auto port_no_less
                    = [](Stats a, Stats b) { return a.port_no() < b.port_no(); };
                auto port_no_equal
                    = [](Stats a, Stats b) { return a.port_no() == b.port_no(); };

                auto sorted_stats
                    = stats.get() | action::sort(port_no_less);
                auto grouped_stats
                    = sorted_stats | view::group_by(port_no_equal);

                RANGES_FOR(auto stats, grouped_stats) {
                    auto port_no = (*begin(stats)).port_no();
                    self->port_impl(port_no)->process_event(stats);
                }
            });

        if (tables.statistics == Tables::no_table) {
            return;
        }

        ofp::flow_stats_request request;
        request.table_id = tables.statistics;
        agent->request_flow_stats(request).then(executor,
            [self](future<OFAgent::sequence<of13::FlowStats>> flow_stats) {
                VLOG(10) << "Entering traffic stats continuation";

                using Stats = of13::FlowStats;
                using namespace ranges;

                auto port_no_less = [](Stats a, Stats b)
                {
                    uint32_t a_in_port = a.match().in_port()->value();
                    uint32_t b_in_port = b.match().in_port()->value();
                    return a_in_port < b_in_port;
                };
                auto port_no_equal = [](Stats a, Stats b)
                {
                    uint32_t a_in_port = a.match().in_port()->value();
                    uint32_t b_in_port = b.match().in_port()->value();
                    return a_in_port == b_in_port;
                };

                auto sorted_stats
                    = flow_stats.get() | action::sort(port_no_less);
                auto grouped_stats
                    = sorted_stats | view::group_by(port_no_equal);

                RANGES_FOR(auto stats, grouped_stats) {
                    auto match = (*begin(stats)).match();
                    auto port_no = match.in_port()->value();
                    self->port_impl(port_no)->process_event(stats);
                }
            });
    } catch (OFAgent::request_error const& ex) {
        LOG(WARNING) << "Can't update stats:";
        diagnostic_information::get().log();
    }
}

void SwitchImpl::loadDriver()
{
    using namespace drivers;
    m_driver = new DefaultDriver();
}

} // namespace runos
