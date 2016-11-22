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
};

struct TraceTree::test_node {
    oxm::field<> need;
    node positive;
    node negative;
};

struct TraceTree::load_node {
    oxm::mask<> mask;
    std::unordered_map< bits<>, node >
        cases;
};

struct TraceTree::Impl {
    class Lookup;
    class Compiler;
    class TracerImpl;
};

class TraceTree::Impl::Compiler : public boost::static_visitor<>
{
    Backend& backend;
    FlowPtr barrier;
    oxm::field_set match;
    uint16_t priority{1};

public:
    Compiler(Backend& backend, FlowPtr barrier)
        : backend(backend)
        , barrier(barrier)
    { }

    void operator()(unexplored&)
    {
        // do nothing
    }

    void operator()(test_node& test)
    {
        boost::apply_visitor(*this, test.negative);
        match.modify(test.need);
        backend.install(priority++, match, barrier);
        boost::apply_visitor(*this, test.positive);
        match.erase(oxm::mask<>(test.need));
    }

    void operator()(load_node& load)
    {
        auto type = load.mask.type();

        for (auto& record : load.cases) {
            match.modify((type == record.first) & load.mask);
            boost::apply_visitor(*this, record.second);
            match.erase(load.mask);
        }
    }

    void operator()(flow_node& node)
    {
        if (auto flow = node.flow.lock())
            backend.install(priority++, match, flow);
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

    node* node_ptr() { return path.back(); }
    void node_push(node* n) { path.push_back(n); }

public:
    explicit TracerImpl(node& root)
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
    }

    void test(oxm::field<> pred, bool ret) override
    {
        if (boost::get<unexplored>(node_ptr())) {
            *node_ptr() = test_node{
                pred, unexplored(), unexplored{}
            };

            node_push( ret ?
                &boost::get<test_node>(node_ptr())->positive :
                &boost::get<test_node>(node_ptr())->negative );

        } else if (test_node* test = boost::get<test_node>(node_ptr())) {
            if (test->need != pred)
                RUNOS_THROW(inconsistent_trace());
            node_push( ret ? &test->positive : &test->negative );
        } else {
            RUNOS_THROW(inconsistent_trace());
        }
    }

    void finish(FlowPtr new_flow) override
    {
        if (boost::get<unexplored>(node_ptr())) {
            *node_ptr() = flow_node{ new_flow };
        } else if (flow_node* leaf = boost::get<flow_node>(node_ptr())) {
            leaf->flow = new_flow;
        } else {
            RUNOS_THROW(inconsistent_trace());
        }

        node_push(nullptr);
    }
};

FlowPtr TraceTree::lookup(const Packet& pkt) const
{
    return boost::apply_visitor(Impl::Lookup(pkt), *m_root);
}

std::unique_ptr<Tracer> TraceTree::augment()
{
    return std::unique_ptr<Tracer>(new Impl::TracerImpl(*m_root));
}

void TraceTree::commit()
{
    m_backend.remove(oxm::field_set{});
    m_backend.barrier();
    Impl::Compiler compiler {m_backend, m_barrier};
    boost::apply_visitor(compiler, *m_root);
    m_backend.barrier();
}

TraceTree::TraceTree(Backend &backend, FlowPtr barrier)
    : m_backend(backend)
    , m_barrier(barrier)
    , m_root(new node)
{ }

TraceTree::~TraceTree() = default;

} // namespace maple
} // namespace runos
