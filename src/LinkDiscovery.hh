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

#pragma once

#include <set>
#include <unordered_map>
#include <chrono>

#include <QTimer>

#include "Common.hh"
#include "Switch.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "ILinkDiscovery.hh"

struct Link {
    typedef std::chrono::time_point<std::chrono::steady_clock>
            valid_through_t;

    switch_and_port source;
    switch_and_port target;
    valid_through_t valid_through;

    Link(const switch_and_port&, const switch_and_port&, const valid_through_t&);
};

class LinkDiscovery : public Application, public ILinkDiscovery, private OFMessageHandlerFactory {
    Q_OBJECT
    APPLICATION(LinkDiscovery, ILinkDiscovery)
public:
    void init(Loader* provider, const Config& config) override;
    void startUp(Loader* provider) override;

    std::string orderingName() const override;
    OFMessageHandler* makeOFMessageHandler() override;
    bool isPostreq(const std::string &name) const override;

signals:
    void linkDiscovered(switch_and_port from, switch_and_port to);
    void linkBroken(switch_and_port from, switch_and_port to);
    void lldpReceived(switch_and_port from, switch_and_port to);

public slots:
    void portUp(Switch* dp, of13::Port port);
    void portModified(Switch* dp, of13::Port port, of13::Port old_port);
    void portDown(Switch* dp, uint32_t port_no);

protected slots:
    void onLldpReceived(switch_and_port from, switch_and_port to);
    void pollTimeout();

private:
    class Handler: public OFMessageHandler {
        LinkDiscovery* app;
    public:
        Handler(LinkDiscovery* ld) : app(ld) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };

    unsigned c_poll_interval;
    SwitchManager* m_switch_manager;
    QTimer* m_timer;

    std::set<Link> m_links;
    std::unordered_map<switch_and_port, std::set<Link>::iterator >
                   m_out_edges;

    void sendLLDP(Switch *dp, of13::Port port);
    void clearLinkAt(const switch_and_port & ap);
};

inline bool operator<(const Link& a, const Link& b) {
    return a.valid_through < b.valid_through;
}


