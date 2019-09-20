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
 
#include "OFAgentImpl.hpp"

#include <utility>
#include <algorithm>
#include <iterator>

namespace runos {

class OFAgentImpl::SendHandler
    : public OFConnection::SendHookHandler<of13::BarrierRequest>
{
public:
    explicit SendHandler(OFAgentImpl* agent)
        : self(agent)
    { }

    void process(of13::BarrierRequest& br) {
        boost::unique_lock<boost::shared_mutex> wlock(self->tasks_mutex_);
        self->tasks_.emplace_back(barrier_session(br.xid()));
    }
private:
    OFAgentImpl* self;
};

template<class Session, class T>
void set_value(Session& session, T&& value)
{
    session.promise_.set_value(std::forward<T>(value));
    session.value_set = true;
}

class OFAgentImpl::RecvHandler
    : public OFConnection::ReceiveHandler<of13::Error>
    , public OFConnection::ReceiveHandler<of13::RoleReply>
    , public OFConnection::ReceiveHandler<of13::BarrierReply>
    , public OFConnection::ReceiveHandler<of13::GetConfigReply>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyDesc>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyPortDescription>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyPortStats>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyQueue>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyFlow>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyAggregate>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyGroup>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyGroupDesc>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyTable>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyMeter>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyMeterConfig>
    , public OFConnection::ReceiveHandler<of13::MultipartReplyMeterFeatures>
{
public:
    explicit RecvHandler(OFAgentImpl* agent)
        : self(agent)
    { }

    void process(of13::Error& e) override
    {
        auto visitor = OFAgentImpl::make_set_exception_visitor(
                    openflow_error(self->dpid(), e.xid(), e.type(), e.code()));
        try {
            self->on_response(e.xid(), std::move(visitor));
        } catch (OFAgent::bad_reply& e) {
            // ignore (it's not our error)
            LOG(ERROR) << "[OFAgent] Bad reply exception - Error on switch with dpid="
                       << e.dpid() << " and xid=" << e.xid() << " has come";
        }
    }

    void process(of13::BarrierReply& br) override
    {
        self->pop_tasks_until(br.xid());
    }

    void process(of13::RoleReply& r) override
    {
        self->on_response(r.xid(),
            [&](role_reply_session& session) {
                ofp::role_config ret;
                ret.role = r.role();
                ret.generation_id = r.generation_id();
                set_value(session, ret);
            }
        );
    }

    void process(of13::GetConfigReply& r) override
    {
        self->on_response(r.xid(),
            [&](get_config_session& session) {
                ofp::switch_config ret;
                ret.flags = r.flags();
                ret.miss_send_len = r.miss_send_len();
                set_value(session, ret);
            }
        );
    }

    void process(of13::MultipartReplyDesc& r) override
    {
        self->on_response(r.xid(),
            [&](switch_desc_session& session) {
                set_value(session, r.desc());
            }
        );
    }

    void process(of13::MultipartReplyPortDescription& pd) override
    {
        bool more = pd.flags() & of13::OFPMPF_REQ_MORE;
        auto ports = pd.ports();

        self->on_response(pd.xid(),
            [&](port_desc_seq_session& session) {
                auto& ret = session.ret;
                std::copy(ports.begin(), ports.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyPortStats& ps) override
    {
        bool more = ps.flags() & of13::OFPMPF_REQ_MORE;
        auto stats = ps.port_stats();

        self->on_response(ps.xid(),
            [&](port_stat_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), ps.xid()));
                } else if (stats.size() != 1) {
                    set_exception(session, bad_reply(self->dpid(), ps.xid()));
                } else {
                    set_value(session, stats[0]);
                }
            },
            [&](port_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyQueue& qs) override
    {
        bool more = qs.flags() & of13::OFPMPF_REQ_MORE;
        auto stats = qs.queue_stats();

        self->on_response(qs.xid(),
            [&](queue_stat_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), qs.xid()));
                } else if (stats.size() != 1) {
                    set_exception(session, bad_reply(self->dpid(), qs.xid()));
                } else {
                    set_value(session, stats[0]);
                }
            },
            [&](queue_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyFlow& fs) override
    {
        bool more = fs.flags() & of13::OFPMPF_REQ_MORE;
        auto stats = fs.flow_stats();

        self->on_response(fs.xid(),
            [&](flow_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyAggregate& as) override
    {
        bool more = as.flags() & of13::OFPMPF_REQ_MORE;
        ofp::aggregate_stats stats;
        stats.flows = as.flow_count();
        stats.bytes = as.byte_count();
        stats.packets = as.packet_count();
        
        self->on_response(as.xid(),
            [&](flow_aggregate_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), as.xid()));
                } else {
                    set_value(session, std::move(stats));
                }
            }
        );
    }

    void process(of13::MultipartReplyGroup& gs) override
    {
        bool more = gs.flags() & of13::OFPMPF_REQ_MORE;
        // Yes, libfluid authors love copy-paste too!
        auto stats = gs.queue_stats();

        self->on_response(gs.xid(),
            [&](group_stat_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), gs.xid()));
                } else if (stats.size() != 1) {
                    set_exception(session, bad_reply(self->dpid(), gs.xid()));
                } else {
                    set_value(session, stats[0]);
                }
            },
            [&](group_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyGroupDesc& gd) override
    {
        bool more = gd.flags() & of13::OFPMPF_REQ_MORE;
        // Yes, libfluid authors love copy-paste too! [2]
        auto desc = gd.queue_stats();

        self->on_response(gd.xid(),
            [&](group_desc_seq_session& session) {
                auto& ret = session.ret;
                std::copy(desc.begin(), desc.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    void process(of13::MultipartReplyTable& ts) override
    {
        bool more = ts.flags() & of13::OFPMPF_REQ_MORE;
        auto stats = ts.table_stats();

        self->on_response(ts.xid(),
            [&](table_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }

    
    void process(of13::MultipartReplyMeter& ms) override
    {
        bool more = ms.flags() & of13::OFPMPF_REQ_MORE;
        auto stats = ms.meter_stats();

        self->on_response(ms.xid(),
            [&](meter_stat_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), ms.xid()));
                } else if (stats.size() != 1) {
                    set_exception(session, bad_reply(self->dpid(), ms.xid()));
                } else {
                    set_value(session, stats[0]);
                }
            },
            [&](meter_stat_seq_session& session) {
                auto& ret = session.ret;
                std::copy(stats.begin(), stats.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
    }
    
    void process(of13::MultipartReplyMeterConfig& mc) override
    {
        bool more = mc.flags() & of13::OFPMPF_REQ_MORE;
        auto conf = mc.meter_config();

        self->on_response(mc.xid(),
            [&](meter_config_seq_session& session) {
                auto& ret = session.ret;
                std::copy(conf.begin(), conf.end(),
                          std::back_inserter(ret));
                if (not more) {
                    set_value(session, std::move(ret));
                }
            }
        );
      
    }
    
    void process(of13::MultipartReplyMeterFeatures& mf) override
    {
        bool more = mf.flags() & of13::OFPMPF_REQ_MORE;
        auto features = mf.meter_features();
 
        self->on_response(mf.xid(),
            [&](meter_features_session& session) {
                if (more) {
                    set_exception(session, bad_reply(self->dpid(), mf.xid()));
                } else {
                    set_value(session, features);
                }
            }
        );
    }

private:
    OFAgentImpl* self;
};

OFAgentImpl::OFAgentImpl(OFConnection* conn)
    : conn_(conn)
    , send_hook_handler_(new SendHandler(this))
    , recv_handler_(new RecvHandler(this))
{
    conn_->send_hook(send_hook_handler_);
    conn_->receive(recv_handler_);
}

auto OFAgentImpl::find_task(uint32_t xid)
    -> session_list::iterator
{
    if (xid < minimal_xid) {
        return tasks_.end();
    }
    return std::find_if(tasks_.begin(), tasks_.end(), [=](session const& s) {
        return boost::polymorphic_get<session_base>(s).xid == xid;
    });
}

void OFAgentImpl::pop_tasks_until(uint32_t xid)
{
    boost::upgrade_lock<boost::shared_mutex> rlock(tasks_mutex_);

    auto begin = tasks_.begin();
    auto end = tasks_.end();
    auto barrier_it = std::find_if(begin, end, [=](session const& s) {
        return boost::polymorphic_get<session_base>(s).xid == xid &&
               boost::get<barrier_session>(&s) != nullptr;
    });

    if (barrier_it == end) {
        return;
    }

    for (auto it = begin; it != barrier_it; ++it) {
        auto& session = boost::polymorphic_get<session_base>(*it);
        if (session.waiting_for_response) {
            set_exception(*it, not_responded(dpid(), session.xid));
        } else {
            boost::get<no_respond_session>(*it).promise_.set_value();
        }
    }

    boost::get<barrier_session>(*barrier_it).promise_.set_value();
    boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
    tasks_.erase(begin, ++barrier_it);
}

auto OFAgentImpl::request_config()
    -> future< ofp::switch_config >
{
    of13::GetConfigRequest req;
    return request<get_config_session>(req);
}

auto OFAgentImpl::set_config(ofp::switch_config config)
    -> future< void >
{
    of13::SetConfig req;
    req.miss_send_len(config.miss_send_len);
    req.flags(config.flags);
    return request<no_respond_session>(req);
}

auto OFAgentImpl::request_switch_desc()
    -> future< fluid_msg::SwitchDesc >
{
    of13::MultipartRequestDesc req;
    req.flags(0);
    return request<switch_desc_session>(req);
}

auto OFAgentImpl::request_role(uint32_t role, uint64_t gen_id)
    -> future< ofp::role_config >
{
    of13::RoleRequest req(70000, role, gen_id);
    req.role(role);
    req.generation_id(gen_id);
    return request<role_reply_session>(req);
}

auto OFAgentImpl::request_port_desc()
    -> future< sequence<of13::Port> >
{
    of13::MultipartRequestPortDescription req;
    req.flags(0);
    return request<port_desc_seq_session>(req);
}

auto OFAgentImpl::request_port_stats(uint32_t port_no)
    -> future< of13::PortStats >
{
    THROW_IF(port_no == of13::OFPP_ANY, invalid_argument());

    of13::MultipartRequestPortStats req;
    req.flags(0);
    req.port_no(port_no);

    return request<port_stat_session>(req);
}

auto OFAgentImpl::request_port_stats() 
    -> future< sequence<of13::PortStats> >
{
    of13::MultipartRequestPortStats req;
    req.flags(0);
    req.port_no(of13::OFPP_ANY);

    return request<port_stat_seq_session>(req);
}

auto OFAgentImpl::request_queue_stats(uint32_t port_no, uint32_t queue_id)
    -> future< of13::QueueStats >
{
    THROW_IF(port_no == of13::OFPP_ANY, invalid_argument());
    THROW_IF(queue_id == OFPQ_ALL, invalid_argument());

    of13::MultipartRequestQueue req;
    req.flags(0);
    req.port_no(port_no);
    req.queue_id(queue_id);

    return request<queue_stat_session>(req);
}

auto OFAgentImpl::request_queue_stats(uint32_t port_no)
    -> future< sequence<of13::QueueStats> >
{
    THROW_IF(port_no == of13::OFPP_ANY, invalid_argument());

    of13::MultipartRequestQueue req;
    req.flags(0);
    req.port_no(port_no);
    req.queue_id(OFPQ_ALL);

    return request<queue_stat_seq_session>(req);
}

auto OFAgentImpl::request_queue_stats()
    -> future< sequence<of13::QueueStats> >
{
    of13::MultipartRequestQueue req;
    req.flags(0);
    req.port_no(of13::OFPP_ANY);
    req.queue_id(OFPQ_ALL);

    return request<queue_stat_seq_session>(req);
}

auto OFAgentImpl::request_flow_stats(ofp::flow_stats_request r)
    -> future< sequence<of13::FlowStats> >
{
    of13::MultipartRequestFlow req;
    req.flags(0);
    req.table_id(r.table_id);
    req.out_port(r.out_port);
    req.out_group(r.out_group);
    req.cookie(r.cookie);
    req.cookie_mask(r.cookie_mask);
    req.match(std::move(r.match));

    return request<flow_stat_seq_session>(req);
}

auto OFAgentImpl::request_aggregate(ofp::flow_stats_request r)
    -> future< ofp::aggregate_stats >
{
    of13::MultipartRequestAggregate req{
        0, // xid
        0, // flags
        r.table_id,
        r.out_port,
        r.out_group,
        r.cookie,
        r.cookie_mask,
        std::move(r.match)
    };

    return request<flow_aggregate_session>(req);
}

auto OFAgentImpl::request_group_desc()
    -> future< sequence<of13::GroupDesc> >
{
    of13::MultipartRequestGroupDesc req;
    req.flags(0);

    return request<group_desc_seq_session>(req);
}

auto OFAgentImpl::request_group_stats(uint32_t group_id)
    -> future< of13::GroupStats >
{
    THROW_IF(group_id == of13::OFPG_ANY, invalid_argument());

    of13::MultipartRequestGroup req;
    req.flags(0);
    req.group_id(group_id);

    return request<group_stat_session>(req);
}

auto OFAgentImpl::request_group_stats()
    -> future< sequence<of13::GroupStats> >
{
    of13::MultipartRequestGroup req;
    req.flags(0);
    req.group_id(of13::OFPG_ALL);

    return request<group_stat_seq_session>(req);
}

auto OFAgentImpl::request_table_stats()
    -> future< sequence<of13::TableStats> >
{
    of13::MultipartRequestTable req;
    req.flags(0);
    return request<table_stat_seq_session>(req);
}

auto OFAgentImpl::request_meter_stats(uint32_t meter_id)
    -> future< of13::MeterStats >
{
    THROW_IF(meter_id == of13::OFPM_ALL, invalid_argument());

    of13::MultipartRequestMeter req;
    req.flags(0);
    req.meter_id(meter_id);

    return request<meter_stat_session>(req);
}

auto OFAgentImpl::request_meter_stats()
    -> future< sequence<of13::MeterStats> >
{
    of13::MultipartRequestMeter req;
    req.flags(0);
    req.meter_id(of13::OFPM_ALL);

    return request<meter_stat_seq_session>(req);
}

auto OFAgentImpl::request_meter_config()
    -> future< sequence<of13::MeterConfig> >
{
    of13::MultipartRequestMeterConfig req;
    req.flags(0);
    req.meter_id(of13::OFPM_ALL);

    return request<meter_config_seq_session>(req);
}

auto OFAgentImpl::request_meter_features()
    -> future< of13::MeterFeatures >
{
    of13::MultipartRequestMeterFeatures req;

    return request<meter_features_session>(req);
}

auto OFAgentImpl::flow_mod(of13::FlowMod &flow_mod)
    -> future<void>
{
    return request<no_respond_session>(flow_mod);
}

auto OFAgentImpl::group_mod(of13::GroupMod &group_mod)
    -> future<void>
{
    return request<no_respond_session>(group_mod);
}

auto OFAgentImpl::meter_mod(of13::MeterMod &meter_mod)
    -> future<void>
{
    return request<no_respond_session>(meter_mod);
}

future<void> OFAgentImpl::barrier()
{
    of13::BarrierRequest br;
    return request<barrier_session>(br);
}

} // namespace runos
