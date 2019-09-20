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

#include "Topology.hpp"

#include "DatabaseConnector.hpp"
#include "SwitchManager.hpp"
#include "Recovery.hpp"
#include "api/Switch.hpp"
#include "api/Port.hpp"
#include <json.hpp>
#include <runos/core/logging.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/property_map/function_property_map.hpp>

#include <algorithm>
#include <climits>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;

namespace runos {

REGISTER_APPLICATION(Topology, {"link-discovery", "switch-manager", 
                                "switch-ordering", "recovery-manager",
                                "database-connector", ""})

using namespace boost;

static constexpr int ready_timeout = 2;
static constexpr uint64_t max_weight  = 10000;

class Route;
class Path;
using RoutePtr = std::shared_ptr<Route>;
using PathPtr = std::shared_ptr<Path>;

struct Path {
    Path() = delete;
    Path(uint8_t id, uint32_t route_id, data_link_route path) :
        id(id), route_id(route_id), m_path(path) {}
    ~Path() {
        for (auto it: flapping_timers) {
            clearFlapping(it.first);
        }
    }

    uint8_t id;
    uint32_t route_id;
    data_link_route m_path;
    uint8_t triggers {0}; // active triggers
    uint8_t broken_links {0}; //counter of broken links
    uint8_t maintn_links {0}; //counter of maintenance links
    std::map<TriggerFlag, QTimer*> flapping_timers;

    // parameters of triggers
    uint16_t flap {0};
    bool broken_flag {false};
    uint8_t drop_threshold {0}; //percents
    uint8_t util_threshold {0}; //percents
    MetricsFlag metrics { MetricsFlag::Hop };

    bool contains(uint64_t dpid) const;
    bool contains(switch_and_port sp) const;
    bool getTrigger(TriggerFlag tf) const;
    void setTrigger(TriggerFlag tf);
    void delTrigger(TriggerFlag tf);
    void resetTriggers();

    // return true if it is necessary to emit signal.
    // we cannot emit in these methods because
    // if there are many links that should be emitted
    // at first we must update trigger-state for all of them
    // and after that we can emit the signal
    bool activateTrigger(TriggerFlag tf);
    bool inactivateTrigger(TriggerFlag tf, Topology* emitter);

    void startFlapping(TriggerFlag tf, Topology* emitter);
    void clearFlapping(TriggerFlag tf);

    json to_json() const {
        json ret;
        ret["flapping"] = flap;
        ret["broken_flag"] = broken_flag;
        ret["drop_threshold"] = drop_threshold;
        ret["util_threshold"] = util_threshold;
        ret["metrics"] = metrics._to_string();

        std::for_each(m_path.begin(), m_path.end(), [&ret](auto sp) {
            ret["m_path"].push_back({sp.dpid, sp.port});
        });
        return ret;
    }
};

struct Route {
    Route() = delete;
    Route(uint32_t id, uint64_t from, uint64_t to):
        id(id), from(from), to(to) {}

    uint32_t id;
    uint64_t from;
    uint64_t to;

    ServiceFlag owner { ServiceFlag::None };
    uint8_t used_path { 0 };
    std::vector<PathPtr> paths;
    bool allowed_dynamic { false };
    mutable RouteSelector dynamic;
    mutable std::mutex mut;

    PathPtr attachPath(data_link_route path) {
        std::lock_guard<std::mutex> lock(mut);
        auto ret = std::make_shared<Path>(paths.size(), id, path);
        paths.push_back(ret);
        return ret;
    }

    void detachPath(uint8_t path_id) {
        std::lock_guard<std::mutex> lock(mut);
        if (paths.size() > path_id) {
            for (size_t i = path_id + 1; i < paths.size(); i++) {
                paths[i]->id--;
            }
            if (used_path > path_id)
                used_path--;
            paths.erase(paths.begin() + path_id);
        }
    }

    PathPtr getPath(uint8_t path_id) const {
        std::lock_guard<std::mutex> lock(mut);
        if (paths.size() > path_id)
            return paths.at(path_id);

        return nullptr;
    }

    PathPtr getFirstWorkPath() const {
        std::lock_guard<std::mutex> lock(mut);
        auto found = std::find_if(paths.begin(), paths.end(), [](auto path) {
            return path->triggers == 0; // not activated triggers
        });

        return (found != paths.end() ? *found : nullptr);
    }

    bool hasPath(const data_link_route& m_path) const {
        std::lock_guard<std::mutex> lock(mut);
        auto eq = std::find_if(paths.begin(), paths.end(),
            [&m_path](auto path) {
                return std::equal(m_path.begin(), m_path.end(), 
                              path->m_path.begin(), path->m_path.end());
            }
        );
        return (eq != paths.end() ? true : false);
    }

    json to_json() const {
        using namespace route_selector;
        json ret = {
            {"id", id},
            {"from", from},
            {"to", to},
            {"dynamic", allowed_dynamic},
            {"used_path", used_path},
            {"owner", owner._to_string()}
        };

        std::for_each(paths.begin(), paths.end(), [&ret](const auto& path) {
            ret["paths"].push_back(path->to_json());
        });

        if (allowed_dynamic) {
            ret["dynamic_settings"] = {
                {"metrics", (*dynamic.get(metrics))._to_string()},
                {"flapping", *dynamic.get(flapping)},
                {"broken_flag", *dynamic.get(broken_trigger)},
                {"drop_threshold", *dynamic.get(drop_trigger)},
                {"util_threshold", *dynamic.get(util_trigger)}
            };
        }

        return ret;
    }
};

bool Path::contains(uint64_t dpid) const
{
    return std::find_if(m_path.begin(), m_path.end(), [dpid](auto sp) {
        return sp.dpid == dpid;
    }) != m_path.end();
}

bool Path::contains(switch_and_port sp) const
{
    return std::find(m_path.begin(), m_path.end(), sp) != m_path.end();
}

bool Path::getTrigger(TriggerFlag tf) const
{
    return triggers & tf;
}

void Path::setTrigger(TriggerFlag tf)
{
    triggers |= tf;
    if (tf == +TriggerFlag::Broken)
        broken_links++;
    else if (tf == +TriggerFlag::Maintenance)
        maintn_links++;
}

void Path::delTrigger(TriggerFlag tf)
{
    if (tf == +TriggerFlag::Broken) {
        broken_links--;
        if (broken_links == 0)
            triggers &= ~tf;
    } else if (tf == +TriggerFlag::Maintenance) {
        maintn_links--;
        if (maintn_links == 0)
            triggers &= ~tf;
    } else
        triggers &= ~tf;
}

void Path::resetTriggers()
{
    triggers = 0;
}

bool Path::activateTrigger(TriggerFlag tf)
{
    if (flapping_timers.count(tf)) { // flap timer is working, stop it
        clearFlapping(tf);
        return false;
    }

    bool need_emit {false};
    if (not getTrigger(tf))
        need_emit = true;

    setTrigger(tf);
    return need_emit;
}

bool Path::inactivateTrigger(TriggerFlag tf, Topology* emitter)
{
    if (flapping_timers.count(tf)) { // flap timer is working
        delTrigger(tf);                // decrease trigger counter
        flapping_timers[tf]->start();  // restart timer
        return false;
    }

    if (flap) {
        startFlapping(tf, emitter);
        return false;
    }

    bool need_emit {false};
    delTrigger(tf);
    if (not getTrigger(tf)) 
        need_emit = true;

    return need_emit;
}

void Path::startFlapping(TriggerFlag tf, Topology* emitter)
{
    if (!flap) return;

    QTimer* flapper = new QTimer();
    flapper->moveToThread(emitter->thread());
    flapping_timers.emplace(tf, flapper);

    QObject::connect(flapper, &QTimer::timeout, [=]() {
        delTrigger(tf);
        if (not getTrigger(tf))
            emit emitter->routeTriggerInactive(route_id, id, tf);
        clearFlapping(tf);
    });
    flapper->start(flap * 1000);
}

void Path::clearFlapping(TriggerFlag tf)
{
    try {
        auto timer = flapping_timers.at(tf);
        timer->stop();
        flapping_timers.erase(tf);
        delete timer;
    } catch (const std::out_of_range& ex) {
        // not found, do nothing...
    }
}

using TopologyGraph = adjacency_list< multisetS, vecS, undirectedS, no_property, link_property>;
using vertex_descriptor = TopologyGraph::vertex_descriptor;
using edge_descriptor = TopologyGraph::edge_descriptor;

struct TopologyImpl {
    TopologyImpl(Topology* app): app(app) {};

    std::mutex graph_mutex;
    Topology* app;

    int timer_id {0};
    uint32_t pending_id {1};

    TopologyGraph graph;
    std::unordered_map<uint64_t, vertex_descriptor>
        vertex_map;
    std::unordered_map<uint32_t, RoutePtr> route_map;

    std::map<switch_and_port, std::pair<uint8_t, uint8_t> > triggers;
    std::map<switch_and_port, uint64_t> speed_rate; // bytes/s

    vertex_descriptor vertex(uint64_t dpid) const {
        auto it = vertex_map.find(dpid);
        return it != vertex_map.end() ? it->second
                   : TopologyGraph::null_vertex();
    }

    vertex_descriptor new_vertex(uint64_t dpid) {
        auto it = vertex_map.find(dpid);
        return it != vertex_map.end() ? it->second
                   : vertex_map[dpid] = add_vertex(graph);
    }

    void delete_vertex(uint64_t dpid) {
        auto it = vertex_map.find(dpid);
        if (it != vertex_map.end()) {
            vertex_map.erase(dpid);
        }
    }

    std::pair<edge_descriptor, bool>
    edge(switch_and_port sp, const TopologyGraph& g) const {
        auto vert = vertex(sp.dpid);
        auto null_edge = *edges(g).first;
        if (vert == TopologyGraph::null_vertex()) {
            return std::make_pair(null_edge, false);
        }

        auto edges = out_edges(vert, g);
        for (auto it = edges.first; it != edges.second; ++it) {
            const auto& link = g[*it];
            if (link.source == sp or link.target == sp) {
                return std::make_pair(*it, true);
            }
        }

        return std::make_pair(null_edge, false);
    }

    RoutePtr addRoute(uint64_t from, uint64_t to, uint32_t id = 0) { //TODO mutex
        auto route = std::make_shared<Route>(id ? id : pending_id++, from, to);
        // needed only for restoring from redis on recovery -- pending = (max from all ids in redis) + 1
        if (id && pending_id <= id) pending_id = id + 1;
        route_map[route->id] = route;
        return route;
    }

    void eraseRoute(uint32_t route_id) { // TODO mutex
        if (route_map.find(route_id) == route_map.end())
            return;

        route_map.erase(route_id);
    }

    data_link_route findPath(RoutePtr route, RouteSelector selector) const;
    data_link_route exactPath(switch_list exact) const;
    data_link_route inExPath(switch_list include, switch_list exclude,
                                 MetricsFlag m, RoutePtr route, TopologyGraph& g) const;
    data_link_route computePath(uint64_t from_dpid, uint64_t to_dpid, 
                                 MetricsFlag mf, const TopologyGraph& g) const;

    void maxWeight(const data_link_route& route, TopologyGraph& g) const;
    std::vector<link_property> get_dump() { // TODO: check
        std::vector<link_property> ret;
        auto e = edges(graph);
        auto e_iter = e.first;
        while (e_iter != e.second) {
            ret.push_back(graph[*e_iter]);
            e_iter++;
        }

        return ret;
    }
};

data_link_route TopologyImpl::computePath(uint64_t from_dpid, uint64_t to_dpid, 
                                 MetricsFlag mf, const TopologyGraph& g) const
{
    data_link_route ret;
    if (num_vertices(g) == 0)
        return ret;

    auto v = vertex(from_dpid);
    auto e = vertex(to_dpid);
    if (v == TopologyGraph::null_vertex() || e == TopologyGraph::null_vertex())
        return ret;

    std::vector<vertex_descriptor> p(num_vertices(g), TopologyGraph::null_vertex());

    // _constructor_ for weight_map
    auto select_metrics = [&g, &mf](TopologyGraph::edge_descriptor ed) {
        switch (mf) {
        case MetricsFlag::Hop:
            return g[ed].hop_metrics;
        case MetricsFlag::PortSpeed:
            return g[ed].ps_metrics;
        case MetricsFlag::PortLoading:
            return g[ed].pl_metrics;
        default:
            // FIXME: should throw exception?
            LOG(ERROR) << "[Topology] Incorrect metrics!";
            return (uint64_t)0;
        }
    };
    auto metrics_weight_map = 
         boost::make_function_property_map<TopologyGraph::edge_descriptor, uint64_t>
         (select_metrics);

    // computing predecessor_map with dijkstra algorithm
    dijkstra_shortest_paths_no_color_map(g, e, weight_map( metrics_weight_map )
        .predecessor_map( make_iterator_property_map(p.begin(), boost::get(vertex_index, g)) )
    );

    // computing result path from v to e
    // using predecessor_map
    auto u = p.at(v);
    auto from = from_dpid;
    while (v != e) {
        uint64_t min_metrics = 0;
        switch_and_port res1, res2;
        auto edges = edge_range(v, u, g);
        for (auto it = edges.first; it != edges.second; it++) {
            // comparing parallel links using selected metrics
            auto check_metrics = [&min_metrics](const auto& link, auto mf){
                uint64_t curr {0};
                switch (mf) {
                case MetricsFlag::Hop:
                    curr = link.hop_metrics;
                    break;
                case MetricsFlag::PortSpeed:
                    curr = link.ps_metrics;
                    break;
                case MetricsFlag::PortLoading:
                    curr = link.pl_metrics;
                    break;
                default:
                    LOG(ERROR) << "[Topology] Incorrect metrics";
                }

                if (!min_metrics || min_metrics > curr) {
                    min_metrics = curr;
                    return true;
                }
                return false;
            };

            const auto& link = g[*it];
            if (check_metrics(link, mf)) {
                res1 = link.source;
                res2 = link.target;
            }
        }

        // selected only one link among parallel links
        // with minimal metrics
        if (min_metrics) {
            if (res1.dpid == from) {
                ret.push_back(res1);
                ret.push_back(res2);
                from = res2.dpid;
            } else {
                ret.push_back(res2);
                ret.push_back(res1);
                from = res1.dpid;
            }
        } else {
            return ret;
        }

        // move to next vertexes in predecessor_map
        u = p[u];
        v = p[v];
    }
    return ret;
}

void TopologyImpl::maxWeight(const data_link_route& path, TopologyGraph& g) const
{
    if (path.size() == 0) return;

    for (auto it = path.begin(), next = it++; it != path.end(); it++, next++) {
        if (it->dpid == next->dpid)
            continue;

        CHECK(vertex(it->dpid) != TopologyGraph::null_vertex(), "Inconsistent topology graph");

        auto edges = edge_range(vertex(it->dpid), vertex(next->dpid), g);
        for (auto par = edges.first; par != edges.second; ++par) {
            link_property& prop = g[*par];
            if (prop.source != *it && prop.target != *it) { // if parallel links
                continue;
            }

            prop.hop_metrics += max_weight;
            prop.ps_metrics += max_weight;
            prop.pl_metrics += max_weight;
        }
    }
}

data_link_route TopologyImpl::exactPath(switch_list exact) const
{
    data_link_route ret;
    for (size_t i = 0; i < exact.size() - 1; i++) {
        auto curr = exact.at(i);
        auto sw = app->m_switch_manager->switch_(curr);
        auto next = exact.at(i+1);
        auto ports = sw->ports();

        auto found = std::find_if(ports.begin(), ports.end(), 
            [this, curr, next](auto port) {
                switch_and_port sp { curr, port->number() };
                return app->other(sp).dpid == next;
        });

        if (found == ports.end()) {
            LOG(WARNING) << "[Topology] Creating path - Can't create exact path"
                            "between <" << exact.front() <<
                            "> and <" << exact.back() << ">";
            return data_link_route{};
        }

        switch_and_port sp {curr, (*found)->number()};
        switch_and_port other = app->other(sp);
        ret.push_back(sp);
        ret.push_back(other);
    }

    return ret;
}

data_link_route TopologyImpl::inExPath(switch_list include, switch_list exclude,
                                       MetricsFlag m, RoutePtr route,  TopologyGraph& g) const
{
    data_link_route ret;
    auto from = route->from;
    auto to = route->to;

    for (auto dpid : exclude) {
        if (dpid != from && dpid != to && 
                vertex(dpid) != TopologyGraph::null_vertex())
            clear_vertex(vertex(dpid), g);
    }

    switch_list ends {from, to};
    if (include.size() > 0 && std::find_first_of(include.begin(), include.end(),
                           ends.begin(), ends.end()) != include.end()) {
        LOG(WARNING) << "[Topology] Creating path - "
                        "<from> and <to> DPIDs can't be in include list!";
        return ret;
    }
    
    include.push_back(to); // FIXME: may me reverse include? NP-hard task

    auto tfrom = from;
    for (auto dpid : include) {
        auto part = computePath(tfrom, dpid, m, g);
        if (part.size() == 0) {
            VLOG(2) << "[Topology] Creating path - No path between <" 
                    << from << "> and <" << to
                    << "> via <" << dpid << ">";
            return data_link_route{};
        }
        for (auto it : part)
            ret.push_back(it);

        tfrom = dpid;
    }

    return ret;
}

data_link_route TopologyImpl::findPath(RoutePtr route, RouteSelector selector) const
{
    using namespace route_selector;

    app->updateMetrics();

    data_link_route ret;
    TopologyGraph tmp_graph(graph);

    std::for_each(route->paths.begin(), route->paths.end(), [&tmp_graph, this](auto path) {
        this->maxWeight(path->m_path, tmp_graph);
    });

    // erase maintenance switches
    for (auto sw : app->m_switch_manager->switches()) {
        if (sw->maintenance()) {
            clear_vertex(vertex(sw->dpid()), tmp_graph);
        }
    }

    // erase maintenance and overloaded links
    uint8_t util = selector.get(util_trigger) ? *selector.get(util_trigger) : 0;
    for (auto sw : app->m_switch_manager->switches()) {
        if (sw->maintenance()) continue; // already removed

        for (auto port : sw->ports()) {
            auto sp = switch_and_port {sw->dpid(), port->number()};
            auto other = app->other(sp);
            if (other.dpid == 0 || sp.dpid == other.dpid) // not core port or loopback
                continue;

            if (port->maintenance()) { // erase maintenance port
                auto e = edge(sp, tmp_graph);
                if (e.second) {
                    remove_edge(e.first, tmp_graph);
                }
            }
            else if (util > 0) { // else check overload
                auto eth = std::dynamic_pointer_cast<EthernetPort>(port);
                auto stats = port->stats();
                auto& speed = stats.current_speed;
                uint64_t max = eth->current_speed(); // Kbps
                uint64_t tx = (uint64_t)speed.tx_bytes() * 8 / 1000;
                uint64_t rx = (uint64_t)speed.rx_bytes() * 8 / 1000;
                uint64_t allowed = max * util / 100;

                if (tx > allowed || rx > allowed) {
                    VLOG(7) << "[Topology] Overloaded link - " << sp;
                    auto e = edge(sp, tmp_graph);
                    if (e.second) {
                        remove_edge(e.first, tmp_graph);
                    }
                }
            }
        }
    }

    auto from = route->from;
    auto to = route->to;
    MetricsFlag metr = selector.get(metrics) ? *selector.get(metrics) : +MetricsFlag::Hop;

    if (selector.get(exact_dpid)) {
        ret = std::move(exactPath(std::move(*selector.get(exact_dpid))));
        // reverse if path description came with reverse order
        if (ret.size() > 1 && ret[0].dpid == to && ret[ret.size()-1].dpid == from) {
            std::reverse(std::begin(ret), std::end(ret));
        }
        // return empty result if path is incorrect
        if (ret.front().dpid != from || ret.back().dpid != to) {
            return data_link_route{};
        }

    } else if (selector.get(include_dpid) || selector.get(exclude_dpid)) {
        switch_list include, exclude;
        if (selector.get(include_dpid))
            include = std::move(*selector.get(include_dpid));
        if (selector.get(exclude_dpid))
            exclude = std::move(*selector.get(exclude_dpid));
        ret = std::move(inExPath(std::move(include), std::move(exclude),
                                 metr, route, tmp_graph));
    } else {
        ret = std::move(computePath(from, to, metr, tmp_graph));
    }

    if (ret.empty() || route->hasPath(ret))
        return data_link_route{};

    return ret;
}

Topology::Topology()
{
    m = new TopologyImpl(this);
}

Topology::~Topology()
{
    delete m;
}

void Topology::init(Loader *loader, const Config &rootConfig)
{
    QObject* ld = ILinkDiscovery::get(loader);
    ld_app = dynamic_cast<ILinkDiscovery*>(ld);
    m_switch_manager = SwitchManager::get(loader);
    recovery = RecoveryManager::get(loader);
    db_connector_ = DatabaseConnector::get(loader);

    stats_timer = new QTimer(this);
    connect(stats_timer, &QTimer::timeout, this, &Topology::reloadStats);
    stats_timer->start(2000);

    connect(recovery, &RecoveryManager::signalRecovery, this, &Topology::onRecovery);
    connect(recovery, &RecoveryManager::signalSetupPrimaryMode, 
                            this, &Topology::onPrimary);

    connect(ld, SIGNAL(linkDiscovered(switch_and_port, switch_and_port)),
            this, SLOT(linkDiscovered(switch_and_port, switch_and_port)));
    connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
            this, SLOT(linkBroken(switch_and_port, switch_and_port)));

    connect(m_switch_manager, &SwitchManager::portMaintenanceStart,
            this, &Topology::onPMaintenance);
    connect(m_switch_manager, &SwitchManager::portMaintenanceEnd,
            this, &Topology::onPMaintenanceOff);
    connect(m_switch_manager, &SwitchManager::switchMaintenanceStart,
            this, &Topology::onSMaintenance);
    connect(m_switch_manager, &SwitchManager::switchMaintenanceEnd,
            this, &Topology::onSMaintenanceOff);

    /* Do logging */
    connect(this, &Topology::routeTriggerActive,
         [](uint32_t id, uint8_t path_id, TriggerFlag tf) {
             LOG(WARNING) << "[Topology] Trigger " << tf._to_string()
                          << " active - On route "
                          << id << ":" << (int)path_id;
         });
    connect(this, &Topology::routeTriggerInactive,
         [](uint32_t id, uint8_t path_id, TriggerFlag tf) {
             LOG(WARNING) << "[Topology] Trigger " << tf._to_string()
                          << " inactive - On route "
                          << id << ":" << (int)path_id;
         });
}

void Topology::onRecovery()
{
    for (const auto& link : ld_app->links()) {
        addLink(link.source, link.target);
    }

    load_from_database();
}

void Topology::onPrimary()
{
    for (const auto& link : ld_app->links()) {
        addLink(link.source, link.target);
    }

    clear_database(); // TODO: think about it
}

void Topology::onPMaintenance(PortPtr port)
{
    std::vector<PathPtr> need_emit;
    switch_and_port mnt { port->switch_()->dpid(), port->number() };
    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [mnt, &need_emit](auto path) {
            if (path->contains(mnt)) {
                if (path->activateTrigger(TriggerFlag::Maintenance)) {
                    need_emit.push_back(path);
                }
            }
        });
    }

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        emit this->routeTriggerActive(path->route_id, path->id, TriggerFlag::Maintenance);
    });
}

void Topology::onPMaintenanceOff(PortPtr port)
{
    std::vector<PathPtr> need_emit;
    switch_and_port mnt { port->switch_()->dpid(), port->number() };
    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [mnt, &need_emit, this](auto path) {
            if (path->contains(mnt)) {
                if (path->inactivateTrigger(TriggerFlag::Maintenance, this)) {
                    need_emit.push_back(path);
                }
            }
        });
    }

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        if (path->flap)
            path->startFlapping(TriggerFlag::Maintenance, this);
        else
            emit this->routeTriggerInactive(path->route_id, path->id, TriggerFlag::Maintenance);
    });
}

void Topology::onSMaintenance(SwitchPtr sw)
{
    auto dpid = sw->dpid();
    std::vector<PathPtr> need_emit;
    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [dpid, &need_emit, this](auto path) {
            if (path->contains(dpid)) {
                if (path->activateTrigger(TriggerFlag::Maintenance)) {
                    need_emit.push_back(path);
                }
            }
        });
    }

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        emit this->routeTriggerActive(path->route_id, path->id, TriggerFlag::Maintenance);
    });
}

void Topology::onSMaintenanceOff(SwitchPtr sw)
{
    auto dpid = sw->dpid();
    std::vector<PathPtr> need_emit;
    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [dpid, &need_emit, this](auto path) {
            if (path->contains(dpid)) {
                if (path->inactivateTrigger(TriggerFlag::Maintenance, this)) {
                    need_emit.push_back(path);
                }
            }
        });
    }

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        if (path->flap)
            path->startFlapping(TriggerFlag::Maintenance, this);
        else
            emit this->routeTriggerInactive(path->route_id, path->id, TriggerFlag::Maintenance);
    });
}

std::vector<link_property> Topology::dumpWeights()
{
    return m->get_dump();
}

std::vector<uint32_t> Topology::getRoutes() const
{
    std::vector<uint32_t> ret;
    for (auto it : m->route_map) {
        ret.push_back(it.first);
    }

    return ret;
}

std::vector<uint32_t> Topology::getRoutes(ServiceFlag sf) const
{
    std::vector<uint32_t> ret;
    for (auto it : m->route_map) {
        if (it.second->owner == sf)
            ret.push_back(it.first);
    }

    return ret;
}

void Topology::linkDiscovered(switch_and_port from, switch_and_port to)
{
    std::vector<PathPtr> need_emit;

    { // mutex
    std::lock_guard<std::mutex> lk(m->graph_mutex);

    addLink(from, to);

    if (m->timer_id == 0) {
        m->timer_id = startTimer(ready_timeout * 1000);
    } else {
        killTimer(m->timer_id);
        m->timer_id = startTimer(ready_timeout * 1000);
    }

    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [from, &need_emit, this](auto path) {
            if (path->broken_flag && path->contains(from)) {
                if (path->inactivateTrigger(TriggerFlag::Broken, this)) { // if true, need emit
                    need_emit.push_back(path);
                }
            }
        });
    }

    } // mutex

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        if (path->flap)
            path->startFlapping(TriggerFlag::Broken, this);
        else
            this->emit routeTriggerInactive(path->route_id, path->id, TriggerFlag::Broken);
    });
}

void Topology::linkBroken(switch_and_port from, switch_and_port to)
{
    std::vector<PathPtr> need_emit;

    { // mutex
    std::lock_guard<std::mutex> lk(m->graph_mutex);

    auto e = m->edge(from, m->graph);
    if (e.second) {
        remove_edge(e.first, m->graph);
    }

    for (auto it : m->route_map) {
        auto& paths = it.second->paths;
        std::for_each(paths.begin(), paths.end(), [from, &need_emit](auto path) {
            if (path->broken_flag && path->contains(from)) {
                if (path->activateTrigger(TriggerFlag::Broken)) { // if true, need emit signal
                    need_emit.push_back(path);
                }
            }
        });
    }

    } // mutex

    std::for_each(need_emit.begin(), need_emit.end(), [this](auto path) {
        emit this->routeTriggerActive(path->route_id, path->id, TriggerFlag::Broken);
    });
}

void Topology::reloadStats()
{
    for (auto sw : m_switch_manager->switches()) {
        for (auto port : sw->ports()) {
            switch_and_port sp {sw->dpid(), port->number()};
            uint64_t other_dpid = other(sp).dpid;
            if (other_dpid == 0 || other_dpid == sp.dpid) // not core port or loopback
                continue;

            uint64_t max = getSpeedRate(sp);
            if (max == 0) {
                LOG(WARNING) << "[Topology] Incorrect port - "
                             << sp << " has null current_speed!";
                continue;
            }

            auto stats = port->stats();
            auto& speed = stats.current_speed;
            uint64_t tx = (uint64_t)speed.tx_bytes();
            uint64_t rx = (uint64_t)speed.rx_bytes();
            uint64_t tdrop = (uint64_t)speed.tx_dropped();
            uint64_t rdrop = (uint64_t)speed.rx_dropped();
            uint64_t terr = (uint64_t)speed.tx_errors();
            uint64_t rerr = (uint64_t)speed.rx_errors();

            tx = (tx >= LLONG_MAX ? 0 : tx);
            rx = (rx >= LLONG_MAX ? 0 : rx);
            tx = (tx >= max ? max : tx);
            rx = (rx >= max ? max : rx);
            tdrop = (tdrop >= LLONG_MAX ? 0 : tdrop);
            rdrop = (rdrop >= LLONG_MAX ? 0 : rdrop);
            terr = (terr >= LLONG_MAX ? 0 : terr);
            rerr = (rerr >= LLONG_MAX ? 0 : rerr);

            uint8_t tx_drops = (tx > 0 ? (100 * tdrop / tx) : 0);
            uint8_t rx_drops = (rx > 0 ? (100 * rdrop / rx) : 0);
            uint8_t tx_util  = (100 * tx   / max);
            uint8_t rx_util  = (100 * rx   / max);
            
            auto res = std::make_pair(std::max(tx_drops, rx_drops), std::max(tx_util, rx_util));
            m->triggers[sp] = res;
        }
    }

    std::vector<PathPtr> act_need_emit_drop, inact_need_emit_drop;
    std::vector<PathPtr> act_need_emit_util, inact_need_emit_util;

    auto check_triggers = [&](PathPtr path) {
        if (!path->drop_threshold && !path->util_threshold) return;

        bool util_overload = false;
        bool drop_overload = false;

        uint8_t max = 0;
        for (size_t i = 0; i < path->m_path.size(); i++) {
            auto curr = path->m_path.at(i);
            if (!m->triggers.count(curr)) break;
            auto triggers = m->triggers.at(curr);

            if (path->drop_threshold && triggers.first > path->drop_threshold)
                drop_overload = true;
            if (path->util_threshold && triggers.second > path->util_threshold)
                util_overload = true;

            max = std::max(triggers.second, max);
        }

        if (drop_overload && path->activateTrigger(TriggerFlag::Drop)) {
            act_need_emit_drop.push_back(path);
        }
        else if (!drop_overload && path->getTrigger(TriggerFlag::Drop) &&
                 path->flapping_timers.count(TriggerFlag::Drop) == 0) {

            if (path->inactivateTrigger(TriggerFlag::Drop, this))
                inact_need_emit_drop.push_back(path);
        }

        if (util_overload && path->activateTrigger(TriggerFlag::Util)) {
            act_need_emit_util.push_back(path);
        }
        else if (!util_overload && path->getTrigger(TriggerFlag::Util) &&
                 path->flapping_timers.count(TriggerFlag::Util) == 0) {

            if (path->inactivateTrigger(TriggerFlag::Util, this))
                inact_need_emit_util.push_back(path);
        }
    };

    for (auto it : m->route_map) {
        auto route = it.second;
        auto& paths = route->paths;
        std::for_each(paths.begin(), paths.end(), check_triggers);
    }

    auto emitting = [this](const auto& vec, TriggerFlag tf, bool act) {
        std::for_each(vec.begin(), vec.end(), [this, tf, act](auto path) {
            if (act)
                emit this->routeTriggerActive(path->route_id, path->id, tf);
            else {
                if (path->flap)
                    path->startFlapping(tf, this);
                else
                    emit this->routeTriggerInactive(path->route_id, path->id, tf);
            }
        });
    };

    emitting(act_need_emit_drop, TriggerFlag::Drop, true);
    emitting(act_need_emit_util, TriggerFlag::Util, true);
    emitting(inact_need_emit_drop, TriggerFlag::Drop, false);
    emitting(inact_need_emit_util, TriggerFlag::Util, false);
}

void Topology::updateMetrics()
{
    for (auto sw : m_switch_manager->switches()) {
        for (auto port : sw->ports()) {
            auto neighbor = other(switch_and_port{sw->dpid(), port->number()});
            if (neighbor.dpid == 0 || neighbor.dpid == sw->dpid()) // not core port or loopback
                continue;

            auto vert = m->vertex(sw->dpid());
            if (vert == TopologyGraph::null_vertex()) {
                continue;
            }

            auto edges = out_edges(vert, m->graph);
            for (auto it = edges.first; it != edges.second; it++) {
                if (target(*it, m->graph) != m->vertex(neighbor.dpid)) continue;

                link_property& prop = m->graph[*it];
                if (prop.source != neighbor && prop.target != neighbor) // parallel links
                    continue;

                auto eth = std::dynamic_pointer_cast<EthernetPort>(port);
                auto stats = port->stats();
                auto& speed = stats.current_speed;
                uint64_t tx = (uint64_t)speed.tx_bytes();
                uint64_t rx = (uint64_t)speed.rx_bytes();
                tx = (tx >= LLONG_MAX ? 0 : tx);
                rx = (rx >= LLONG_MAX ? 0 : rx);
                uint64_t max = getSpeedRate(neighbor); // Bps
                uint64_t cur = std::max(tx, rx);
                uint64_t weight = max_weight - 8 * (max - cur) / 1000000; // Mbit
                prop.pl_metrics = (weight > 0 ? weight : 1);
            }
        }
    }
}

void Topology::timerEvent(QTimerEvent *event)
{
    killTimer(m->timer_id);
    m->timer_id = 0;
    emit ready();
}

void Topology::addLink(switch_and_port from, switch_and_port to)
{
    if (from.dpid == to.dpid) {
        LOG(WARNING) << "[Topology] Ignoring loopback link - On " << from.dpid;
        return;
    }

    auto sw = m_switch_manager->switch_(from.dpid);
    if (not sw) return;
    auto port = sw->port(from.port);
    if (not port) return;

    auto eth = std::dynamic_pointer_cast<EthernetPort>(port);
    uint64_t speed = (eth ? eth->current_speed()/1000 : max_weight);
    auto u = m->new_vertex(from.dpid);
    auto v = m->new_vertex(to.dpid);

    uint64_t ps_metrics = (speed >= max_weight ? 1 : max_weight - speed + 1);
    add_edge(u, v, link_property{from, to, 1, ps_metrics, 1}, m->graph);
}

void Topology::switchUp(SwitchPtr sw)
{
    m->new_vertex(sw->dpid());
}

void Topology::switchDown(SwitchPtr sw)
{
    m->delete_vertex(sw->dpid());
}

void Topology::update_database(uint32_t route_id)
{
    if (!db_connector_) return;

    if (m->route_map.count(route_id)) {
        std::string route_dump = m->route_map.at(route_id)->to_json().dump();
        db_connector_->putSValue("topology:route", std::to_string(route_id), route_dump);
    }
}

void Topology::erase_from_database(uint32_t route_id)
{
    if (!db_connector_) return;

    db_connector_->delJson("topology:route", std::to_string(route_id));
}

void Topology::load_from_database()
{
    if (!db_connector_) return;

    auto keys = db_connector_->getKeys("topology:route");
    for (const auto& i : keys) {
        auto tmp = db_connector_->getSValue("topology:route", i);
        json jr = json::parse(tmp);
        uint32_t id = jr["id"];
        uint64_t from = jr["from"];
        uint64_t to = jr["to"];
        bool dynamic = jr["dynamic"];
        if (m->route_map.count(id)) continue;

        auto route = m->addRoute(from, to, id);
        std::string owner = jr["owner"];
        route->allowed_dynamic = dynamic;
        route->owner = ServiceFlag::_from_string(owner.c_str());
        route->used_path =jr["used_path"]; 

        for (auto& jp : jr["paths"]) {
            data_link_route p;
            for (auto& sp : jp["m_path"]) {
                p.push_back({sp.at(0), sp.at(1)});
            }
            auto path = route->attachPath(p);

            path->flap = jp["flapping"];
            path->broken_flag = jp["broken_flag"];
            path->drop_threshold = jp["drop_threshold"];
            path->util_threshold = jp["util_threshold"];
            std::string metrics = jp["metrics"];
            path->metrics = MetricsFlag::_from_string(metrics.c_str());
        }

        if (dynamic) {
            std::string mf = jr["dynamic_settings"]["metrics"];
            uint8_t flap = jr["dynamic_settings"]["flapping"];
            uint8_t drop = jr["dynamic_settings"]["drop_threshold"];
            uint8_t util = jr["dynamic_settings"]["util_threshold"];
            bool broken = jr["dynamic_settings"]["broken_flag"];

            RouteSelector dyn_sel {
                route_selector::metrics = MetricsFlag::_from_string(mf.c_str()),
                route_selector::flapping = flap,
                route_selector::broken_trigger = broken,
                route_selector::drop_trigger = drop,
                route_selector::util_trigger = util
            };

            route->dynamic = std::move(dyn_sel);
        }
    }
}

void Topology::clear_database()
{
    if (!db_connector_) return;

    db_connector_->delPrefix("topology");
}

switch_and_port Topology::other(switch_and_port sp)
{
   return ld_app->other(sp);
}

uint32_t Topology::newRoute(uint64_t from, uint64_t to, RouteSelector selector)
{
    using namespace route_selector;
    std::lock_guard<std::mutex> lk(m->graph_mutex);

    if (m_switch_manager->switch_(from) == nullptr ||
            m_switch_manager->switch_(to) == nullptr) {
        LOG(WARNING) << "[Topology] Creating route - No switch for route";
        return 0;
    }

    auto owner { ServiceFlag::None };
    if (selector.get(app))
        owner = *selector.get(app);

    auto route = m->addRoute(from, to);
    route->owner = owner;

    auto path_id = newPath(route->id, selector);
    if (path_id == max_path_id) {
        VLOG(1) << "[Topology] Creating route - Can't create route: "
                << from << " -> " << to;
        deleteRoute(route->id);
        return 0;
    }

    auto path = route->paths.at(path_id);

    if (selector.get(configured_count)) {
        uint8_t count = *selector.get(configured_count);
        if (count > 0 && count < 10) { // allowed values
            RouteSelector aux_selector { metrics = path->metrics,
                                         flapping = path->flap,
                                         broken_trigger = path->broken_flag,
                                         drop_trigger = path->drop_threshold,
                                         util_trigger = path->util_threshold,
                                         exclude_dpid =
                                             *selector.get(exclude_dpid)
                                       };
            for (auto i = 1; i < count; i++) { // aux paths
                newPath(route->id, aux_selector);
            }
        }
    }

    update_database(route->id);
    return route->id;
}

uint8_t Topology::newPath(uint32_t route_id, RouteSelector selector)
{
    //TODO: mutex
    using namespace route_selector;
    if (m->route_map.count(route_id) == 0)
        return max_path_id;

    auto route = m->route_map.at(route_id);
    auto computed = std::move(m->findPath(route, selector));

    if (computed.size() == 0)
        return max_path_id;

    auto path = route->attachPath(computed);

    // get parameters of triggers
    if (selector.get(flapping))
        path->flap = *selector.get(flapping);
    if (selector.get(broken_trigger))
        path->broken_flag = *selector.get(broken_trigger);
    if (selector.get(drop_trigger))
        path->drop_threshold = *selector.get(drop_trigger);
    if (selector.get(util_trigger))
        path->util_threshold = *selector.get(util_trigger);

    path->metrics = selector.get(metrics) ? *selector.get(metrics)
                                          : +MetricsFlag::Hop;

    VLOG(2) << "[Topology] Created path - "
            << route_id << ":" << (int)path->id;
    update_database(route_id);
    return path->id;
}

bool Topology::deletePath(uint32_t route_id, uint8_t path_id)
{
    // we use async because `deletePath` method can be called
    // from another thread (REST) and
    // we must stop flapping timers (if exists) from app's thread.
    return async(executor, [=]() {
        if (m->route_map.count(route_id) == 0) return false;

        auto route = m->route_map.at(route_id);
        if (route->paths.size() > 1 &&           // erase if only more than one path
                route->used_path != path_id &&   // and this path isn't using now
                route->paths.size() > path_id) { // path_id exists
            route->detachPath(path_id);
        } else {
            return false;
        }

        VLOG(2) << "[Topology] Removed path - "
                << route_id << ":" << (int)path_id;
        update_database(route_id);
        return true;
    }).get();
}

data_link_route Topology::predictPath(uint32_t route_id) const
{
    if (m->route_map.count(route_id) == 0)
        return data_link_route{};

    auto route = m->route_map.at(route_id);
    if (not route->allowed_dynamic) return data_link_route{};

    auto computed = std::move(m->findPath(route, route->dynamic));
    return computed.size() > 0 ? computed : data_link_route{};
}

data_link_route Topology::getPath(uint32_t route_id, uint8_t path_id) const
{
    //TODO: mutex
    if (m->route_map.count(route_id) == 0)
        return data_link_route{};

    auto route = m->route_map.at(route_id);
    auto path = route->getPath(path_id);

    return path == nullptr ? data_link_route{} : path->m_path;
}

data_link_route Topology::getFirstWorkPath(uint32_t route_id) const
{
    if (m->route_map.count(route_id) == 0)
        return data_link_route{};

    auto route = m->route_map.at(route_id);
    auto path = route->getFirstWorkPath();

    return (path ? path->m_path : data_link_route{});
}

uint8_t Topology::getFirstWorkPathId(uint32_t route_id) const
{
    if (m->route_map.count(route_id) == 0)
        return max_path_id;

    auto route = m->route_map.at(route_id);
    auto path = route->getFirstWorkPath();

    return (path ? path->id : max_path_id);

}

ServiceFlag Topology::getOwner(uint32_t id) const
{
    if (m->route_map.count(id) == 0)
        return ServiceFlag::None;

    return m->route_map.at(id)->owner;
}

uint8_t Topology::getFlapping(uint32_t id, uint8_t path_id) const
{
    try {
        return m->route_map.at(id)->paths.at(path_id)->flap;
    } catch (const std::out_of_range& ex) {
        LOG(WARNING) << "[Topology] Get info - Trying to get bad route";
        LOG(WARNING) << ex.what();
    }
    return 0;
}

uint8_t Topology::getTriggerThreshold(uint32_t id, uint8_t path_id, TriggerFlag tf) const
{ // TODO: make enum safety!
    try {
        auto path = m->route_map.at(id)->paths.at(path_id);
        if (tf == +TriggerFlag::Broken)
            return (uint8_t)path->broken_flag;

        else if (tf == +TriggerFlag::Drop)
            return path->drop_threshold;

        else if (tf == +TriggerFlag::Util)
            return path->util_threshold;
    } catch (const std::out_of_range& ex) {
        LOG(WARNING) << "[Topology] Get info - Trying to get bad route";
        LOG(WARNING) << ex.what();
    }

    return 0;
}

bool Topology::getStatus(uint32_t id, uint8_t path_id, TriggerFlag tf) const
{
    try {
        return m->route_map.at(id)->paths.at(path_id)->getTrigger(tf);
    } catch (const std::out_of_range& ex) {
        LOG(WARNING) << "[Topology] Get info - Trying to get bad route or path";
        LOG(WARNING) << ex.what();
    }

    return false;
}

uint64_t Topology::getMinRouteRate(uint32_t id, uint8_t path_id) const
{
    if (m->route_map.count(id) == 0)
        return 0;
    auto route = m->route_map.at(id);
    auto path = route->getPath(path_id);
    if (!path) {
        LOG(ERROR) << "[Topology] Get info - Trying to get bad path";
        return 0;
    }

    uint64_t min_rate = LLONG_MAX;
    for (size_t i = 0; i < path->m_path.size(); i += 2) {
        auto curr = path->m_path.at(i);
        auto sw = m_switch_manager->switch_(curr.dpid);
        auto port = sw->port(curr.port);

        auto eth = std::dynamic_pointer_cast<EthernetPort>(port);
        uint64_t max = eth->current_speed()*125; // bytes/s
        if (min_rate > max) min_rate = max;
    }

    return min_rate;
}

std::pair<uint64_t, uint64_t> Topology::getPortUtility(switch_and_port sp) const
{
    auto sw = m_switch_manager->switch_(sp.dpid);
    auto port = sw->port(sp.port);

    auto stats = port->stats();
    auto& speed = stats.current_speed;
    uint64_t tx = (uint64_t)speed.tx_bytes();
    uint64_t rx = (uint64_t)speed.rx_bytes();

    tx = (tx >= LLONG_MAX ? 0 : tx);
    rx = (rx >= LLONG_MAX ? 0 : rx);
    return {tx, rx};
}

uint64_t Topology::getSpeedRate(switch_and_port sp) const
{
    if (m->speed_rate.count(sp)) {
        return m->speed_rate[sp];
    }

    auto get_speed_rate = [mgr = this->m_switch_manager]
        (switch_and_port sp) -> uint64_t {
            auto sw = mgr->switch_(sp.dpid);
            if (not sw) {
                return 0;
            }
            auto port = sw->port(sp.port);
            if (not port) {
                return 0;
            }
            auto eth = std::dynamic_pointer_cast<EthernetPort>(port);
            return eth->current_speed()*125; // bytes/s
        };

    auto rate = get_speed_rate(sp);
    // try to get neighbor speed rate
    if (not rate) {
        auto neighbor = ld_app->other(sp);
        rate = get_speed_rate(neighbor);
    }
    m->speed_rate.emplace(sp, rate);

    return rate;
}

std::string Topology::getRouteDump(uint32_t id) const
{
    if (m->route_map.count(id) == 0)
        return "";

    return m->route_map.at(id)->to_json().dump();
}

uint8_t Topology::getUsedPath(uint32_t id) const
{
    if (m->route_map.count(id) == 0)
        return max_path_id;

    return m->route_map.at(id)->used_path;
}

void Topology::setUsedPath(uint32_t id, uint8_t path_id)
{
    if (m->route_map.count(id) == 0) return;

    auto route = m->route_map.at(id);
    if (route->used_path == path_id) return;

    route->used_path = path_id;
    if (path_id < route->paths.size()) {
        // reset triggers on selected path
        auto path = route->paths[path_id];
        path->resetTriggers();
    }

    update_database(id);
}

bool Topology::movePath(uint32_t id, uint8_t path_id, uint8_t new_id)
{
    if (m->route_map.count(id) == 0 || path_id == new_id) return false;

    auto route = m->route_map.at(id);
    uint8_t size = route->paths.size();
    if (path_id >= size || new_id >= size) return false;

    auto path = route->paths.at(path_id);
    auto diff = new_id - path_id;
    route->paths.insert(route->paths.erase(route->paths.begin() + path_id) + diff, path);
    if (path_id == route->used_path)
        route->used_path = new_id;
    else if (new_id > path_id && route->used_path > path_id && route->used_path <= new_id) {
        route->used_path--;
    } else if (new_id < path_id && route->used_path >= new_id && route->used_path < path_id) {
        route->used_path++;
    }

    for (size_t i = 0; i < route->paths.size(); i++)
        route->paths.at(i)->id = i;

    update_database(id);
    return true;
}

bool Topology::modifyPath(uint32_t id, uint8_t path_id, RouteSelector selector)
{
    using namespace route_selector;

    if (m->route_map.count(id) == 0) {
        return false;
    }

    auto route = m->route_map.at(id);
    auto path = route->getPath(path_id);
    if (not path) {
        return false;
    }

    path->flap =           selector.get(flapping) ?
                          *selector.get(flapping) : path->flap;
    path->broken_flag =    selector.get(broken_trigger) ?
                          *selector.get(broken_trigger) : path->broken_flag;
    path->drop_threshold = selector.get(drop_trigger) ?
                          *selector.get(drop_trigger) : path->drop_threshold;
    path->util_threshold = selector.get(util_trigger) ?
                          *selector.get(util_trigger) : path->util_threshold;
    return true;
}

bool Topology::addDynamic(uint64_t route_id, RouteSelector selector)
{
    if (m->route_map.count(route_id) == 0) return false;

    auto route = m->route_map.at(route_id);
    route->dynamic = std::move(selector);
    route->allowed_dynamic = true;
    update_database(route_id);
    return true;
}

bool Topology::delDynamic(uint64_t route_id)
{
    if (m->route_map.count(route_id) == 0) return false;

    auto route = m->route_map.at(route_id);
    route->dynamic = RouteSelector{};
    route->allowed_dynamic = false;
    update_database(route_id);
    return true;
}

uint8_t Topology::getDynamic(uint64_t route_id)
{
    if (m->route_map.count(route_id) == 0) return max_path_id;

    auto ret {max_path_id};
    auto route = m->route_map.at(route_id);
    if (route->allowed_dynamic)
        ret = newPath(route_id, route->dynamic);

    return ret;
}

void Topology::deleteRoute(uint32_t id)
{
    //std::lock_guard<std::mutex> lk(m->graph_mutex);
    m->eraseRoute(id);
    erase_from_database(id);
}

uint8_t Topology::minHops(uint64_t from, uint64_t to)
{
    if (from == to) return 0;

    auto path = std::move(m->computePath(from, to, MetricsFlag::Hop, m->graph));
    return path.size()/2;
}

} // namespace runos
