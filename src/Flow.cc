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

#include "Flow.hh"

#include <chrono>
#include "Match.hh"

using std::chrono::time_point;
using std::chrono::steady_clock;
using std::chrono::duration;

typedef time_point<steady_clock> timepoint_t;
typedef Flow::FlowState FlowState;
typedef Flow::FlowFlags FlowFlags;

struct FlowImpl {
    // Initialization
    Packet*               pkt;

    // State
    FlowState             state;
    FlowFlags             flags;
    Trace                 trace;

    // Timeouts
    timepoint_t           live_until;
    uint16_t              idle_timeout;

    // Decisions
    ActionList actions;

    FlowImpl(Packet* pkt_)
        : pkt(pkt_),
          state(Flow::New),
          flags((FlowFlags) 0),
          live_until(timepoint_t::max())
    { }
};

Flow::Flow(Packet* pkt, QObject* parent)
    : QObject(parent),
      m(new FlowImpl(pkt))
{ }

Flow::~Flow()
{ delete m; }

FlowState Flow::state() const
{ return m->state; }

FlowFlags Flow::flags() const
{ return m->flags; }

void Flow::setFlags(FlowFlags flags)
{ m->flags = static_cast<FlowFlags>(m->flags | flags); }

Trace& Flow::trace()
{ return m->trace; }

Packet* Flow::pkt()
{ return m->pkt; }

ActionList& Flow::get_action()
{ return m->actions; }

bool Flow::outdated()
{ return steady_clock::now() > m->live_until; }

void Flow::add_action(Action* action)
{ m->actions.add_action(action); }

void Flow::add_action(Action& action)
{ m->actions.add_action(action); }

void Flow::setLive()
{
    auto old_state = m->state;
    if (m->state == Live)
        return;

    m->state = Live;
    m->pkt = nullptr;

    emit stateChanged(m->state, old_state);
}

void Flow::setShadow()
{
    auto old_state = m->state;
    if (m->state == Shadowed)
        return;

    m->state = Shadowed;

    emit stateChanged(m->state, old_state);
}

void Flow::setDestroy()
{
    auto old_state = m->state;
    if (m->state == Destroyed)
        return;

    m->state = Destroyed;
    m->pkt = nullptr;

    emit stateChanged(m->state, old_state);
}

uint16_t Flow::idleTimeout(uint16_t seconds)
{
    if (seconds == 0) // infinite
        return m->idle_timeout;
    if (m->idle_timeout == 0)
        return m->idle_timeout = seconds;
    return m->idle_timeout = std::min(seconds, m->idle_timeout);
}

void Flow::timeToLive(uint32_t seconds, bool force)
{
    auto timepoint = (seconds > 0) ?
            (steady_clock::now() + std::chrono::seconds(seconds)) :
            timepoint_t::max();

    if (force) {
        m->live_until = timepoint;
    } else if (m->state == New && timepoint < m->live_until) {
        m->live_until = timepoint;
    }
}

uint16_t Flow::hardTimeout()
{
    if (m->live_until == timepoint_t::max())
        return 0;

    duration<double> ttl = m->live_until - steady_clock::now();
    double hard_timeout = ceil(ttl.count());
    if (hard_timeout > UINT16_MAX)
        return UINT16_MAX;
    else
        return (uint16_t) hard_timeout;
}

void Flow::initFlowMod(of13::FlowMod *fm)
{
    fm->idle_timeout(m->idle_timeout);
    fm->hard_timeout(hardTimeout());

    uint16_t flags = 0;

    // TODO: make it optional or debug mode only
    flags |= of13::OFPFF_CHECK_OVERLAP;

    if (m->flags & TrackFlowRemoval)
        flags |= of13::OFPFF_SEND_FLOW_REM;

    fm->flags(flags);

    of13::ApplyActions applyActions;
    applyActions.actions(m->actions);
    of13::InstructionSet instructions;
    instructions.add_instruction(applyActions);
    fm->instructions(instructions);
}

void Flow::initPacketOut(of13::PacketOut *po)
{
    po->in_port(m->pkt->readInPort());
    po->actions(m->actions);
}

void Flow::load(of13::OXMTLV& tlv)
{
    m->pkt->read(tlv);
    m->trace.push_back(TraceEntry(TraceEntry::Load, tlv));
}

#define LOAD_IMPL(field) \
    decltype(of13::field().value()) Flow::load##field() \
    { \
        auto value = m->pkt->read##field(); \
        m->trace.push_back(TraceEntry(TraceEntry::Load, of13::field(value))); \
        return value; \
    }

#define MATCH_IMPL(field) \
    bool Flow::match(const of13::field& val) \
    { \
        bool res = oxm_match<of13::field>(val, m->pkt->read##field()); \
        m->trace.push_back(TraceEntry(TraceEntry::Test, val, res)); \
        return res; \
    }

LOAD_IMPL(InPort);
LOAD_IMPL(InPhyPort);
LOAD_IMPL(Metadata);

LOAD_IMPL(EthSrc);
LOAD_IMPL(EthDst);
LOAD_IMPL(EthType);

LOAD_IMPL(ARPOp);
LOAD_IMPL(ARPSPA);
LOAD_IMPL(ARPSHA);
LOAD_IMPL(ARPTHA);
LOAD_IMPL(ARPTPA);

LOAD_IMPL(IPv4Src);
LOAD_IMPL(IPv4Dst);
LOAD_IMPL(IPECN);
LOAD_IMPL(IPDSCP);
LOAD_IMPL(IPProto);

MATCH_IMPL(InPort);
MATCH_IMPL(InPhyPort);
MATCH_IMPL(Metadata);

MATCH_IMPL(EthSrc);
MATCH_IMPL(EthDst);
MATCH_IMPL(EthType);

MATCH_IMPL(ARPOp);
MATCH_IMPL(ARPSPA);
MATCH_IMPL(ARPSHA);
MATCH_IMPL(ARPTHA);
MATCH_IMPL(ARPTPA);

MATCH_IMPL(IPv4Src);
MATCH_IMPL(IPv4Dst);
MATCH_IMPL(IPDSCP);
MATCH_IMPL(IPECN);
MATCH_IMPL(IPProto);
