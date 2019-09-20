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

#include "Application.hpp"
#include "SwitchOrdering.hpp"
#include "ILinkDiscovery.hpp"
#include "lib/kwargs.hpp"
#include "lib/better_enum.hpp"
#include "lib/qt_executor.hpp"

#include <vector>

#include <QTimer>

namespace runos {

using data_link_route = std::vector<switch_and_port>;
using switch_list = std::vector<uint64_t>;

static constexpr uint8_t max_path_id = 255;

struct link_property {
    switch_and_port source;
    switch_and_port target;
    uint64_t hop_metrics;
    uint64_t ps_metrics;
    uint64_t pl_metrics;
};

inline bool operator<(link_property a, link_property b)
{ return std::tie(a.source, a.target) < std::tie(b.source, b.target); }

BETTER_ENUM(ServiceFlag, uint16_t, InBand,
                                   MCast,
                                   BBD,
                                   None);

BETTER_ENUM(MetricsFlag, uint16_t, Hop,
                                   PortSpeed,
                                   PortLoading,
                                   Manual,
                                   None);

BETTER_ENUM(TriggerFlag, uint16_t, Broken      = 1 << 0,
                                   Maintenance = 1 << 1,
                                   Drop        = 1 << 2,
                                   Util        = 1 << 3,
                                   None        = 1 << 4);

namespace route_selector {
    constexpr kwarg<struct app_tag, ServiceFlag> app;
    constexpr kwarg<struct metrics_tag, MetricsFlag> metrics;
    constexpr kwarg<struct flap_tag, uint16_t> flapping;
    constexpr kwarg<struct broken_tag, bool> broken_trigger;
    constexpr kwarg<struct drops_tag, uint8_t> drop_trigger;
    constexpr kwarg<struct util_tag, uint8_t> util_trigger;
    constexpr kwarg<struct conf_tag, uint8_t> configured_count;

    constexpr kwarg<struct include_tag, switch_list> include_dpid;
    constexpr kwarg<struct exclude_tag, switch_list> exclude_dpid;
    constexpr kwarg<struct exact_tag, switch_list> exact_dpid;
};

using RouteSelector = decltype(kwargs(
    route_selector::app,
    route_selector::metrics,
    route_selector::flapping,
    route_selector::broken_trigger,
    route_selector::drop_trigger,
    route_selector::util_trigger,
    route_selector::configured_count,

    route_selector::include_dpid,
    route_selector::exclude_dpid,
    route_selector::exact_dpid
));

class Topology : public Application
               , public SwitchEventHandler
{
    Q_OBJECT
    SIMPLE_APPLICATION(Topology, "topology")
public:
    friend struct TopologyImpl;

    Topology();
    ~Topology();
    void init(Loader* provider, const Config& config) override;
    std::vector<link_property> dumpWeights();
    std::vector<uint32_t> getRoutes() const;
    std::vector<uint32_t> getRoutes(ServiceFlag sf) const;

    switch_and_port other(switch_and_port sp);

    template<class ...Args>
    uint32_t newRoute(uint64_t from, uint64_t to, Args&& ...args)
    {
        return newRoute( from, to, { std::forward<Args>(args)... } );
    }
    template<class ...Args>
    uint8_t newPath(uint32_t route_id, Args&& ...args)
    {
        return newPath( route_id, { std::forward<Args>(args)... } );
    }


    // Modifiers
    uint32_t newRoute(uint64_t from, uint64_t to, RouteSelector selector);
    uint8_t newPath(uint32_t route_id, RouteSelector selector);
    void deleteRoute(uint32_t id);
    bool deletePath(uint32_t route_id, uint8_t path_id);
    void setUsedPath(uint32_t id, uint8_t path_id);
    bool movePath(uint32_t id, uint8_t path_id, uint8_t new_id);
    bool modifyPath(uint32_t id, uint8_t path_id, RouteSelector selector);

    bool addDynamic(uint64_t route_id, RouteSelector selector);
    bool delDynamic(uint64_t route_id);
    uint8_t getDynamic(uint64_t route_id);

    // Observers
    data_link_route predictPath(uint32_t route_id) const;
    data_link_route getPath(uint32_t route_id, uint8_t path_id) const;
    data_link_route getFirstWorkPath(uint32_t route_id) const;
    uint8_t getFirstWorkPathId(uint32_t route_id) const;
    uint8_t getUsedPath(uint32_t id) const;

    // Aux observers
    ServiceFlag getOwner(uint32_t id) const;
    uint8_t getFlapping(uint32_t id, uint8_t path_id) const;
    uint8_t getTriggerThreshold(uint32_t id, uint8_t path_id, TriggerFlag tf) const;
    bool getStatus(uint32_t id, uint8_t path_id, TriggerFlag tf) const;
    uint64_t getMinRouteRate(uint32_t id, uint8_t path_id) const;
    std::pair<uint64_t, uint64_t> getPortUtility(switch_and_port sp) const;
    uint64_t getSpeedRate(switch_and_port sp) const;
    std::string getRouteDump(uint32_t id) const;

    uint8_t minHops(uint64_t from, uint64_t to);

protected slots:
    void linkDiscovered(switch_and_port from, switch_and_port to);
    void linkBroken(switch_and_port from, switch_and_port to);
    void reloadStats();
    void onRecovery();
    void onPrimary();
    void onPMaintenance(PortPtr port);
    void onPMaintenanceOff(PortPtr port);
    void onSMaintenance(SwitchPtr port);
    void onSMaintenanceOff(SwitchPtr port);

private:
    struct TopologyImpl* m;
    QTimer* stats_timer;
    ILinkDiscovery* ld_app;
    class SwitchManager* m_switch_manager;
    class RecoveryManager* recovery;
    class DatabaseConnector* db_connector_ = nullptr;
    qt_executor executor {this};

    void updateMetrics();
    void timerEvent(QTimerEvent *event) override;
    void addLink(switch_and_port from, switch_and_port to);

    void switchUp(SwitchPtr sw) override;
    void switchDown(SwitchPtr sw) override;

    void update_database(uint32_t route_id);
    void erase_from_database(uint32_t route_id);
    void load_from_database();
    void clear_database();
signals:
    void ready();

    void routeTriggerActive(uint32_t id, uint8_t path_id, TriggerFlag tf);
    void routeTriggerInactive(uint32_t id, uint8_t path_id, TriggerFlag tf);
};

} // namespace runos
