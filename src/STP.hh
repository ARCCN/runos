#pragma once

#include <QTimer>

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"

#include "ILinkDiscovery.hh"
#include "LinkDiscovery.hh"
#include "Switch.hh"
#include "Topology.hh"

#define POLL_TIMEOUT 1

struct Port {
    uint32_t port_no;
    // this port is broadcast or not
    bool broadcast;
    // it means that port is between two switches
    bool to_switch;
    class SwitchSTP* nextSwitch;

    Port(uint32_t _port_no, bool _b = true): port_no(_port_no), broadcast(_b), to_switch(false), nextSwitch(NULL) {}
};

class STP;

class SwitchSTP : public QObject {
    Q_OBJECT
public:
    Switch* sw;
    STP* parent;
    bool root;
    bool computed;
    SwitchSTP* nextSwitchToRoot;
    QTimer* timer;
    std::unordered_map<uint32_t, Port*> ports;

    SwitchSTP() = delete;
    SwitchSTP(Switch* _sw, STP* stp, bool _root = false, bool _comp = false):
        sw(_sw),
        parent(stp),
        root(_root),
        computed(_comp),
        nextSwitchToRoot(NULL),
        timer(new QTimer(this)) {}

    bool existsPort(uint32_t port_no) { return (ports.count(port_no) > 0 ? true : false); }
    void unsetBroadcast(uint32_t port_no)  { ports.at(port_no)->broadcast = false; }
    void setBroadcast(uint32_t port_no)    { ports.at(port_no)->broadcast = true;  }
    void setSwitchPort(uint32_t port_no, uint64_t dpid);
    void resetBroadcast();
protected slots:
    void computeSTP();
};

class STP : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(STP, "stp")
public:
    void init(Loader* loader, const Config& config) override;

    std::vector<uint32_t> getSTP(uint64_t dpid);

protected slots:
    void onLinkDiscovered(switch_and_port from, switch_and_port to);
    void onLinkBroken(switch_and_port from, switch_and_port to);
    void onSwitchDiscovered(Switch* dp);
    void onSwitchDown(Switch* dp);
    void onPortUp(Switch* dp, of13::Port port);

private:
    std::unordered_map<uint64_t, SwitchSTP*> switch_list;
    Topology* topo;

    SwitchSTP* findRoot();
    void computePathForSwitch(uint64_t dpid);

    friend class SwitchSTP;
};
