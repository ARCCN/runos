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

#include <memory>
#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>

#include "Flow.hh"
#include "Backend.hh"
#include "Tracer.hh"

namespace runos {

class Packet;

namespace maple {

class TraceTree {
public:

    TraceTree(Backend& backend, FlowPtr barrier);
    ~TraceTree();

    FlowPtr lookup(const Packet& pkt) const;
    std::unique_ptr<Tracer> augment();

    void commit();
    void gc();

protected:
    struct unexplored;
    struct flow_node;
    struct test_node;
    struct load_node;

    using node =
        boost::variant< unexplored
                      , flow_node
                      , boost::recursive_wrapper<test_node>
                      , boost::recursive_wrapper<load_node> >;

    struct Impl;

    Backend& m_backend;
    FlowPtr m_barrier;
    std::unique_ptr<node> m_root;
};

} // namespace maple
} // namespace runos
