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

#include "StatsRulesManager.hpp"

namespace runos {

REGISTER_APPLICATION(StatsRulesManager, {"stats-bucket-manager", ""})

class RulesCreator {
public:
    explicit RulesCreator(const PortPtr& port)
        : dpid_(port->switch_()->dpid())
        , in_port_(port->number())
        , stag_(0)
        , installation_table_(port->switch_()->tables.statistics)
        , next_table_(port->switch_()->tables.admission)
        , matches_{ make_broadcast_match(),
                    make_multicast_match(),
                    make_unicast_match() }
    { }

    explicit RulesCreator(const PortPtr& port, uint16_t stag)
        : dpid_(port->switch_()->dpid())
        , in_port_(port->number())
        , stag_(stag)
        , installation_table_(port->switch_()->tables.ep_statistics)
        , next_table_(port->switch_()->tables.admission)
        , matches_{ make_broadcast_match(),
                    make_multicast_match(),
                    make_unicast_match() }
    {
        if (stag_ != 0) {
            add_stag_field();
        }
    }

    BucketsMap makeBuckets(StatsBucketManager* mgr)
    {
        BucketsMap new_buckets;
        auto names = bucketsNames();
        for (size_t i = 0; i < names.size(); ++i) {
            auto bucket = mgr->aggregateFlows(
                std::chrono::seconds(1),
                names[i],
                flow_selector::dpid = {dpid_},
                flow_selector::table = installation_table_,
                flow_selector::match = matches_[i]
            );
        }
        return new_buckets;
    }

    BucketsNames bucketsNames()
    {
        BucketsNames names;
        names.push_back(bucket_name(ForwardingType::Broadcast));
        names.push_back(bucket_name(ForwardingType::Multicast));
        names.push_back(bucket_name(ForwardingType::Unicast));
        return names;
    }

    FlowModPtrSequence makeInstallRules() const
    {
        FlowModPtrSequence rules;
        auto go_to_admission_table = of13::GoToTable(next_table_);

        for (auto& match: matches_) {
            auto rule = make_flow_mod(add_command, match);
            rule->add_instruction(go_to_admission_table);
            rules.emplace_back(std::move(rule));
        }

        return rules;
    }

    FlowModPtrSequence makeClearRules() const
    {
        FlowModPtrSequence rules;
        for (auto& match: matches_) {
            auto rule = make_flow_mod(delete_command, match);
            rules.emplace_back(std::move(rule));
        }
        return rules;
    }

    static constexpr uint64_t cookie = 0x1500;

private:
    uint64_t dpid_;
    uint32_t in_port_;
    uint16_t stag_;
    uint8_t installation_table_;
    uint8_t next_table_;
    std::vector<of13::Match> matches_;

    FlowModPtr make_flow_mod(uint8_t command, const of13::Match& match) const
    {
        auto fmp = std::make_shared<of13::FlowMod>();
        fmp->cookie(cookie);
        fmp->command(command);
        fmp->table_id(installation_table_);
        fmp->match(match);
        return fmp;
    }

    of13::Match make_unicast_match() const
    {
        of13::Match match;
        auto oxm_in_port = of13::InPort(in_port_);
        match.add_oxm_field(oxm_in_port);
        return match;
    }

    of13::Match make_broadcast_match() const
    {
        auto match = make_unicast_match();
        auto oxm_eth_dst = of13::EthDst(broadcast_mac);
        match.add_oxm_field(oxm_eth_dst);
        return match;
    }

    of13::Match make_multicast_match() const
    {
        auto match = make_unicast_match();
        auto oxm_eth_dst = of13::EthDst(multicast_mac, multicast_mac_mask);
        match.add_oxm_field(oxm_eth_dst);
        return match;
    }

    void add_stag_field()
    {
        auto stag_field = of13::VLANVid(stag_);
        for (auto& match: matches_) {
            match.add_oxm_field(stag_field);
        }
    }

    std::string bucket_name(ForwardingType ftype) const
    {
        return std::string("traffic") + "-" +
               ftype._to_string() + "_" +
               std::to_string(dpid_) + "_" +
               std::to_string(in_port_) + "_" +
               std::to_string(stag_);
    }

    static constexpr uint8_t add_command = of13::OFPFC_ADD;
    static constexpr uint8_t delete_command = of13::OFPFC_DELETE;
    static constexpr auto broadcast_mac = "ff:ff:ff:ff:ff:ff";
    static constexpr auto multicast_mac = "01:00:00:00:00:00";
    static constexpr auto multicast_mac_mask = "ff:00:00:00:00:00";
};

void StatsRulesManager::init(Loader* loader, const Config&)
{
    bucket_mgr_ = StatsBucketManager::get(loader);
}

BucketsMap StatsRulesManager::installEndpointRules(PortPtr port,
                                                   uint16_t stag)
{
    auto sw = port->switch_();
    if (sw->tables.ep_statistics == Switch::Tables::no_table) {
        return BucketsMap();
    }
    RulesCreator creator(port, stag);
    for (auto& rule: creator.makeInstallRules()) {
        sw->connection()->send(*rule);
    }
    return creator.makeBuckets(bucket_mgr_);
}

BucketsNames StatsRulesManager::deleteEndpointRules(PortPtr port,
                                                    uint16_t stag)
{
    auto sw = port->switch_();
    if (sw->tables.ep_statistics == Switch::Tables::no_table) {
        return BucketsNames();
    }
    RulesCreator creator(port, stag);
    for (auto& rule: creator.makeClearRules()) {
        sw->connection()->send(*rule);
    }
    return creator.bucketsNames();
}

void StatsRulesManager::installRules(PortPtr new_port)
{
    auto sw = new_port->switch_();
    if (sw->property("local_port", of13::OFPP_LOCAL) == new_port->number() ||
        sw->tables.statistics == Switch::Tables::no_table) {
        return;
    }
    RulesCreator creator(new_port);
    for (auto& rule: creator.makeInstallRules()) {
        sw->connection()->send(*rule);
    }
}

void StatsRulesManager::deleteRules(PortPtr down_port)
{
    auto sw = down_port->switch_();
    if (sw->property("local_port", of13::OFPP_LOCAL) == down_port->number() ||
        sw->tables.statistics == Switch::Tables::no_table) {
        return;
    }
    RulesCreator creator(down_port);
    for (auto& rule: creator.makeClearRules()) {
        sw->connection()->send(*rule);
    }
}

void StatsRulesManager::clearStatsTable(SwitchPtr sw)
{
    if (sw->tables.statistics == Switch::Tables::no_table) {
        return;
    }

    of13::FlowMod cl;
    cl.command(of13::OFPFC_DELETE);
    cl.cookie(RulesCreator::cookie);
    cl.table_id(sw->tables.statistics);
    sw->connection()->send(cl);
}

} // namespace runos
