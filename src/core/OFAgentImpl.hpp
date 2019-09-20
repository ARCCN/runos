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

#include "lib/lambda_visitor.hpp"
#include "api/OFAgent.hpp"
#include "api/OFConnection.hpp"
#include <runos/core/throw.hpp>
#include <runos/core/logging.hpp>

#include <boost/variant.hpp>
#include <boost/variant/polymorphic_get.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <atomic>
#include <list>
#include <utility> // declval
#include <memory>

namespace runos {

namespace of13 = fluid_msg::of13;

class OFAgentImpl;
using OFAgentImplPtr = std::shared_ptr<OFAgentImpl>;

class OFAgentImpl final : public OFAgent
{
public:
    explicit OFAgentImpl(OFConnection* conn);

    // Synchronization
    future< void >
        barrier() override;

    // Switch Config
    future< ofp::switch_config >
        request_config() override;
    future< void >
        set_config(ofp::switch_config config) override;

    // Switch Description
    future< fluid_msg::SwitchDesc >
        request_switch_desc() override;

    // Switch Role
    future< ofp::role_config >
        request_role(uint32_t role, uint64_t gen_id) override;

    // Port desc
    future< sequence<of13::Port> >
        request_port_desc() override;

    // Port stats
    future< of13::PortStats >
        request_port_stats(uint32_t port_no) override;
    future< sequence<of13::PortStats> >
        request_port_stats() override;
    
    // Queue stats
    future< of13::QueueStats >
        request_queue_stats(uint32_t port_no, uint32_t queue_id) override;
    future< sequence<of13::QueueStats> >
        request_queue_stats(uint32_t port_no) override;
    future< sequence<of13::QueueStats> >
        request_queue_stats() override;
    
    // Flow stats
    future< sequence<of13::FlowStats> >
        request_flow_stats(ofp::flow_stats_request r) override;
    future< ofp::aggregate_stats >
        request_aggregate(ofp::flow_stats_request r) override;
    
     // Group desc
    future< sequence<of13::GroupDesc> >
        request_group_desc() override;
   
    // Group stats
    future< sequence<of13::GroupStats> >
        request_group_stats() override;
    future< of13::GroupStats >
        request_group_stats(uint32_t group_id) override;
    
    // Table stats
    future< sequence<of13::TableStats> >
        request_table_stats() override;

    // Meter stats
    future< sequence<of13::MeterStats> >
        request_meter_stats() override;
    future< of13::MeterStats >
        request_meter_stats(uint32_t meter_id) override;
    
    //Meter Config
    future < sequence< of13::MeterConfig > >
        request_meter_config() override;

    //Meter Features
    future< of13::MeterFeatures >
        request_meter_features() override;

    // Flow mod
    future < void >
        flow_mod(of13::FlowMod& flow_mod) override;
    // Group mod
    future < void >
        group_mod(of13::GroupMod& group_mod) override;
    // Meter mod
    future < void >
        meter_mod(of13::MeterMod& meter_mod) override;

    static uint_fast32_t constexpr get_minimal_xid() { return minimal_xid; }

protected:
    class SendHandler;
    class RecvHandler;

    struct session_base {
        const uint32_t xid;
        // set it to `false` if you don't receive response
        // from the switch but want to notify client about completition
        // when next barrier received
        const bool waiting_for_response;
        bool value_set {false};

        explicit session_base(uint32_t xid, bool wfr)
            : xid(xid), waiting_for_response(wfr)
        { }

        // <concept>
        // promise<T> promise_;
        // </concept>
    };

    // session for messages which don't need the answer
    struct no_respond_session : session_base {
        explicit no_respond_session(uint32_t xid)
            : session_base(xid, false)
        { }
        
        promise<void> promise_;
    };

    struct get_config_session : session_base {
        explicit get_config_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<ofp::switch_config> promise_;
    };

    struct switch_desc_session : session_base {
        explicit switch_desc_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<fluid_msg::SwitchDesc> promise_;
    };

    struct role_reply_session : session_base {
        explicit role_reply_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<ofp::role_config> promise_;
    };

    struct barrier_session : session_base {
        explicit barrier_session(uint32_t xid)
            : session_base(xid, true)
        { }

        promise<void> promise_;
    };

    struct port_desc_seq_session : session_base {
        explicit port_desc_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::Port> ret;
        promise<sequence<of13::Port>> promise_;
    };

    struct port_stat_seq_session : session_base {
        explicit port_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::PortStats> ret;
        promise<sequence<of13::PortStats>> promise_;
    };

    struct port_stat_session : session_base {
        explicit port_stat_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<of13::PortStats> promise_;
    };

    struct queue_stat_seq_session : session_base {
        explicit queue_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::QueueStats> ret;
        promise<sequence<of13::QueueStats>> promise_;
    };

    struct queue_stat_session : session_base {
        explicit queue_stat_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<of13::QueueStats> promise_;
    };

    struct flow_stat_seq_session : session_base {
        explicit flow_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }

        sequence<of13::FlowStats> ret;
        promise< sequence<of13::FlowStats> > promise_;
    };

    struct flow_aggregate_session : session_base {
        explicit flow_aggregate_session(uint32_t xid)
            : session_base(xid, true)
        { }

        promise< ofp::aggregate_stats > promise_;
    };

    struct group_desc_seq_session : session_base {
        explicit group_desc_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::GroupDesc> ret;
        promise< sequence<of13::GroupDesc> > promise_;
    };

    struct group_stat_seq_session : session_base {
        explicit group_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::GroupStats> ret;
        promise<sequence<of13::GroupStats> > promise_;
    };

    struct group_stat_session : session_base {
        explicit group_stat_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<of13::GroupStats> promise_;
    };

    struct table_stat_seq_session : session_base {
        explicit table_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::TableStats> ret;
        promise<sequence<of13::TableStats> > promise_;
    };
    

    struct meter_stat_seq_session : session_base {
        explicit meter_stat_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::MeterStats> ret;
        promise<sequence<of13::MeterStats>> promise_;
    };

    struct meter_stat_session : session_base {
        explicit meter_stat_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<of13::MeterStats> promise_;
    };

    struct meter_config_seq_session : session_base {
        explicit meter_config_seq_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        sequence<of13::MeterConfig> ret;
        promise<sequence<of13::MeterConfig> > promise_;
    };
    
    struct meter_features_session : session_base {
        explicit meter_features_session(uint32_t xid)
            : session_base(xid, true)
        { }
        
        promise<of13::MeterFeatures> promise_;
    };

    // TODO - boost::variant has a restriction on params count = 20
    // To solve this problem use another approach.
    // For example:
    // #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    // #define BOOST_MPL_LIMIT_LIST_SIZE 30
    // #define BOOST_MPL_LIMIT_VECTOR_SIZE 30

    using session = boost::variant<
        barrier_session,
        no_respond_session,
        get_config_session,
        switch_desc_session,
        role_reply_session,
        port_desc_seq_session,
        port_stat_session,
        port_stat_seq_session,
        queue_stat_session,
        queue_stat_seq_session,
        flow_stat_seq_session,
        flow_aggregate_session,
        group_desc_seq_session,
        group_stat_session,
        group_stat_seq_session,
        table_stat_seq_session,
        meter_stat_session,
        meter_stat_seq_session,
        meter_config_seq_session,
        meter_features_session
    >;

    using session_list = std::list<session>;

    struct DefaultOnResponseVisitor {
        template<class T>
        void operator()(T& s) const {
            THROW(bad_reply(0, s.xid), "Unexpected session type for this xid: {}",
                                       typeid(T).name());
        }
    };

    //
    // Template Methods
    //

    template<class T>
    struct set_exception_visitor;

    template<class T>
    static set_exception_visitor<T> make_set_exception_visitor(T&& value);

    template<class... Visitor>
    void on_response(uint32_t xid, Visitor&&... visitor);

    template<class Session, class T>
    static void set_exception(Session& session, T&& value);

    template<class T>
    static void set_exception(OFAgentImpl::session& session, T&& value);

    template<class Session, class Message>
    auto request(Message& msg)
        -> decltype( std::declval<Session>().promise_.get_future() );

    //
    // Methods
    //

    session_list::iterator find_task(uint32_t xid);
    void pop_tasks_until(uint32_t xid);
    uint64_t dpid() const { return conn_->dpid(); }

private:
    OFConnection* conn_;
    mutable boost::shared_mutex tasks_mutex_;
    session_list tasks_;

    OFConnection::SendHookHandlerPtr send_hook_handler_;
    OFConnection::ReceiveHandlerPtr recv_handler_;

    static uint_fast32_t constexpr minimal_xid = 0x10000;
    std::atomic_uint_fast32_t next_xid_ {minimal_xid};
};

///////////////////////////////////////////////////////////////////////////////
//                           Implementation                                  //
///////////////////////////////////////////////////////////////////////////////

template<class T>
struct OFAgentImpl::set_exception_visitor : virtual boost::static_visitor<void> {
    T&& value;

    explicit set_exception_visitor(T&& value)
        : value(std::move(value))
    { }

    template<class Session>
    void operator()(Session& session)
    {
        session.promise_.set_exception(std::move(value));
        session.value_set = true;
    }
};

template<class T>
auto OFAgentImpl::make_set_exception_visitor(T&& value)
    -> set_exception_visitor<T>
{
    return set_exception_visitor<T>{ std::move(value) };
}

template<class Session, class T>
void OFAgentImpl::set_exception(Session& session, T&& value)
{
    set_exception_visitor<T> visitor { std::move(value) };
    visitor(session);
}

template<class T>
void OFAgentImpl::set_exception(OFAgentImpl::session& session, T&& value)
{
    set_exception_visitor<T> visitor { std::move(value) };
    boost::apply_visitor(visitor, session);
}

template<class Session, class Message>
auto OFAgentImpl::request(Message& msg)
    -> decltype( std::declval<Session>().promise_.get_future() )
{
    uint64_t xid = next_xid_++;
    msg.xid(xid);

    boost::unique_lock< boost::shared_mutex > wlock(tasks_mutex_);
    Session session{ msg.xid() };
    auto fut = session.promise_.get_future();
    tasks_.push_back(std::move(session));
    wlock.unlock();

    if (conn_->alive()) {
        // TODO: Race condition can be here
        conn_->send(msg);
    } else {
        THROW(request_error(dpid(), xid), "Request to offline switch");
    }

    return std::move(fut);
}

template<class... Visitors>
void OFAgentImpl::on_response(uint32_t xid, Visitors&&... visitors)
{
    if (xid < minimal_xid)
        return;

    boost::upgrade_lock<boost::shared_mutex> rlock(tasks_mutex_);

    auto task_it = find_task(xid);
    if (task_it == tasks_.end()) {
        THROW(bad_reply(dpid(), xid), "Unexpected reply");
    }

    auto& session = boost::polymorphic_get<session_base>(*task_it);
    if (not session.waiting_for_response) {
        set_exception(*task_it, not_responded(dpid(), xid));
    }

    auto visitor = lambda_visitor<void, Visitors..., DefaultOnResponseVisitor>(
        std::forward<Visitors>(visitors)..., DefaultOnResponseVisitor()
    );
    boost::apply_visitor(visitor, *task_it);

    if (session.value_set) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
        tasks_.erase(task_it);
    }
}

} // namespace runos
