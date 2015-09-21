#pragma once

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "Controller.hh"
#include "ILinkDiscovery.hh"
#include "Switch.hh"
#include "LearningSwitch.hh"

struct Route {
    std::string eth_src;
    std::string eth_dst;
    std::vector<switch_and_port> path;
    std::vector<Flow*> flows;

    Route(std::string src, std::string dst) : eth_src(src), eth_dst(dst) {}
};

class PathVerifier : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(PathVerifier, "path-verifier")
public:
    void init(Loader* loader, const Config& config) override;
private:
    SwitchManager* sm;

    std::vector<Route*> routes;
    Route* findRoute(std::string src, std::string dst);
    void removeFlows(switch_and_port sp);

private slots:
    void onLinkBroken(switch_and_port from, switch_and_port to);
    void onNewRoute(Flow* flow, std::string src, std::string dst, uint64_t dpid, uint32_t out_port);
};
