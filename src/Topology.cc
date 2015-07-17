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

#include "Topology.hh"

#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>

#include "Common.hh"

REGISTER_APPLICATION(Topology, {"link-discovery", "rest-listener", ""})

using namespace boost;

struct link_property {
    switch_and_port source;
    switch_and_port target;
    int weight;
};

class Link : public AppObject {
    switch_and_port source;
    switch_and_port target;
    int weight;
    uint64_t obj_id;

public:
    Link(switch_and_port _source, switch_and_port _target, int _weight, uint64_t _id):
        source(_source), target(_target), weight(_weight), obj_id(_id) {}

    json11::Json to_json() const;
    json11::Json to_floodlight_json() const;
    uint64_t id() const;
};

uint64_t Link::id() const {
    return obj_id;
}

json11::Json Link::to_json() const {
    json11::Json src = json11::Json::object {
        {"src_id", uint64_to_string(source.dpid)},
        {"src_port", (int)source.port} };
    json11::Json dst = json11::Json::object {
        {"dst_id", uint64_to_string(target.dpid)},
        {"dst_port", (int)target.port} };

    return json11::Json::object {
        {"ID", id_str()},
        {"connect", json11::Json::array { src, dst } },
        {"bandwidth", weight}
    };
}

json11::Json Link::to_floodlight_json() const {
    return json11::Json::object {
        {"src-switch", AppObject::uint64_to_string(source.dpid)},
        {"src-port", (int)source.port},
        {"dst-switch", AppObject::uint64_to_string(target.dpid)},
        {"dst-port", (int)target.port},
        {"type", "internal"},
        {"direction", "bidirectional"}
    };
}

typedef adjacency_list< vecS, vecS, undirectedS, no_property, link_property>
    TopologyGraph;

typedef TopologyGraph::vertex_descriptor
    vertex_descriptor;

struct TopologyImpl {
    QReadWriteLock graph_mutex;

    TopologyGraph graph;
    std::unordered_map<uint64_t, vertex_descriptor>
        vertex_map;

    vertex_descriptor vertex(uint64_t dpid) {
        auto it = vertex_map.find(dpid);
        if (it != vertex_map.end()) {
            return it->second;
        } else {
            return vertex_map[dpid] = add_vertex(graph);
        }
    }
};

void Topology::init(Loader *loader, const Config &config)
{
    QObject* ld = ILinkDiscovery::get(loader);

    QObject::connect(ld, SIGNAL(linkDiscovered(switch_and_port, switch_and_port)),
                     this, SLOT(linkDiscovered(switch_and_port, switch_and_port)));
    QObject::connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
                     this, SLOT(linkBroken(switch_and_port, switch_and_port)));

    RestListener::get(loader)->newListener("topology", r);
}

Topology::Topology()
{
    m = new TopologyImpl;
    r = new TopologyRest("Topology", "topology.html");
    r->makeEventApp();
}

Topology::~Topology()
{
    delete m;
    delete r;
}

void Topology::linkDiscovered(switch_and_port from, switch_and_port to)
{
    QWriteLocker locker(&m->graph_mutex);

    if (from.dpid == to.dpid) {
        LOG(WARNING) << "Ignoring loopback link on " << FORMAT_DPID << from.dpid;
        return;
    }

    /* TODO: calculate metric */
    auto u = m->vertex(from.dpid);
    auto v = m->vertex(to.dpid);
    add_edge(u, v, link_property{from, to, 1}, m->graph);

    Link* link = new Link(from, to, 5, rand()%1000 + 2000);
    r->topo.push_back(link);
    r->addEvent(Event::Add, link);
}

void Topology::linkBroken(switch_and_port from, switch_and_port to)
{
    QWriteLocker locker(&m->graph_mutex);
    remove_edge(m->vertex(from.dpid), m->vertex(to.dpid), m->graph);

    //TODO: remove from r->topo
    //TODO: create Event::Delete event
}

data_link_route Topology::computeRoute(uint64_t from_dpid, uint64_t to_dpid)
{
    DVLOG(5) << "Computing route between "
        << FORMAT_DPID << from_dpid << " and " << FORMAT_DPID << to_dpid;

    QReadLocker locker(&m->graph_mutex);
    auto& graph = m->graph;

    std::vector<vertex_descriptor> p(num_vertices(graph), TopologyGraph::null_vertex());

    dijkstra_shortest_paths_no_color_map(graph, m->vertex(to_dpid),
         weight_map( boost::get(&link_property::weight, graph) )
        .predecessor_map( make_iterator_property_map(p.begin(), boost::get(vertex_index, graph)) )
    );

    data_link_route ret;
    // TODO: compute complete route

    vertex_descriptor v = m->vertex(from_dpid);
    vertex_descriptor u = p.at(v);

    if (u != TopologyGraph::null_vertex()) {
        DVLOG(10) << "Getting properties of edge (" << v << ", " << u << ")";
        link_property link = graph[edge(v, u, graph).first];

        if (link.source.dpid == from_dpid) {
            ret.push_back(link.source);
            ret.push_back(link.target);
        } else {
            assert(link.target.dpid == from_dpid);
            ret.push_back(link.target);
            ret.push_back(link.source);
        }
    }

    return ret;
}

std::string TopologyRest::handle(std::vector<std::string> params)
{
    if (params[0] == "GET") {
        if (params[2] == "links") {
            return json11::Json(topo).dump();
        }
        if (params[2] == "f_links") {
            std::vector<json11::Json> res;
            for (auto it = topo.begin(); it != topo.end(); it++)
                res.push_back((*it)->to_floodlight_json());
            return json11::Json(res).dump();
        }
        if (params[2] == "external_links") {
            return "[]";
        }
    }
    
    return "{}";
}

int TopologyRest::count_objects()
{
    return topo.size();
}

TopologyRest::~TopologyRest()
{
    for (auto it = topo.begin(); it != topo.end(); it++) {
        //TODO: check if all element are deleted
        delete *it;
    }
}
