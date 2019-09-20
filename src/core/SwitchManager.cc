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

#include "SwitchManager.hpp"

#include "OFServer.hpp"
#include "Controller.hpp"
#include "SwitchImpl.hpp"
#include "DpidChecker.hpp"
#include "StatsRulesManager.hpp"

#include <runos/DeviceDb.hpp>
#include <runos/core/logging.hpp>

#include <fluid/of13msg.hh>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <vector>
#include <map>
#include <algorithm> // copy
#include <iterator> // back_inserter

namespace runos {

namespace of13 = fluid_msg::of13;

REGISTER_APPLICATION(SwitchManager, {"controller", "of-server",
                                     "dpid-checker", "stats-rules-manager", ""})

/////////////////
//// Handler ////
/////////////////

struct SwitchManager::implementation final
    : std::enable_shared_from_this<implementation>
    , OFMessageHandler<of13::FeaturesReply>
    , OFMessageHandler<of13::PortStatus>
{
    SwitchManager& app;
    StatsRulesManager* stats_rules_mgr;
    Controller* controller;
    OFServer* ofserver;
    Rc<DeviceDb> propdb;

    std::map<uint64_t, SwitchImplPtr> switches;
    mutable boost::shared_mutex smutex;

    implementation(SwitchManager& app)
        : app(app)
    {
        propdb = std::make_shared<DeviceDb>();
        propdb->add_default();
    }

    void connect_stats_rules_mgr()
    {
        QObject::connect(&app, &SwitchManager::portAdded,
                         stats_rules_mgr, &StatsRulesManager::installRules);
        QObject::connect(&app, &SwitchManager::portDeleted,
                         stats_rules_mgr, &StatsRulesManager::deleteRules);
        QObject::connect(&app, &SwitchManager::switchMaintenanceEnd,
                         stats_rules_mgr, &StatsRulesManager::clearStatsTable);
        QObject::connect(&app, &SwitchManager::portMaintenanceStart,
                         stats_rules_mgr, &StatsRulesManager::deleteRules);
        QObject::connect(&app, &SwitchManager::portMaintenanceEnd,
                         stats_rules_mgr, &StatsRulesManager::installRules);
    }

    safe::shared_ptr<SwitchImpl> switch_(uint64_t dpid) const
    {
        boost::shared_lock< boost::shared_mutex > lock(smutex);
        auto it = switches.find(dpid);
        return (it != switches.end()) ? it->second : nullptr;
    }

    SwitchImplPtr make_switch(of13::FeaturesReply& fr, OFConnectionPtr conn)
    {
        auto ret = std::make_shared<SwitchImpl>(fr, propdb, conn, &app);

        QObject::connect(ret.get(), &Switch::portAdded,
                         &app, &SwitchManager::portAdded);

        QObject::connect(ret.get(), &Switch::portDeleted,
                         &app, &SwitchManager::portDeleted);

        QObject::connect(ret.get(), &Switch::linkUp,
                         &app, &SwitchManager::linkUp);

        QObject::connect(ret.get(), &Switch::linkDown,
                         &app, &SwitchManager::linkDown);

        QObject::connect(ret.get(), &Switch::switchUp,
                         &app, &SwitchManager::switchUp);

        QObject::connect(ret.get(), &Switch::switchDown,
                         &app, &SwitchManager::switchDown);

        QObject::connect(ret.get(), &Switch::portMaintenanceStart,
                         &app, &SwitchManager::portMaintenanceStart);

        QObject::connect(ret.get(), &Switch::portMaintenanceEnd,
                         &app, &SwitchManager::portMaintenanceEnd);

        QObject::connect(ret.get(), &Switch::switchMaintenanceStart,
                         &app, &SwitchManager::switchMaintenanceStart);

        QObject::connect(ret.get(), &Switch::switchMaintenanceEnd,
                         &app, &SwitchManager::switchMaintenanceEnd);

        stats_rules_mgr->clearStatsTable(ret);

        return ret;
    }

    void remove_switch(uint64_t dpid)
    {
        boost::upgrade_lock< boost::shared_mutex > rslock(smutex);
        auto it = switches.find(dpid);
        if (switches.end() != it) {
            it->second.get()->disconnect();
            boost::upgrade_to_unique_lock< boost::shared_mutex > wslock(rslock);
            switches.erase(it);
        }
    }

    bool process(of13::FeaturesReply& fr, OFConnectionPtr conn) override
    {
        boost::upgrade_lock< boost::shared_mutex > rslock(smutex);
        auto it = switches.find(fr.datapath_id());
        if (it != switches.end()) {
            auto dpid = it->first;
            auto sw = it->second;
            if (sw->nbuffers() != fr.n_buffers() ||
                sw->ntables() != fr.n_tables() ||
                sw->capabilites() != fr.capabilities())
            {
                LOG(ERROR) << "Switch features changed for dpid=" << dpid << ". "
                           << "Assign new dpid or restart controller.";
                conn->close();
            } else {
                sw->set_up();
            }
            rslock.unlock();
        } else {
            boost::upgrade_to_unique_lock< boost::shared_mutex >
                wslock(rslock);
            switches.emplace(fr.datapath_id(), make_switch(fr, conn));
        }
        return true;
    }

    bool process(of13::PortStatus& ps, OFConnectionPtr conn) override
    {
        if (auto sw = switch_(conn->dpid())) {
            sw->process_event(ps);
        } else {
            LOG(WARNING) << "Ignoring port status from " << conn->dpid()
                         << " because it Switch instance isn't yet created";
        }
        return false;
    }
};

SwitchManager::SwitchManager() = default;
SwitchManager::~SwitchManager() = default;

void SwitchManager::init(Loader* loader, const Config&)
{
    qRegisterMetaType<runos::SwitchPtr>("SwitchPtr");
    qRegisterMetaType<fluid_msg::of13::Port>("of13::Port");

    impl.reset(new implementation{ *this });

    impl->controller = Controller::get(loader);
    impl->ofserver = OFServer::get(loader);
    impl->stats_rules_mgr = StatsRulesManager::get(loader);

    impl->connect_stats_rules_mgr();
    impl->controller->register_handler(impl, -50);
    QObject::connect(impl->ofserver, &OFServer::connectionDown,
        [this, dpid_checker = DpidChecker::get(loader)](OFConnectionPtr conn) {
            conn->reset_stats();
            SwitchImplPtr sw = impl->switch_(conn->dpid());
            if (sw) {
                sw->set_down();
            }
            if (not dpid_checker->isRegistered(conn->dpid())) {
                impl->remove_switch(sw->dpid());
            }
        }
    );
}

safe::shared_ptr<Switch> SwitchManager::switch_(uint64_t dpid) const
{
    return impl->switch_(dpid);
}

std::vector<SwitchPtr> SwitchManager::switches() const
{
    std::vector<SwitchPtr> ret;
    boost::shared_lock< boost::shared_mutex > lock(impl->smutex);
    boost::copy(impl->switches | boost::adaptors::map_values,
                std::back_inserter(ret));
    return ret;
}

} // namespace runos
