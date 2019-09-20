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

#include "Application.hpp"
#include "Loader.hpp"
#include "SwitchManager.hpp"
#include "RestListener.hpp"
#include "DpidChecker.hpp"
#include "Recovery.hpp"
#include "api/Statistics.hpp"
#include "json11.hpp"
#include "runos/core/logging.hpp"

#include <boost/lexical_cast.hpp>
#include <fluid/of13msg.hh>

#include <sstream>

namespace runos {

namespace of13 = fluid_msg::of13;

static std::string speedReadable(double bytes)
{
    double bits = bytes * 8;
    if (bits < 1000) {
        return std::to_string(bits) + " bps";
    }

    bits /= 1000;
    if (bits < 1000) {
        return std::to_string(bits) + " Kbps";
    }

    bits /= 1000;
    if (bits < 1000) {
        return std::to_string(bits) + " Mbps";
    }

    bits /= 1000;
    return std::to_string(bits) + " Gbps";
}

struct SwitchCollection : rest::resource {
    SwitchManager* app;
    DpidChecker* dpid_checker;

    explicit SwitchCollection(SwitchManager* app, DpidChecker* dpid_checker)
        : app(app), dpid_checker(dpid_checker)
    { }

    rest::ptree Get() const override {
        rest::ptree root;
        rest::ptree switches;

        for (const auto& sw : app->switches()) {
            rest::ptree spt;
            spt.put("dpid", sw->dpid());
            spt.put("role", dpid_checker->role(sw->dpid())._to_string());
            spt.put("peer", sw->connection()->peer_address());
            spt.put("aux_address", sw->aux_address());
            spt.put("hardware", sw->hardware());
            spt.put("connection-status",
                    sw->connection()->alive() ?  "up" : "down");

            switches.push_back(std::make_pair("", std::move(spt)));
        }

        root.add_child("array", switches);
        root.put("_size", switches.size());
        return root;
    }
};

struct SwitchRoles : rest::resource {
    DpidChecker* checker;
    RecoveryManager* recovery_manager;
    const SwitchRole switch_role;

    explicit SwitchRoles(DpidChecker* checker,
                         RecoveryManager* recovery_manager,
                         SwitchRole switch_role = SwitchRole::UNDEFINED)
        : checker(checker)
        , recovery_manager(recovery_manager)
        , switch_role(switch_role)
    { }

    rest::ptree Get() const override {
        rest::ptree switches;
        auto scan_roles = [] (const SwitchList& list,
                              const std::string& type,
                              rest::ptree& switches)
        {
            rest::ptree dpids;
            for (const auto& dpid : list) {
                rest::ptree sw;
                sw.put("", dpid);
                dpids.push_back(std::make_pair("", std::move(sw)));
            }
            switches.add_child(type, std::move(dpids));
        };

        if (+SwitchRole::AR == switch_role ||
                +SwitchRole::UNDEFINED == switch_role) {
            scan_roles(checker->switchList(SwitchRole::AR), "AR", switches);
        }
        if (+SwitchRole::DR == switch_role ||
                +SwitchRole::UNDEFINED == switch_role) {
            scan_roles(checker->switchList(SwitchRole::DR), "DR", switches);
        }
        return switches;
    }

    rest::ptree Put(rest::ptree const& pt) override {
        auto error_lambda = [](auto descr) -> rest::ptree
        {
            rest::ptree res;
            res.put("result", "error");
            res.put("message", "incorrect request");
            res.put("description", descr);
            return res;
        };

        // Block writing to db if it's not primary
        if (not recovery_manager->isPrimary()) {
            LOG(ERROR) << "[SwitchManager-REST] Error during writing to db. "
                          "The controller is not primary. "
                          "Writing to database is blocked";
            return error_lambda("The controller is NOT primary. "
                                "Writing request to DB is blocked");
        }

        if (+SwitchRole::UNDEFINED != switch_role) {
            return error_lambda("role is defined. To put extra "
                                "value you should undefine role.");
        }
        rest::ptree::const_assoc_iterator it = pt.find("dpid");
        if (pt.not_found() == it) {
            return error_lambda("didn't find necessary dpid in request");
        }
        const auto dpid = it->second.get_value<uint64_t>();
        if (checker->isRegistered(dpid)) {
            return error_lambda("To change switch role you must delete this "
                                "switch at first and then add with new role");
        }
        it = pt.find("role");
        if (pt.not_found() == it) {
            return error_lambda("didn't find necessary role in request");
        }
        const auto role_string = it->second.get_value<std::string>();

        rest::ptree res;
        auto add_switch_lambda = [=](SwitchRole role, rest::ptree& res)
        {
            checker->addSwitch(dpid, role);
            res.put("result", "done");
            res.put("message", "switch was added to topology");
            res.put("description", "switch with dpid=" +
                    std::to_string(dpid) +
                    " has been successfully added to topology with role " +
                    role._to_string());
        };

        if ("ar" == role_string || "AR" == role_string) {
            add_switch_lambda(SwitchRole::AR, res);
        } else if ("dr" == role_string || "DR" == role_string) {
            add_switch_lambda(SwitchRole::DR, res);
        } else {
            return error_lambda("role is not defined correctly"
                                "print either ar or dr.");
        }
        return res;
    }
};

struct SwitchCurrentRole : rest::resource {
    DpidChecker* dpid_checker;
    RecoveryManager* recovery_manager;
    const uint64_t dpid;

    explicit SwitchCurrentRole(DpidChecker* dpid_checker,
                               RecoveryManager* recovery_manager,
                               uint64_t dpid)
        : dpid_checker(dpid_checker)
        , recovery_manager(recovery_manager)
        , dpid(dpid)
    { }

    rest::ptree Get() const override {
        rest::ptree res;
        auto switch_role = dpid_checker->role(dpid);
        if (+SwitchRole::UNDEFINED == switch_role) {
            res.put("result", "error");
            res.put("message", "dpid not found");
            res.put("description", "dpid=" +
                    std::to_string(dpid) +
                    " is not registered in topology.");
        } else {
            res.put("dpid", dpid);
            res.put("role", switch_role._to_string());
        }
        return res;
    }

    rest::ptree Delete() override {
        rest::ptree res;
        if (not recovery_manager->isPrimary()) {
            rest::ptree res;
            res.put("result", "error");
            res.put("message", "incorrect request");
            res.put("description", "The controller is NOT primary. "
                                   "Writing request to DB is blocked");
            return res;
        }

        auto switch_role = dpid_checker->role(dpid);
        if (+SwitchRole::UNDEFINED == switch_role) {
            res.put("result", "error");
            res.put("message", "dpid not found");
            res.put("description", "dpid=" +
                    std::to_string(dpid) +
                    " was not registered in topology.");
        } else {
            dpid_checker->removeSwitch(dpid, switch_role);
            res.put("result", "done");
            res.put("message", "dpid deleted");
            res.put("description", "switch with dpid=" +
                    std::to_string(dpid) +
                    " and role=" +
                    switch_role._to_string() +
                    " has been deleted.");
        }
        return res;
    }
};

struct SwitchResource : rest::resource {
    UnsafeSwitchPtr sw;

    explicit SwitchResource(SwitchPtr sw)
        : sw(sw.not_null())
    { }

    rest::ptree Get() const override
    {
        rest::ptree pt;

        pt.put("dpid", sw->dpid());
        pt.put("peer", sw->connection()->peer_address());
        pt.put("aux_address", sw->aux_address());
        pt.put("ntables", sw->ntables());
        pt.put("nbuffers", sw->nbuffers());
        pt.put("manufacturer", sw->manufacturer());
        pt.put("hardware", sw->hardware());
        pt.put("serial-number", sw->serial_number());
        pt.put("description", sw->description());
        pt.put("connection-status",
               sw->connection()->alive() ?  "up" : "down");
        pt.put("miss-send-len", sw->miss_send_len());

        rest::ptree ports;
        for (const auto& port : sw->ports()) {
            if (sw->property("local_port", of13::OFPP_LOCAL) == port->number())
                continue;
            rest::ptree st;
            st.put("", port->number());
            ports.push_back(std::make_pair("", st));
        }
        pt.add_child("ports", ports);

        std::ostringstream scap;

        auto caps = sw->capabilites();
        rest::ptree stats;
        if (caps & of13::OFPC_FLOW_STATS)
            stats.push_back(std::make_pair("",
                            rest::ptree().put("", "flow")));
        if (caps & of13::OFPC_TABLE_STATS)
            stats.push_back(std::make_pair("",
                            rest::ptree().put("", "table")));
        if (caps & of13::OFPC_PORT_STATS)
            stats.push_back(std::make_pair("",
                            rest::ptree().put("", "port")));
        if (caps & of13::OFPC_GROUP_STATS)
            stats.push_back(std::make_pair("",
                            rest::ptree().put("", "group")));
        if (caps & of13::OFPC_QUEUE_STATS)
            stats.push_back(std::make_pair("",
                            rest::ptree().put("", "queue")));
        pt.add_child("capabilities.stats", stats);

        pt.put("capabilities.ip-reassembly",
                !!(caps & of13::OFPC_IP_REASM));
        pt.put("capabilities.looped-port-blocking",
                !!(caps & of13::OFPC_PORT_BLOCKED));

        return pt;
    }

    rest::ptree Put(rest::ptree const& pt) override
    {
        rest::ptree ret;
        rest::ptree::const_assoc_iterator it;

        auto mod = sw->modify();

        it = pt.find("miss-send-len");
        if (it != pt.not_found()) {
            auto len = it->second.get_value<unsigned>();
            mod->miss_send_len( len );
            ret.add_child(it->first, it->second);
        }

        it = pt.find("fragment-policy");
        if (it != pt.not_found()) {
            auto policy = it->second.get_value<unsigned>();
            mod->fragment_policy( policy );
            ret.add_child(it->first, it->second);
        }

        return ret;
    }
};

struct SwitchMaintenanceResource : rest::resource {
    UnsafeSwitchPtr sw;

    explicit SwitchMaintenanceResource(SwitchPtr sw)
        : sw(sw.not_null())
    { }

    rest::ptree Get() const override
    {
        rest::ptree ret;
        ret.put("switch", sw->dpid());
        ret.put("maintenance", sw->maintenance());

        return ret;
    }

    rest::ptree Put(const rest::ptree& pt) override
    {
        try {
            auto val = pt.get<bool>("maintenance");
            sw->set_maintenance(val);
        } catch (const boost::property_tree::ptree_bad_data& ex) {
            THROW(rest::http_error(400), "incorrect request JSON");
        }

        return pt;
    }
};

struct PortResource : rest::resource {
    UnsafePortPtr port;

    explicit PortResource(PortPtr port)
        : port(port.not_null())
    { }

    rest::ptree Get() const override
    {
        rest::ptree pt;

        pt.put("number", port->number());
        pt.put("hw-addr", boost::lexical_cast<std::string>(port->hw_addr()));
        pt.put("name", port->name());

        pt.put("config.port-down", port->disabled());
        pt.put("config.no-recv", port->no_receive());
        pt.put("config.no-fwd", port->no_forward());
        pt.put("config.no-packet-in", port->no_packet_in());

        pt.put("link.status", port->link_down() ? "down" : "up");
        if (not port->link_down()) {
            pt.put("link.blocked", port->blocked());
            pt.put("link.live", port->live());
        }

        rest::ptree queues;
        for (uint32_t queue_id : port->queues()) {
            rest::ptree st;
            st.put("", queue_id);
            queues.push_back(std::make_pair("", st));
        }
        pt.add_child("queues", queues);

        if (auto port = std::dynamic_pointer_cast<EthernetPort>(this->port)) {
            pt.put("ethernet.curr-speed", port->current_speed());
            pt.put("ethernet.max-speed", port->max_speed());

            auto curr = port->current();
            auto adv = port->advertised();
            auto peer = port->peer();
            auto supp = port->supported();
            supp |= curr; // workaround ovs bug
            auto& feat_pt = pt.put_child("ethernet.features", rest::ptree());

            auto eth_feat_to_string = [&](const char* name, uint32_t feat) {
                if (supp & feat) {
                    std::string ret;
                    static const std::string sep = ",";

                    if (curr & feat)
                        ret += "current";
                    if (adv & feat)
                        ret += (ret.empty() ? "" : sep) + "advertised";
                    if (peer & feat)
                        ret += (ret.empty() ? "" : sep) + "peer";

                    feat_pt.put(name, ret);
                }
            };

            eth_feat_to_string("10Mb-half-duplex", of13::OFPPF_10MB_HD);
            eth_feat_to_string("10Mb-full-duplex", of13::OFPPF_10MB_FD);
            eth_feat_to_string("100Mb-half-duplex", of13::OFPPF_100MB_HD);
            eth_feat_to_string("100Mb-full-duplex", of13::OFPPF_100MB_FD);
            eth_feat_to_string("1Gb-half-duplex", of13::OFPPF_1GB_HD);
            eth_feat_to_string("1Gb-full-duplex", of13::OFPPF_1GB_FD);
            eth_feat_to_string("10Gb-full-duplex", of13::OFPPF_10GB_FD);
            eth_feat_to_string("40Gb-full-duplex", of13::OFPPF_40GB_FD);
            eth_feat_to_string("100Gb-full-duplex", of13::OFPPF_100GB_FD);
            eth_feat_to_string("1Tb-full-duplex", of13::OFPPF_1TB_FD);
            eth_feat_to_string("custom-rate", of13::OFPPF_OTHER);

            eth_feat_to_string("auto-negotiation", of13::OFPPF_AUTONEG);
            eth_feat_to_string("pause", of13::OFPPF_PAUSE);
            eth_feat_to_string("assymmetric-pause", of13::OFPPF_PAUSE);

            if (curr & of13::OFPPF_COPPER) {
                pt.add("ethernet.medium", "copper");
            }
            if (curr & of13::OFPPF_FIBER) {
                pt.add("ethernet.medium", "fiber");
            }
        }

        return pt;
    }

    rest::ptree Put(const rest::ptree& pt) override
    {
        auto hw_addr = port->hw_addr();
        THROW_IF(hw_addr != pt.get("hw-addr", hw_addr), rest::http_error(409),
                 "hw_addr in request doesn't match");

        auto mod = port->modify();
        rest::ptree::const_assoc_iterator it;
        rest::ptree ret;

        auto config = pt.get_child("config");
        auto& ret_config = ret.add_child("config", rest::ptree());

        it = config.find("no-fwd");
        if (it != config.not_found()) {
            mod->no_forward( it->second.get_value<bool>() );
            ret_config.add_child(it->first, it->second);
        }

        it = config.find("no-packet-in");
        if (it != config.not_found()) {
            mod->no_packet_in( it->second.get_value<bool>() );
            ret_config.add_child(it->first, it->second);
        }

        it = config.find("no-recv");
        if (it != config.not_found()) {
            mod->no_receive( it->second.get_value<bool>() );
            ret_config.add_child(it->first, it->second);
        }

        it = config.find("port-down");
        if (it != config.not_found()) {
            mod->disable( it->second.get_value<bool>() );
            ret_config.add_child(it->first, it->second);
        }

        // TODO: port features

        return ret;
    }
};

struct PortStatsResource : rest::resource {
    UnsafePortPtr port;

    explicit PortStatsResource(PortPtr port)
        : port(port.not_null())
    { }

    rest::ptree Get() const override
    {
        //using std::chrono::duration_cast;
        //using std::chrono::duration;

        rest::ptree ret;
        auto stats = port->stats();

        //ret.put("uptime", duration_cast<duration<double>>(stats.uptime())
        //                 .count());

        auto& speed = stats.current_speed;
        auto& max = stats.max_speed;
        auto& integral = stats.integral;

        ret.put("current-speed.rx-packets", speed.rx_packets());
        ret.put("current-speed.tx-packets", speed.tx_packets());
        ret.put("current-speed.rx-bytes", speed.rx_bytes());
        ret.put("current-speed.tx-bytes", speed.tx_bytes());
        ret.put("current-speed.rx-dropped", speed.rx_dropped());
        ret.put("current-speed.tx-dropped", speed.tx_dropped());
        ret.put("current-speed.rx-errors", speed.rx_errors());
        ret.put("current-speed.tx-errors", speed.tx_errors());
        ret.put("current-speed.rx-speed", speedReadable(speed.rx_bytes()));
        ret.put("current-speed.tx-speed", speedReadable(speed.tx_bytes()));

        ret.put("integral.rx-packets", integral.rx_packets());
        ret.put("integral.tx-packets", integral.tx_packets());
        ret.put("integral.rx-bytes", integral.rx_bytes());
        ret.put("integral.tx-bytes", integral.tx_bytes());
        ret.put("integral.rx-dropped", integral.rx_dropped());
        ret.put("integral.tx-dropped", integral.tx_dropped());
        ret.put("integral.rx-errors", integral.rx_errors());
        ret.put("integral.tx-errors", integral.tx_errors());
        ret.put("integral.rx-speed", speedReadable(integral.rx_bytes()));
        ret.put("integral.tx-speed", speedReadable(integral.tx_bytes()));

        ret.put("max.rx-packets", max.rx_packets());
        ret.put("max.tx-packets", max.tx_packets());
        ret.put("max.rx-bytes", max.rx_bytes());
        ret.put("max.tx-bytes", max.tx_bytes());
        ret.put("max.rx-dropped", max.rx_dropped());
        ret.put("max.tx-dropped", max.tx_dropped());
        ret.put("max.rx-errors", max.rx_errors());
        ret.put("max.tx-errors", max.tx_errors());
        ret.put("max.rx-speed", speedReadable(max.rx_bytes()));
        ret.put("max.tx-speed", speedReadable(max.tx_bytes()));

        return ret;
    }
};

struct PortTrafficStatsResource : rest::resource {
    UnsafePortPtr port;
    ForwardingType f_type;

    explicit PortTrafficStatsResource(PortPtr port, ForwardingType type)
        : port(port.not_null())
        , f_type(type)
    { }

    rest::ptree Get() const override
    {
        if (f_type != (+ForwardingType::None)) {
            return trafficStats(f_type);
        }

        rest::ptree ret;

        auto unicast = trafficStats(ForwardingType::Unicast);
        auto broadcast = trafficStats(ForwardingType::Broadcast);
        auto multicast = trafficStats(ForwardingType::Multicast);

        ret.add_child("unicast", unicast);
        ret.add_child("broadcast", broadcast);
        ret.add_child("multicast", multicast);

        return ret;
    }

    rest::ptree trafficStats(ForwardingType type) const
    {
        rest::ptree ret;
        auto stats = port->traffic_stats(type);

        auto& speed = stats.current_speed;
        auto& max = stats.max_speed;
        auto& integral = stats.integral;

        ret.put("current-speed.rx-packets", speed.rx_packets());
        ret.put("current-speed.rx-bytes", speed.rx_bytes());

        ret.put("integral.rx-packets", integral.rx_packets());
        ret.put("integral.rx-bytes", integral.rx_bytes());

        ret.put("max.rx-packets", max.rx_packets());
        ret.put("max.rx-bytes", max.rx_bytes());

        return ret;
    }
};

/*!
 * \brief The SwitchCollectionStats struct returns statistics about Data Plane.
 */
struct SwitchCollectionStats : rest::resource {
    SwitchManager* app;

    explicit SwitchCollectionStats(SwitchManager* app)
        : app(app)
    { }

    rest::ptree Get() const override {
        rest::ptree root;
        rest::ptree stats;

        for (const auto& sw : app->switches()) {
            for (const auto& port : sw->ports()) {
                if (port->number() > of13::OFPP_MAX) continue;
                if (port->link_down()) continue;

                rest::ptree spt;
                spt.put("dpid", sw->dpid());
                spt.put("port", port->number());
                spt.add_child("stats", PortStatsResource{port}.Get());
                stats.push_back(std::make_pair("", std::move(spt)));
            }
        }

        root.add_child("array", stats);
        root.put("_size", stats.size());
        return root;
    }
};

/*!
 * \brief The SwitchCollectionControlStats struct returns statistics about Control Plane.
 */
struct SwitchCollectionControlStats : rest::resource {
    SwitchManager* app;

    explicit SwitchCollectionControlStats(SwitchManager* app)
        : app(app)
    { }

    rest::ptree Get() const override {
        rest::ptree root;
        rest::ptree switches;

        std::chrono::system_clock::time_point end_conn = std::chrono::system_clock::now();
        uint64_t duration;

        for (const auto& sw : app->switches()) {
            rest::ptree spt;
            std::chrono::system_clock::time_point start_conn = sw->connection()->get_start_time();
            duration = std::chrono::duration_cast<std::chrono::seconds> (end_conn - start_conn).count();

            spt.put("dpid", sw->dpid());
            spt.put("peer", sw->connection()->peer_address());
            spt.put("connection-status", sw->connection()->alive() ?  "up" : "down");
            spt.put("uptime", sw->connection()->alive() ?  duration : 0 );
            spt.put("rx_ofpackets", sw->connection()->get_rx_packets());
            spt.put("tx_ofpackets", sw->connection()->get_tx_packets());
            spt.put("pkt_in_ofpackets", sw->connection()->get_pkt_in_packets());

            switches.push_back(std::make_pair("", std::move(spt)));
        }

        root.add_child("control_stats", switches);
        root.put("_size", switches.size());
        return root;
    }
};

struct PortMaintenanceResource : rest::resource {
    UnsafePortPtr port;

    explicit PortMaintenanceResource(PortPtr port)
        : port(port.not_null())
    { }

    rest::ptree Get() const override
    {
        rest::ptree ret;
        ret.put("switch", port->switch_()->dpid());
        ret.put("port", port->number());
        ret.put("maintenance", port->maintenance());

        return ret;
    }

    rest::ptree Put(const rest::ptree& pt) override
    {
        try {
            auto val = pt.get<bool>("maintenance");
            port->set_maintenance(val);
        } catch (const boost::property_tree::ptree_bad_data& ex) {
            THROW(rest::http_error(400), "incorrect request JSON");
        }

        return pt;
    }
};

struct QueueStatsResource : rest::resource {
    UnsafePortPtr port;
    uint32_t queue_id;

    explicit QueueStatsResource(PortPtr port, uint32_t queue_id)
        : port(port.not_null()), queue_id(queue_id)
    { }

    rest::ptree Get() const override
    {
        //using std::chrono::duration_cast;
        //using std::chrono::duration;

        rest::ptree ret;
        auto stats = port->queue_stats(queue_id);

        //ret.put("uptime", duration_cast<duration<double>>(stats.uptime())
        //                 .count());

        auto& speed = stats.current_speed;
        auto& max = stats.max_speed;
        auto& integral = stats.integral;

        ret.put("current-speed.tx-packets", speed.tx_packets());
        ret.put("current-speed.tx-bytes", speed.tx_bytes());
        ret.put("current-speed.tx-bits", speed.tx_bytes() / 8);
        ret.put("current-speed.tx-speed", speedReadable(speed.tx_bytes()));
        ret.put("current-speed.tx-errors", speed.tx_errors());

        ret.put("integral.tx-packets", integral.tx_packets());
        ret.put("integral.tx-bytes", integral.tx_bytes());
        ret.put("integral.tx-bits", integral.tx_bytes() / 8);
        ret.put("integral.tx-speed", speedReadable(integral.tx_bytes()));
        ret.put("integral.tx-errors", integral.tx_errors());

        ret.put("max.tx-packets", max.tx_packets());
        ret.put("max.tx-bytes", max.tx_bytes());
        ret.put("max.tx-bits", max.tx_bytes() / 8);
        ret.put("max.tx-speed", speedReadable(max.tx_bytes()));
        ret.put("max.tx-errors", max.tx_errors());

        return ret;
    }
};

class SwitchManagerRest : public Application
{
    SIMPLE_APPLICATION(SwitchManagerRest, "switch-manager-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = SwitchManager::get(loader);
        auto rest_ = RestListener::get(loader);
        auto checker = DpidChecker::get(loader);
        auto recovery_manager = RecoveryManager::get(loader);

        rest_->mount(path_spec("/switches/"), [=](const path_match&)
        {
            return SwitchCollection {app, checker};
        });

        rest_->mount(path_spec("/switches/role/"), [=](const path_match&)
        {
            return SwitchRoles {checker, recovery_manager};
        });

        rest_->mount(path_spec("/switches/role/ar/"), [=](const path_match&)
        {
            return SwitchRoles {checker, recovery_manager, SwitchRole::AR};
        });

        rest_->mount(path_spec("/switches/role/dr/"), [=](const path_match&)
        {
            return SwitchRoles {checker, recovery_manager, SwitchRole::DR};
        });

        rest_->mount(path_spec("/switches/role/(\\d+)/"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return SwitchCurrentRole {checker, recovery_manager, dpid};
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Switch not found" );
            }
        });

        rest_->mount(path_spec("/switches/ports/stats/"),
                     [=](const path_match&)
        {
            return SwitchCollectionStats {app};
        });

        rest_->mount(path_spec("/switches/(\\d+)/"), [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return SwitchResource { app->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Switch not found" );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/ports/(\\d+)/"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                return PortResource{ app->switch_(dpid)->port(port_no) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Port not found" );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/maintenance/"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                return SwitchMaintenanceResource{ app->switch_(dpid) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Switch not found" );
            }
        });

        // control stats for switches
        rest_->mount(path_spec("/switches/controlstats/"),
                     [=](const path_match& m)
        {
            try {
                return SwitchCollectionControlStats {app};
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Control stats for switches are not found" );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/ports/(\\d+)/stats/"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                return PortStatsResource{ app->switch_(dpid)->port(port_no) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Port or switch not found" );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/ports/(\\d+)/traffic_stats/"
                     "(?:(multicast|broadcast|unicast)/)?"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                auto f_type_str = m[3].str();
                f_type_str = f_type_str.empty() ? "none" : f_type_str;
                f_type_str[0] = std::toupper(f_type_str[0]);
                auto f_type = ForwardingType::_from_string(f_type_str.c_str());

                return PortTrafficStatsResource{ app->switch_(dpid)->port(port_no),
                                                 f_type };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                 THROW( rest::http_error(404), "Port or switch not found" );
            }
        });

        rest_->mount(path_spec("/switches/(\\d+)/ports/(\\d+)/maintenance/"),
                     [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                return PortMaintenanceResource{
                    app->switch_(dpid)->port(port_no)
                };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Port or switch not found" );
            }
        });

        rest_->mount(path_spec(
                         "/switches/(\\d+)/ports/(\\d+)/queues/(\\d+)/stats/"),
        [=](const path_match& m)
        {
            try {
                auto dpid = boost::lexical_cast<uint64_t>(m[1].str());
                auto port_no = boost::lexical_cast<uint32_t>(m[2].str());
                auto queue_id = boost::lexical_cast<uint32_t>(m[3].str());
                return QueueStatsResource{ app->switch_(dpid)->port(port_no),
                                           queue_id };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() );
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Not found" );
            }
        });
    }
};

REGISTER_APPLICATION(SwitchManagerRest, {"rest-listener", "switch-manager",
                                         "dpid-checker", "recovery-manager",
                                         ""})

}
