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

#include <runos/core/exception.hpp>
#include <runos/core/future-decl.hpp>

#include <fluid/ofcommon/common.hh>
#include <fluid/of13msg.hh>

#include "OFAgentFwd.hpp"

#include <vector>

namespace runos {

namespace of13 = fluid_msg::of13;

namespace ofp {

struct aggregate_stats {
    uint64_t packets {0};
    uint64_t bytes {0};
    uint32_t flows {0};
};

struct flow_stats_request {
    // Defaults to "Match All"
    uint8_t table_id {of13::OFPTT_ALL};
    uint32_t out_port {of13::OFPP_ANY};
    uint32_t out_group {of13::OFPG_ANY};
    uint64_t cookie {0};
    uint64_t cookie_mask {0};
    of13::Match match;
};

struct switch_config {
    uint16_t flags;
    uint16_t miss_send_len;
};

struct role_config {
    uint32_t role;
    uint64_t generation_id;
};

} // namespace ofp

class OFAgent {
public:
    template<class T>
    using sequence = std::vector<T>;

    struct error : exception_root {
        explicit error(uint64_t dpid, uint32_t xid) noexcept
            : dpid_(dpid)
            , xid_(xid)
        {
            with("dpid", dpid_, "{:#x}");
            with("xid", xid_);
        }

        uint64_t dpid() const noexcept { return dpid_; }
        uint32_t xid() const noexcept { return xid_; }

    protected:
        uint64_t dpid_;
        uint32_t xid_;
    };
    
    struct openflow_error : error {
        explicit openflow_error(uint64_t dpid, uint32_t xid,
                                uint16_t type, uint16_t code) noexcept
            : error(dpid, xid)
            , type_(type)
            , code_(code)
        {
            with("type", type_);
            with("code", code_);
        }

        uint16_t type() const noexcept { return type_; }
        uint16_t code() const noexcept { return code_; }
    protected:
        uint16_t type_;
        uint16_t code_;
    };

    struct bad_reply : error {
        using error::error;
    };

    struct not_responded : error {
        using error::error;
    };

    struct request_error : error {
        using error::error;
    };

    // Synchronization
    virtual future< void >
        barrier() = 0;

    // Switch Config
    virtual future< ofp::switch_config >
        request_config() = 0;
    virtual future< void >
        set_config(ofp::switch_config config) = 0;

    // Switch Description
    virtual future< fluid_msg::SwitchDesc >
        request_switch_desc() = 0;

    // Switch Role
    virtual future< ofp::role_config >
        request_role(uint32_t role, uint64_t gen_id) = 0;

    // Port desc
    virtual future< sequence<of13::Port> >
        request_port_desc() = 0;

    // Port stats
    virtual future< of13::PortStats >
        request_port_stats(uint32_t port_no) = 0;
    virtual future< sequence<of13::PortStats> >
        request_port_stats() = 0;

    // Queue stats
    virtual future< of13::QueueStats >
        request_queue_stats(uint32_t port_no, uint32_t queue_id) = 0;
    virtual future< sequence<of13::QueueStats> >
        request_queue_stats(uint32_t port_no) = 0;
    virtual future< sequence<of13::QueueStats> >
        request_queue_stats() = 0;
    
    // Flow stats
    virtual future< sequence<of13::FlowStats> >
        request_flow_stats(ofp::flow_stats_request r) = 0;
    virtual future< ofp::aggregate_stats >
        request_aggregate(ofp::flow_stats_request r) = 0;

    // Group desc
    virtual future< sequence<of13::GroupDesc> >
        request_group_desc() = 0;

    // Group stats
    virtual future< sequence<of13::GroupStats> >
        request_group_stats() = 0;
    virtual future< of13::GroupStats >
        request_group_stats(uint32_t group_id) = 0;
    
    // Table stats
    virtual future< sequence<of13::TableStats> >
        request_table_stats() = 0;

    // Meter stats
    virtual future< sequence<of13::MeterStats> >
        request_meter_stats() = 0;
    virtual future< of13::MeterStats >
        request_meter_stats(uint32_t meter_id) = 0;
   
    //Meter Config
    virtual future < sequence< of13::MeterConfig > >
        request_meter_config() = 0;
    //Meter Features
    virtual future< of13::MeterFeatures >
        request_meter_features() = 0;

    // Flow mod
    virtual future < void >
        flow_mod(of13::FlowMod& flow_mod) = 0;
    // Group mod
    virtual future < void >
        group_mod(of13::GroupMod& group_mod) = 0;
    // Meter mod
    virtual future < void >
        meter_mod(of13::MeterMod& meter_mod) = 0;

    virtual ~OFAgent() = default;

#if 0
    // constant switch information
    future<ofp::switch_features> request_switch_features();
    future<ofp::desc> request_description();
    future<ofp::group_features> request_group_features();

    future<ofp::switch_config> request_config();
    future<void> set_config(ofp::switch_config const& sc);

    future<ofp::async_config> request_async_config();
    future<void> set_async_config(ofp::async_config const& aconf);

    future<void> flow_mod(ofp::flow_mod const& fm);
    future<void> group_mod(ofp::group_mod const& gm);
    future<void> port_mod(ofp::port_mod const& pm);
    future<void> meter_mod(ofp::meter_mod const& mm);
    future<void> packet_out(ofp::packet_out const& po);

    future<vector<ofp::table_features>>
        request_table_features();
    future<vector<ofp::table_features>>
        set_table_features(vector<ofp::table_features> features);
    future<vector<ofp::port>>
        request_port_descriptions();
    future<vector<ofp::packet_queue>>
        request_queue_config();
#endif
};

} // namespace runos
