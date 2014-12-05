#pragma once

#include <QtCore>
#include <vector>

#include "Application.hh"
#include "Loader.hh"
#include "ILinkDiscovery.hh"

typedef std::vector< switch_and_port > data_link_route;

class Topology : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(Topology, "topology")
public:
    Topology();
    void init(Loader* provider, const Config& config) override;
    ~Topology();

    data_link_route computeRoute(uint64_t from, uint64_t to);

protected slots:
    void linkDiscovered(switch_and_port from, switch_and_port to);
    void linkBroken(switch_and_port from, switch_and_port to);

private:
    struct TopologyImpl* m;
};
