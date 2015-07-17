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

#include "Common.hh"
#include <list>
#include <stack>
#include <ostream>

class Flow;
class Packet;

struct TraceEntry {
    enum Type {
        Test = 1,
        Load = 2
    } type;
    of13::OXMTLV* tlv;
    bool outcome;

    // ctor
    TraceEntry(Type type_, const of13::OXMTLV& tlv_, bool outcome_ = false)
            : type(type_), tlv(tlv_.clone()), outcome(outcome_) {}
    // default ctor
    TraceEntry() = delete;
    // copy ctor
    TraceEntry(const TraceEntry& o) = delete;
    // move ctor
    TraceEntry(TraceEntry&& o) {
        type = o.type;
        tlv = o.tlv;
        outcome = o.outcome;
        o.tlv = nullptr;
    }
    // dtor
    ~TraceEntry() { delete tlv; }
};

typedef std::vector<TraceEntry> Trace;

class TraceTreeNode {
public:
    enum Type {
        Empty = 0,
        Test  = 1,
        Load  = 2,
        Leaf  = 3
    };

    Type m_type;
    Type type();
    void makeEmpty();
    void makeTest(of13::OXMTLV* value);
    void makeLoad(of13::OXMTLV* value);
    void makeLeaf(Flow* flow, of13::FlowMod* fm_base);

    struct TestData {
        of13::OXMTLV*   value;
        TraceTreeNode*  positiveChild;
        TraceTreeNode*  negativeChild;
    };

    struct LoadData {
        of13::OXMTLV*   value;
        TraceTreeNode*  child;
        LoadData*       next;
    };

    struct LeafData {
        Flow* flow;
        of13::FlowMod* fm;
    };

    union {
        TestData test;
        LoadData load;
        LeafData leaf;
    };

    TraceTreeNode* move(TraceEntry& op);
    LeafData* find(Packet* pkt);
    Flow* find (uint64_t cookie);
    std::ostream& dump(std::ostream& out, size_t level);

    TraceTreeNode();
    ~TraceTreeNode();
};

class TraceTree : public QObject {
    Q_OBJECT
public:
    TraceTree();

    Flow* find(uint64_t cookie);
    TraceTreeNode::LeafData* find(Packet* pkt);
    void augment(Flow* flow, of13::FlowMod* fm_base);
    void cleanFlowTable(OFConnection* ofconn);
    unsigned buildFlowTable(OFConnection* ofconn);
    static bool isTableMiss(of13::PacketIn& pi);
    std::ostream& dump(std::ostream& out);
private:
    TraceTreeNode root;

    void buildFlowTableStep(struct BuildFTContext* ctx);
};
