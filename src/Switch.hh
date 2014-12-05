#pragma once

#include "Common.hh"
#include "Application.hh"
#include "Controller.hh"

class SwitchManager;

class Switch: public QObject {
Q_OBJECT
public:
    Switch(SwitchManager* mgr, OFConnection* conn, of13::FeaturesReply fr);
    ~Switch();

    std::string idstr() const;
    uint64_t id() const;
    uint32_t nbuffers() const;
    uint8_t  ntables() const;
    uint32_t capabilites() const;

    of13::Port port(uint32_t port_no) const;
    std::vector<of13::Port> ports() const;

    void send(OFMsg* msg);
    void requestPortDescriptions();

signals:
    void portUp(Switch* dp, of13::Port port);
    void portModified(Switch* dp, of13::Port port, of13::Port oldPort);
    void portDown(Switch* dp, uint32_t port_no);

    void up(Switch* dp);
    void down(Switch* dp);

protected:
    void portStatus(of13::PortStatus ps);
    void portDescArrived(of13::MultipartReplyPortDescription& mrpd);
    void setUp(OFConnection* conn, of13::FeaturesReply fr);
    void setDown();

private:
    friend class SwitchManager;
    struct SwitchImpl* m;
};

class SwitchManager: public Application  {
Q_OBJECT
SIMPLE_APPLICATION(SwitchManager, "switch-manager")
public:
    SwitchManager();
    ~SwitchManager();

    void init(Loader* provider, const Config& config) override;

    Switch* getSwitch(OFConnection* ofconn) const;
    Switch* getSwitch(uint64_t dpid);
    std::vector<Switch*> switches();

signals:
    void switchDiscovered(Switch* dp);

protected slots:
    void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);
    void onPortStatus(OFConnection* ofconn, of13::PortStatus ps);
    void onSwitchDown(OFConnection* ofconn);
    void onPortDescriptions(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> msg);

private:
    friend class Switch;
    struct SwitchManagerImpl* m;
};
