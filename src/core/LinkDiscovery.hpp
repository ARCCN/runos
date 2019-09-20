/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include "Application.hpp"
#include "SwitchOrdering.hpp"
#include "Loader.hpp"
#include "ILinkDiscovery.hpp"
#include "Controller.hpp" // OFMessageHandlerPtr

#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility> // std::pair

namespace runos {

inline bool operator<(const DiscoveredLink& a, const DiscoveredLink& b) {
    return std::tie(a.valid_through, a.source, a.target) < std::tie(b.valid_through, b.source, b.target);
}

class LinkDiscovery : public Application
                    , public SwitchEventHandler
                    , public ILinkDiscovery
{
    Q_OBJECT
    APPLICATION(LinkDiscovery, runos::ILinkDiscovery)
public:
    ~LinkDiscovery();

    void init(Loader* provider, const Config& config) override;
    void startUp(Loader* provider) override;

    const std::set<DiscoveredLink> links() const override
    {
        return m_links;
    }

    switch_and_port other(switch_and_port sp) const override;

    unsigned int pollInterval(void) const { return c_poll_interval; }
    int outputQueueId(void) const { return queue_id; }

signals:
    void linkDiscovered(switch_and_port from, switch_and_port to) override;
    void linkBroken(switch_and_port from, switch_and_port to) override;

private slots:
    void clearLinkAt(PortPtr port);
    void polling();

private:
    unsigned c_poll_interval;
    int queue_id;
    class RecoveryManager* recovery;
    class SwitchManager* m_switch_manager;
    class DatabaseConnector* db_connector_;
    OFMessageHandlerPtr handler;

    using links_set = std::set<DiscoveredLink>;
    using links_set_iterator = links_set::iterator;
    using link_pair = std::pair<switch_and_port, switch_and_port>;

    links_set m_links;
    links_set m_waiting_links;
    std::unordered_map<switch_and_port, links_set_iterator >
            m_out_edges;
    mutable std::mutex links_mutex; //protect link containers

    class Poller* poller; //run sending lldp timer from separate threads

    void handleBeacon(switch_and_port from, switch_and_port to);

    // thread-unsafe functions, lock links_mutex before calling
    links_set_iterator find_link(const links_set& set, const DiscoveredLink& link) const;
    bool add_link(DiscoveredLink& link);
    link_pair remove_link(switch_and_port from, switch_and_port to);
    std::optional<link_pair> remove_broken_link(switch_and_port from);
    void add_waiting_link(DiscoveredLink link);
    void remove_waiting_link(switch_and_port from, switch_and_port to);

    void switchUp(SwitchPtr sw) override;
    void linkUp(PortPtr port) override;
    void linkDown(PortPtr port) override;

    void save_to_database();
    void load_from_database();
};

} // namespace runos
