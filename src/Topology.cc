#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>

#include "Common.hh"
#include "Topology.hh"

REGISTER_APPLICATION(Topology, {"link-discovery", ""})

using namespace boost;

struct link_property {
    switch_and_port source;
    switch_and_port target;
    int weight;
};

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
        LOG(WARNING) << "Ignoring loopback link on " << FORMAT_DPID << from.dpid;
        return;
    }

    /* TODO: calculate metric */
    auto u = m->vertex(from.dpid);
    auto v = m->vertex(to.dpid);
    add_edge(u, v, link_property{from, to, 1}, m->graph);
}

void Topology::linkBroken(switch_and_port from, switch_and_port to)
{
    QWriteLocker locker(&m->graph_mutex);
    remove_edge(m->vertex(from.dpid), m->vertex(to.dpid), m->graph);
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
