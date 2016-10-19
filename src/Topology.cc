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
#include <boost/lexical_cast.hpp>
#include <boost/assert.hpp>

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

    json11::Json to_json() const override;
    json11::Json to_floodlight_json() const;
    uint64_t id() const override;

    friend class Topology;
};

uint64_t Link::id() const {
    return obj_id;
}

Link* Topology::getLink(switch_and_port from, switch_and_port to)
{
    for (Link* it : topo) {
        if ((it->source == from && it->target == to) || (it->source == to && it->target == from))
            return it;
    }
    return nullptr;
}

json11::Json Link::to_json() const {
    json11::Json src = json11::Json::object {
        {"src_id", boost::lexical_cast<std::string>(source.dpid)},
        {"src_port", (int)source.port} };
    json11::Json dst = json11::Json::object {
        {"dst_id", boost::lexical_cast<std::string>(target.dpid)},
        {"dst_port", (int)target.port} };

    return json11::Json::object {
        {"ID", id_str()},
        {"connect", json11::Json::array { src, dst } },
        {"bandwidth", weight}
    };
}

json11::Json Link::to_floodlight_json() const {
    return json11::Json::object {
        {"src-switch", boost::lexical_cast<std::string>(source.dpid)},
        {"src-port", (int)source.port},
        {"dst-switch", boost::lexical_cast<std::string>(target.dpid)},
        {"dst-port", (int)target.port},
        {"type", "internal"},
        {"direction", "bidirectional"}
    };
}

struct dpid_t {
    typedef vertex_property_tag kind;
};

typedef adjacency_list< vecS, vecS, undirectedS,
                        property<dpid_t, uint64_t>,
                        link_property>
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
            auto v = it->second;
            BOOST_ASSERT(get(dpid_t(), graph, v) == dpid);
            return v;
        } else {
            auto v = vertex_map[dpid] = add_vertex(graph);
            put(dpid_t(), graph, v, dpid);
            return v;
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

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "links");
}

Topology::Topology()
{
    m = new TopologyImpl;
}

Topology::~Topology()
{
    delete m;
}

void Topology::linkDiscovered(switch_and_port from, switch_and_port to)
{
    QWriteLocker locker(&m->graph_mutex);

    if (from.dpid == to.dpid) {
        LOG(WARNING) << "Ignoring loopback link on " << from.dpid;
        return;
    }

    /* TODO: calculate metric */
    auto u = m->vertex(from.dpid);
    auto v = m->vertex(to.dpid);
    add_edge(u, v, link_property{from, to, 1}, m->graph);

    Link* link = new Link(from, to, 5, rand()%1000 + 2000);
    topo.push_back(link);
    addEvent(Event::Add, link);
}

void Topology::linkBroken(switch_and_port from, switch_and_port to)
{
    QWriteLocker locker(&m->graph_mutex);
    remove_edge(m->vertex(from.dpid), m->vertex(to.dpid), m->graph);

    Link* link = getLink(from, to);
    addEvent(Event::Delete, link);
    topo.erase(std::remove(topo.begin(), topo.end(), link), topo.end());
}

data_link_route Topology::computeRoute(uint64_t from_dpid, uint64_t to_dpid)
{
    DVLOG(5) << "Computing route between " << from_dpid << " and " << to_dpid;

    QReadLocker locker(&m->graph_mutex);
    const auto& graph = m->graph;
    vector_property_map<vertex_descriptor> p;
    vertex_descriptor v = m->vertex(from_dpid);

    dijkstra_shortest_paths_no_color_map(graph, m->vertex(to_dpid),
         weight_map( boost::get(&link_property::weight, graph) )
        .predecessor_map( p )
    );

    data_link_route ret;


    BOOST_ASSERT( v != TopologyGraph::null_vertex() );

    for (; v != p[v]; v = p[v]) {
        BOOST_ASSERT(edge(v, p[v], graph).second);
        link_property link = graph[edge(v, p[v], graph).first];

        if (link.source.dpid == boost::get(dpid_t(), graph, v)) {
            BOOST_ASSERT(link.target.dpid == boost::get(dpid_t(), graph, p[v]));
            ret.push_back(link.source);
            ret.push_back(link.target);
        } else {
            BOOST_ASSERT(link.target.dpid == boost::get(dpid_t(), graph, v));
            BOOST_ASSERT(link.source.dpid == boost::get(dpid_t(), graph, p[v]));
            ret.push_back(link.target);
            ret.push_back(link.source);
        }
    }

    return ret;
}

json11::Json Topology::handleGET(std::vector<std::string> params, std::string body)
{
    if (params[0] == "links")
        return json11::Json(topo).dump();

    return "{}";
}
