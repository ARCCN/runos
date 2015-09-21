#include "PathVerifier.hh"

REGISTER_APPLICATION(PathVerifier, {"controller", "link-discovery", "switch-manager", ""})

void PathVerifier::init(Loader* loader, const Config& config)
{
    sm = SwitchManager::get(loader);
    QObject* ld = ILinkDiscovery::get(loader);
    QObject::connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkBroken(switch_and_port, switch_and_port)));

    LearningSwitch* ls = LearningSwitch::get(loader);
    connect(ls, &LearningSwitch::newRoute, this, &PathVerifier::onNewRoute);

}

Route* PathVerifier::findRoute(std::string src, std::string dst)
{
    for (auto r : routes) {
        if (r->eth_src == src && r->eth_dst == dst)
            return r;
    }
    return NULL;
}

static const uint64_t flowCookieBase = 0x100000000UL;
static const uint64_t flowCookieMask = 0xffffffff00000000UL;

void PathVerifier::removeFlows(switch_and_port sp)
{
    Switch* sw = sm->getSwitch(sp.dpid);
    if (sw->ofconn()) {
        of13::FlowMod fm;
        fm.cookie(flowCookieBase);
        fm.cookie_mask(flowCookieMask);
        fm.table_id(1);
        fm.command(of13::OFPFC_DELETE);
        fm.out_port(of13::OFPP_ANY);
        fm.out_group(of13::OFPG_ANY);

        uint8_t* buf = fm.pack();
        sw->ofconn()->send(buf, fm.length());
        OFMsg::free_buffer(buf);
    }
}

void PathVerifier::onLinkBroken(switch_and_port from, switch_and_port to)
{
    for (auto it = routes.begin(); it != routes.end(); it++) {
        Route* r = *it;
        bool found = false;
        for (switch_and_port sp : r->path) {
            if (sp == from || sp == to) {
                found = true;
                break;
            }
        }
        if (found) {
            LOG(WARNING) << "route broken";
            auto prev = --it;

            for (Flow* flow : r->flows) {
                if (flow) {
                    flow->setDestroy();
                }
            }
            for (switch_and_port sp : r->path) {
                removeFlows(sp);
            }

            routes.erase(std::remove(routes.begin(), routes.end(), r), routes.end());
            delete r;
            it = prev;
        }
    }
}

void PathVerifier::onNewRoute(Flow* flow, std::string src, std::string dst, uint64_t dpid, uint32_t out_port)
{
    Route* route = findRoute(src, dst);
    if (route) {
        switch_and_port sp;
        sp.dpid = dpid;
        sp.port = out_port;
        route->path.push_back(sp);
    }
    else {
        route = new Route(src, dst);
        routes.push_back(route);
    }
    route->flows.push_back(flow);
}
