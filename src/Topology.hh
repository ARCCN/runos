#pragma once

#include <QtCore>
#include <vector>

#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"
#include "ILinkDiscovery.hh"
#include "Controller.hh"
#include "Rest.hh"
#include "RestListener.hh"
#include "AppObject.hh"

typedef std::vector< switch_and_port > data_link_route;

/**
 * REST interface for Topology
 */
class TopologyRest : public Rest {
    friend class Topology;
    std::vector<class Link*> topo;
public:
    TopologyRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;
    int count_objects() override;
    ~TopologyRest();
};

class Topology : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(Topology, "topology")
public:
    Topology();
    void init(Loader* provider, const Config& config) override;
    Rest* rest() {return dynamic_cast<Rest*>(r); }
    ~Topology();

    data_link_route computeRoute(uint64_t from, uint64_t to);

protected slots:
    void linkDiscovered(switch_and_port from, switch_and_port to);
    void linkBroken(switch_and_port from, switch_and_port to);

private:
    struct TopologyImpl* m;
    struct TopologyRest* r;
};
