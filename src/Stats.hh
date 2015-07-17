#pragma once

#include <QTimer>
#include <unordered_map>

#include "Common.hh"
#include "Switch.hh"
#include "Application.hh"
#include "Controller.hh"
#include "Loader.hh"
#include "Rest.hh"
#include "RestListener.hh"
#include "AppObject.hh"

struct port_packets_bytes : public AppObject {
    uint32_t port_no;
    uint64_t tx_packets;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;

    uint64_t tx_byte_speed;
    uint64_t rx_byte_speed;

    port_packets_bytes(uint32_t pn, uint64_t tp, uint64_t rp, uint64_t tb, uint64_t rb);
    port_packets_bytes();

    uint64_t id() const override;
    json11::Json to_json() const override;
};

class SwitchPortStats {
private:
    // switch id
    Switch* sw;
    // stats for each port
    std::unordered_map<int, port_packets_bytes> port_stats;

public:
    void insertElem(std::pair<int, port_packets_bytes> elem) { port_stats.insert(elem); }
    port_packets_bytes& getElem(int key) { return port_stats.at(key); }
    std::vector<port_packets_bytes> getPPB_vec();

    SwitchPortStats(Switch* _sw);
    SwitchPortStats();

    friend class SwitchStats;
    friend class SwitchStatsRest;
};

class SwitchStatsRest : public Rest {
    Q_OBJECT
    class SwitchStats* app;
public:
    SwitchStatsRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;

    friend class SwitchStats;
};

/**
* An application which gathers port statistics from all known switches every n seconds
*/
class SwitchStats: public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(SwitchStats, "switch-stats")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* provider) override;

public slots:
    /**
    * Called when a switch has answered with MulipartReplyPortStats message
    */ 
    void portStatsArrived(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> reply);
    void newSwitch(Switch* sw);

private slots:
    void pollTimeout();

private:
    unsigned c_poll_interval;
    QTimer* m_timer;
    SwitchManager* m_switch_manager;
    // port stats for each switch
    std::unordered_map<uint64_t, SwitchPortStats> switch_stats;
    OFTransaction* pdescr;
    SwitchStatsRest* r;

    friend class SwitchStatsRest;
};

