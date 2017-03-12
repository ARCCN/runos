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

#include "TraceTree.hh"

#include <unordered_map>

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/recursive_variant.hpp>

#include "api/Packet.hh"
#include "TraceablePacketImpl.hh"

namespace runos {
namespace maple {

struct TraceTree::unexplored {
};

struct TraceTree::flow_node {
    std::weak_ptr<Flow> flow;
    uint16_t prio;
};

struct TraceTree::test_node {
    oxm::field<> need;
    node positive;
    node negative;
    uint64_t id;
    uint16_t prio;
};

static uint64_t id_generator()
{
    static uint64_t next_id(1);
    return next_id++;
}

struct TraceTree::load_node {
    oxm::mask<> mask;
    std::unordered_map< bits<>, node >
        cases;
};

struct TraceTree::Impl {
    class Lookup;
    class Compiler;
    class TracerImpl;
    class PriorityUpdater;
};

class TraceTree::Impl::Compiler : public boost::static_visitor<>
{
    Backend& backend;
    oxm::expirementer::full_field_set match;

public:
    Compiler(Backend& backend,
            const oxm::expirementer::full_field_set &match)
        : backend(backend), match(match)
    { }
    Compiler(Backend& backend)
        : backend(backend)
    { }


    void operator()(unexplored&)
    {
        // do nothing
    }

    void operator()(test_node& test)
    {
        match.exclude(test.need);
        boost::apply_visitor(*this, test.negative);
        match.include(oxm::mask<>(test.need));

        match.add(test.need);
        backend.barrier_rule(test.prio, match, test.need, test.id);
        boost::apply_visitor(*this, test.positive);
        match.erase(oxm::mask<>(test.need));
    }

    void operator()(load_node& load)
    {
        auto type = load.mask.type();

        for (auto& record : load.cases) {
            match.add((type == record.first) & load.mask);
            boost::apply_visitor(*this, record.second);
            match.erase(load.mask);
        }
    }

    void operator()(flow_node& node)
    {
        if (auto flow = node.flow.lock())
            backend.install(node.prio, match, flow);
    }

};

class TraceTree::Impl::Lookup : public boost::static_visitor<FlowPtr>
{
    const Packet& pkt;
public:
    explicit Lookup(const Packet& pkt)
        : pkt(pkt)
    { }

    FlowPtr operator()(const unexplored&) const
    {
        return nullptr;
    }

    FlowPtr operator()(const test_node& test) const
    {
        if (pkt.test(test.need))
            return boost::apply_visitor(*this, test.positive);
        else
            return boost::apply_visitor(*this, test.negative);
    }

    FlowPtr operator()(const load_node& load) const
    {
        auto it = load.cases.find( pkt.load(load.mask).value_bits() );
        if (it != load.cases.end())
            return boost::apply_visitor(*this, it->second);
        else
            return nullptr;
    }

    FlowPtr operator()(const flow_node& leaf) const
    {
        if (auto flow = leaf.flow.lock())
            return flow;
        else
            return nullptr;
    }
};

class TraceTree::Impl::TracerImpl : public Tracer {
    std::vector<node*> path;
    Backend& backend;
    uint16_t left_prio, right_prio;
    oxm::expirementer::full_field_set match;

    node* node_ptr() { return path.back(); }
    void node_push(node* n) { path.push_back(n); }

public:
    explicit TracerImpl(node& root,
                        Backend& backend,
                        uint16_t left_prio,
                        uint16_t right_prio)
        : backend(backend), left_prio(left_prio), right_prio(right_prio)
    {
        path.push_back(&root);
    }

    void load(oxm::field<> data) override
    {
        if (boost::get<unexplored>(node_ptr())) {
            *node_ptr() = load_node{ oxm::mask<>(data), {} };
            node_push( &boost::get<load_node>(*node_ptr())
                       .cases
                       .emplace(data.value_bits(), unexplored())
                       .first->second ); // inserted value
        } else if (load_node* load = boost::get<load_node>(node_ptr())) {
            if (load->mask != oxm::mask<>(data))
                RUNOS_THROW(inconsistent_trace());
            path.push_back(&load->cases[ data.value_bits() ]);
        } else {
            RUNOS_THROW(inconsistent_trace());
        }
        match.add(data);
    }

    void test(oxm::field<> pred, bool ret) override
    {
        uint16_t test_prio;
        if (boost::get<unexplored>(node_ptr())) {
            test_prio = (left_prio + right_prio) / 2;
            if (test_prio <= left_prio or test_prio >= right_prio)
                RUNOS_THROW(priority_exceeded());
            uint64_t id = id_generator();
            *node_ptr() = test_node{
                pred, unexplored(), unexplored{}, id, test_prio
            };

            node_push( ret ?
                &boost::get<test_node>(node_ptr())->positive :
                &boost::get<test_node>(node_ptr())->negative );
            auto tmp_match = match;
            tmp_match.add(pred);
            backend.barrier_rule(test_prio, tmp_match, pred, id);

        } else if (test_node* test = boost::get<test_node>(node_ptr())) {
            if (test->need != pred)
                RUNOS_THROW(inconsistent_trace());
            test_prio = test->prio;
            node_push( ret ? &test->positive : &test->negative );
        } else {
            RUNOS_THROW(inconsistent_trace());
        }

        // update priotity range, and match
        if (ret){
            left_prio = test_prio;
            match.add(pred);
        } else {
            right_prio = test_prio;
            match.exclude(pred);
        }
    }

    Installer finish(FlowPtr new_flow) override
    {
        if (boost::get<unexplored>(node_ptr())) {
            uint16_t prio = (left_prio + right_prio) / 2;
            if (prio <= left_prio or prio >= right_prio)
                RUNOS_THROW(priority_exceeded());
            *node_ptr() = flow_node{ new_flow, prio };
        } else if (flow_node* leaf = boost::get<flow_node>(node_ptr())) {
            leaf->flow = new_flow;
        } else {
            RUNOS_THROW(inconsistent_trace());
        }

        auto node = node_ptr();

        node_push(nullptr);

        return [match=match, node=node, &backend=backend](){
            Impl::Compiler compiler(backend, match);
            boost::apply_visitor(compiler, *node);
        };
    }
};

class TraceTree::Impl::PriorityUpdater {
    struct Scope{
        unsigned positive; // count of test/flow nodes under test
        unsigned negative;;
        Scope(unsigned pos, unsigned neg)
            : positive(pos), negative(neg)
        { }
    };
    using Depth = std::unordered_map<uint64_t, Scope>;
    Depth depth;

    uint16_t from;
    uint16_t to;

    class DepthCounter : public boost::static_visitor<unsigned>
    {
        Depth &depth;
    public:
        DepthCounter(Depth &depth)
            :depth(depth)
        { }

        unsigned operator()(const unexplored&) const
        {
            return 1;
        }

        unsigned operator()(const test_node& test) const
        {
            unsigned pos = boost::apply_visitor(*this, test.positive);
            unsigned neg = boost::apply_visitor(*this, test.negative);
            depth.emplace(std::piecewise_construct,
                    std::forward_as_tuple(test.id),
                    std::forward_as_tuple(pos, neg));
            return pos + neg + 1;

        }
        unsigned operator()(const load_node& load) const
        {
            unsigned max_d = 0;
            for (auto& record: load.cases) {
                max_d =
                    std::max(boost::apply_visitor(*this, record.second), max_d);
            }
            return max_d;
        }

        unsigned operator()(const flow_node& ) const
        {
            return 1;
        }
    };

    class PriorityAssigner : public boost::static_visitor<>
    {
        const Depth& depth;
        double from, to;

        double average(double from, double to, unsigned k, unsigned m)
        { return (from * m + to * k) / (m + k); }

    public:
        PriorityAssigner(const Depth& depth, uint16_t from, uint16_t to)
            : depth(depth), from(from), to(to)
        { }

        void operator() (unexplored&)
        {
            // do nothing
        }

        void operator() (load_node& load)
        {
            for (auto& record : load.cases) {
                boost::apply_visitor(*this, record.second);
            }
        }

        void operator() (test_node& test)
        {
            double old_from = from, old_to = to;
            auto d = depth.at(test.id);
            double this_prio = average(from, to, d.negative, d.positive);

            // handle negative branch
            to = this_prio;
            boost::apply_visitor(*this, test.negative);
            to = old_to;

            // handle positive branch
            from = this_prio;
            boost::apply_visitor(*this, test.positive);
            from = old_from;

            test.prio = std::round(this_prio);
        }

        void operator() (flow_node& node)
        {
            node.prio = std::round(average(from, to, 1, 1));
        }
    };

public:

    PriorityUpdater(uint16_t from, uint16_t to)
        : from(from), to(to)
    { }

    void operator() (node& node)
    {
        DepthCounter dc{depth};
        boost::apply_visitor(dc, node);

        PriorityAssigner pa{depth, from, to};
        boost::apply_visitor(pa, node);
    }

};

FlowPtr TraceTree::lookup(const Packet& pkt) const
{
    return boost::apply_visitor(Impl::Lookup(pkt), *m_root);
}

std::unique_ptr<Tracer> TraceTree::augment()
{
    return std::unique_ptr<Tracer>(
            new Impl::TracerImpl(*m_root, m_backend, left_prio, right_prio)
        );
}

void TraceTree::update()
{
    Impl::PriorityUpdater pu(left_prio, right_prio);
    pu(*m_root);
}

void TraceTree::commit()
{
    m_backend.remove(oxm::field_set{});
    m_backend.barrier();
    Impl::Compiler compiler {m_backend};
    boost::apply_visitor(compiler, *m_root);
    m_backend.barrier();
}

TraceTree::TraceTree(Backend &backend,
                     uint16_t left_prio,
                     uint16_t right_prio)
    : m_backend(backend)
    , m_root(new node)
    , left_prio(left_prio)
    , right_prio(right_prio)
{ }

TraceTree::TraceTree(Backend &backend,
                     std::pair<uint16_t, uint16_t> prio_space)
    : TraceTree(backend, prio_space.first, prio_space.second)
{ }

TraceTree::~TraceTree() = default;

} // namespace maple
} // namespace runos
