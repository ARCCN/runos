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

#include "StatsBucket.hpp"

#include <runos/core/future.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/catch_all.hpp>

#include "StatisticsStore.hpp"
#include "OFServer.hpp"

#include "lib/qt_executor.hpp"
#include "api/OFAgent.hpp"

#include <boost/thread/shared_mutex.hpp>
#include <range/v3/action/transform.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

#include <unordered_map>
#include <iterator> // begin, end, move
#include <chrono>
#include <atomic>
#include <functional> // mem_fn

namespace runos {

REGISTER_APPLICATION(StatsBucketManager, {"of-server", ""})

class FlowStatsBucketImpl : public FlowStatsBucket
                          , public std::enable_shared_from_this<FlowStatsBucketImpl>
{
    Q_OBJECT

public:
    explicit FlowStatsBucketImpl(std::string name,
                                 StatsBucketManager::FlowSelector selector,
                                 QObject* parent)
        : id_(next_id++)
        , name_(std::move(name))
    {
        using namespace flow_selector;
        moveToThread(parent->thread());

        ofp::flow_stats_request req_proto;

        if (selector.get(table)) {
            req_proto.table_id = *selector.get(table);
        }

        if (selector.get(cookie) && not selector.get(cookie_mask)) {
            req_proto.cookie = *selector.get(cookie);
            req_proto.cookie_mask = 0xFFFFFFFFFFFFFFFFULL;
        }

        if (selector.get(cookie) && selector.get(cookie_mask)) {
            req_proto.cookie = *selector.get(cookie);
            req_proto.cookie_mask = *selector.get(cookie_mask);
        }

        if (selector.get(match)) {
            req_proto.match = std::move(*selector.get(match));
        }

        std::vector<uint32_t> out_ports;
        if (selector.get(out_port)) {
            out_ports = std::move(*selector.get(out_port));
        } else {
            out_ports.push_back(of13::OFPP_ANY);
        }

        std::vector<uint32_t> out_groups;
        if (selector.get(out_group)) {
            out_groups = std::move(*selector.get(out_group));
        } else {
            out_groups.push_back(of13::OFPG_ANY);
        }

        for (auto port : out_ports) {
            for (auto group : out_groups) {
                auto req = req_proto;
                req.out_port = port;
                req.out_group = group;
                requests_.push_back( std::move(req) );
            }
        }

        dpids_ = *selector.get(dpid);
        per_request_stats_.resize( dpids_.size() * requests_.size() );
    }

    void start(OFServer* ofserver, std::chrono::milliseconds period);

    // observers
    int id() const override { return id_; }
    std::string name() const override { return name_; }

    Statistics<FlowMeasurement> stats() const override
    {
        return aggregated_stats_.get();
    }

    void update();

protected:
    void timerEvent(QTimerEvent*) override;

private:
    using stats_store_type
        = StatisticsStore<FlowMeasurement>;

    static std::atomic<int> next_id;
    const int id_;
    const std::string name_;

    std::vector<uint64_t> dpids_;
    std::vector<OFAgentPtr> agents_;
    std::vector<ofp::flow_stats_request> requests_;

    std::chrono::steady_clock clock_;
    stats_store_type aggregated_stats_;
    std::vector< ofp::aggregate_stats > per_request_stats_;

    qt_executor executor {this};
    std::vector<OFAgentPtr> agents;
};

std::atomic<int> FlowStatsBucketImpl::next_id {0};

using FlowStatsBucketImplPtr = std::shared_ptr<FlowStatsBucketImpl>;
using FlowStatsBucketImplWeakPtr = std::weak_ptr<FlowStatsBucketImpl>;

void FlowStatsBucketImpl::start(OFServer* ofserver, std::chrono::milliseconds period)
{
    auto futures = dpids_
        | ranges::view::transform([&](auto id) { return ofserver->agent(id); })
        | ranges::to_<std::vector>();

    when_all_fix(futures.begin(), futures.end()).then(executor,
        [self = shared_from_this(), period](auto ret) {
            VLOG(10) << "Activating stats bucket " << self->id()
                << " (" << self->name() << ')';

            for (auto&& agent : ret.get()) {
                self->agents_.push_back(agent.get());
            }

            self->startTimer(period.count());
            self->update();
        });
}

void FlowStatsBucketImpl::timerEvent(QTimerEvent*)
try {
    update();
} catch (std::bad_weak_ptr const&) {
    // shared_ptr control block may be destroyed before
    // QObject deleted by deleteLater(), so ignore this
    // exception (caused by shared_from_this())
}

void FlowStatsBucketImpl::update()
{
    using future_vector =
        std::vector< future<ofp::aggregate_stats> >;

    future_vector futures;
    futures.reserve( per_request_stats_.size() );

    int j = 0;
    for (int i = agents_.size()-1; i >= 0; --i) {
        auto agent = agents_[i];
        for (auto& req : requests_) {
            try {
                auto f = agent->request_aggregate(req);
                futures.push_back(std::move(f));
            } catch (OFAgent::request_error const& ex) {
                futures.push_back(make_ready_future(per_request_stats_[j]));
                VLOG(10) << "Failed to request flow stats " << j << ":";
                //diagnostic_information::get().log();
            }
            ++j;
        }
    }

    when_all_fix(futures.begin(), futures.end()).then(executor,
        [self = shared_from_this()](future< future_vector > result) {
            VLOG(10) << "Entering flow stats continuation";

            FlowMeasurement<uint64_t> acc;
            ranges::fill(acc, 0);

            int j = 0;
            for (auto& fstats : result.get()) {
                ofp::aggregate_stats stats;
                try {
                    stats = fstats.get();
                } catch (OFAgent::openflow_error const& ex) {
                    stats = self->per_request_stats_[j];
                    VLOG(10) << "Failed to get flow stats " << j << ":";
                    diagnostic_information::get().log();
                }

                acc.packets() += stats.packets;
                acc.bytes() += stats.bytes;
                acc.flows() += stats.flows;
                ++j;
            }

            VLOG(10) << "Bucket " << self->id() << " (" << self->name() << ") updated";
            self->aggregated_stats_.append(self->clock_.now().time_since_epoch(), acc);
            emit self->updated();
        }
    );
}

///////////////////////////
//        Manager        //
///////////////////////////

struct StatsBucketManager::implementation
{
    OFServer* ofserver;
    mutable boost::shared_mutex mutex;
    std::unordered_map<int, FlowStatsBucketImplWeakPtr> bucket;
    std::unordered_map<std::string, FlowStatsBucketImplWeakPtr> bucket_by_name;
};

StatsBucketManager::StatsBucketManager()
    : impl(new implementation)
{ }

StatsBucketManager::~StatsBucketManager() noexcept = default;

void StatsBucketManager::init(Loader* loader, const Config&)
{
    impl->ofserver = OFServer::get(loader);
}

auto StatsBucketManager::bucket(int id) const
    -> FlowStatsBucketPtr
{
    boost::shared_lock< boost::shared_mutex > lock(impl->mutex);
    return impl->bucket.at(id).lock();
}

auto StatsBucketManager::bucket(std::string const& name) const
    -> FlowStatsBucketPtr
{
    boost::shared_lock< boost::shared_mutex > lock(impl->mutex);
    auto bucket_it = impl->bucket_by_name.find(name);
    if (bucket_it == impl->bucket_by_name.end())
        return nullptr;
    return bucket_it->second.lock();
}

auto StatsBucketManager::aggregateFlows(duration poll_interval,
                                        std::string name,
                                        FlowSelector selector)
    -> FlowStatsBucketPtr
{
    FlowStatsBucketImplPtr bucket {new FlowStatsBucketImpl(
        std::move(name),
        std::move(selector),
        this
    ), std::mem_fn(&QObject::deleteLater)};

    {
        boost::unique_lock< boost::shared_mutex > lock(impl->mutex);
        impl->bucket[bucket->id()] = bucket;
        if (not bucket->name().empty()) {
            impl->bucket_by_name[bucket->name()] = bucket;
        }
    }

    bucket->start(impl->ofserver, poll_interval);
    return bucket;
}

} // namespace runos

#include "StatsBucket.moc"
