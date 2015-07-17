#include "STP.hh"

REGISTER_APPLICATION(STP, {"switch-manager", "link-discovery", "topology", ""})

void SwitchSTP::computeSTP()
{
    parent->computePathForSwitch(this->sw->id());
}

void SwitchSTP::resetBroadcast()
{
    for (auto port : ports) {
        if (port.second->to_switch)
            unsetBroadcast(port.second->port_no);
    }
}

void SwitchSTP::setSwitchPort(uint32_t port_no, uint64_t dpid)
{
    ports.at(port_no)->to_switch = true;
    ports.at(port_no)->nextSwitch = parent->switch_list[dpid];
}

void STP::init(Loader* loader, const Config& config)
{    
    QObject* ld = ILinkDiscovery::get(loader);

    connect(ld, SIGNAL(linkDiscovered(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkDiscovered(switch_and_port, switch_and_port)));
    connect(ld, SIGNAL(linkBroken(switch_and_port, switch_and_port)),
                     this, SLOT(onLinkBroken(switch_and_port, switch_and_port)));

    SwitchManager* sw = SwitchManager::get(loader);
    connect(sw, &SwitchManager::switchDiscovered, this, &STP::onSwitchDiscovered);
    connect(sw, &SwitchManager::switchDown, this, &STP::onSwitchDown);

    topo = Topology::get(loader);
}

std::vector<uint32_t> STP::getSTP(uint64_t dpid)
{
    std::vector<uint32_t> ports;
    if (switch_list.count(dpid) == 0)
        return ports;

    SwitchSTP* sw = switch_list[dpid];    
    if (!sw->computed)
        return ports;

    for (auto port : sw->ports) {
        if (port.second->broadcast)
            ports.push_back(port.second->port_no);
    }
    return ports;
}

void STP::onLinkDiscovered(switch_and_port from, switch_and_port to)
{
    if (switch_list.count(from.dpid) == 0)
        return;

    if (switch_list.count(to.dpid) == 0)
        return;

    SwitchSTP* sw = switch_list[from.dpid];
    if (sw->existsPort(from.port)) {
        sw->unsetBroadcast(from.port);
        sw->setSwitchPort(from.port, to.dpid);
    } else {
        Port* port = new Port(from.port, false);
        sw->ports[from.port] = port;
        sw->setSwitchPort(from.port, to.dpid);
    }

    sw = switch_list[to.dpid];
    if (sw->existsPort(to.port)) {
        sw->unsetBroadcast(to.port);
        sw->setSwitchPort(to.port, from.dpid);
    } else {
        Port* port = new Port(to.port, false);
        sw->ports[to.port] = port;
        sw->setSwitchPort(to.port, from.dpid);
    }

    // recompute pathes for all switches
    for (auto ss : switch_list) {
        if (!ss.second->root)
            ss.second->computed = false;
    }
}

void STP::onLinkBroken(switch_and_port from, switch_and_port to)
{
    // recompute pathes for all switches
    for (auto ss : switch_list) {
        if (!ss.second->root)
            ss.second->computed = false;
    }
}

void STP::onSwitchDiscovered(Switch* dp)
{
    SwitchSTP* sw;
    if (switch_list.empty())
        sw = new SwitchSTP(dp, this, true, true);
    else
        sw = new SwitchSTP(dp, this);

    switch_list[dp->id()] = sw;

    connect(dp, &Switch::portUp, this, &STP::onPortUp);
    connect(sw->timer, SIGNAL(timeout()), sw, SLOT(computeSTP()));
    sw->timer->start(POLL_TIMEOUT * 1000);
}

void STP::onSwitchDown(Switch* dp)
{
    if (switch_list.count(dp->id() > 0)) {
        SwitchSTP* sw = switch_list[dp->id()];
        sw->timer->stop();
        switch_list.erase(dp->id());
    }
}

void STP::onPortUp(Switch *dp, of13::Port port)
{
    if (switch_list.count(dp->id()) > 0) {
        SwitchSTP* sw = switch_list[dp->id()];
        if ( !sw->existsPort(port.port_no()) && port.port_no() < of13::OFPP_MAX ) {
            Port* p = new Port(port.port_no());
            sw->ports[port.port_no()] = p;
        }
    }
}

SwitchSTP* STP::findRoot()
{
    for (auto it : switch_list) {
        if (it.second->root)
            return it.second;
    }
    return NULL;
}

void STP::computePathForSwitch(uint64_t dpid)
{
    if (!switch_list[dpid]->computed) {
        SwitchSTP* root = findRoot();
        if (root == NULL) {
            LOG(ERROR) << "Root switch not found!";
            return;
        }

        SwitchSTP* sw = switch_list[dpid];
        std::vector<uint32_t> old_broadcast = getSTP(dpid);
        sw->resetBroadcast();

        data_link_route route = topo->computeRoute(dpid, root->sw->id());
        if (route.size() > 0) {
            uint32_t broadcast_port = route[0].port;

            if (sw->existsPort(broadcast_port))
                sw->setBroadcast(broadcast_port);

            sw->nextSwitchToRoot = switch_list[route[1].dpid];

            // getting broadcast port on second switch
            data_link_route r_route = topo->computeRoute(route[1].dpid, dpid);
            SwitchSTP* r_sw = switch_list[r_route[0].dpid];
            uint32_t r_broadcast_port = r_route[0].port;
            if (r_sw->existsPort(r_broadcast_port))
                r_sw->setBroadcast(r_broadcast_port);

            for (auto port : sw->ports) {
                if (port.second->to_switch) {
                    if (port.second->nextSwitch->nextSwitchToRoot == sw) {
                        sw->setBroadcast(port.second->port_no);
                    }
                }
            }

            if (getSTP(dpid) == old_broadcast)
                sw->computed = true;

        } else {
            LOG(WARNING) << "Path between " << FORMAT_DPID << dpid
                << " and root switch " << FORMAT_DPID << root->sw->id() << " not found";
        }
    }
}
