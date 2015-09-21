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
#include "TraceTree.hh"
#include "Packet.hh"

/**
  @brief Represents installed or future flow on a switch.

  This class provides an abstract way for OFMessageHandler to read and modify
  flow information. Using this abstraction multiple applications can handle
  overlapped traffic classes without worring about how flow-mod will affect
  other applications.

  When flow has established you can use this class to monitor flow lifecycle and stats.
*/
class Flow: public QObject {
    Q_OBJECT
public:
    enum FlowState {
        // Table-miss packet-in received, but no flow mod issued yet
        New,
        // Flow installed on the switch
        Live,
        // Flow removed from the switch but can be reactivated 
        // Warning: only actual when TrackFlowRemoval is on.
        Shadowed,

        Destroyed
    };

    enum FlowFlags {
        TrackFlowRemoval = 1,
        TrackFlowStats = 2,
        Disposable = 4
    };

    /* This object should be constructed by Controller */
    Flow() = delete;
    Flow(Flow &other) = delete;
    Flow& operator=(const Flow& other) = delete;

    /**
     * Set specified set of flags to ON.
     * Other flags keeps unchanged.
     */
    void setFlags(FlowFlags flags);

    /**
     * Configures flow idle timeout which corresponds to OpenFlow idle timeout.
     * When called multiple times smallest value will be chosen.
     */
    uint16_t idleTimeout(uint16_t seconds = 0);

    /**
     * Setup the maximum flow lifetime like OpenFlow hard timeout.
     * Note that during this period flow entry can be removed due,
     * for example, idle timeout and then installed again without
     * OFMessageHandler processing.
     *
     * @param seconds Value to set. If multiple processors requests
     *   different values, smallest value will be set.
     * @return Current actual value of hard timeout.
     */
    void timeToLive(uint32_t seconds, bool force = false);

    /**
     * Returns raw packet field reader.
     *
     * It doesn't add new match field, so you SHOULD NOT use it
     * to decide what to do with a flow. Use it to collect stats,
     * to learn from slow path, and other situations that can not affect
     * decision and program's control flow.
     */
    Packet* pkt();

    //@{
    /**
     * Loads packet field and add it to flow match.
     * You should use this data to make flow decisions.
     */
    void load(of13::OXMTLV& tlv);
    // OpenFlow
    uint32_t        loadInPort();
    uint32_t        loadInPhyPort();
    uint64_t        loadMetadata();
    // Ethernet
    EthAddress      loadEthSrc();
    EthAddress      loadEthDst();
    uint16_t        loadEthType();
    // ARP
    uint16_t        loadARPOp();
    IPAddress       loadARPSPA();
    IPAddress       loadARPTPA();
    EthAddress      loadARPSHA();
    EthAddress      loadARPTHA();
    // IP
    IPAddress       loadIPv4Src();
    IPAddress       loadIPv4Dst();
    uint8_t         loadIPDSCP();
    uint8_t         loadIPECN();
    uint8_t         loadIPProto();
    //@}

    //@{
    /**
     * Tests if a packet field matches specified value.
     *
     * This allows to implement if-else statements on a packet fields.
     * Unlike `load`, this methods provide you a way to implement handlers
     * only for the particular value of packet.
     */
    // OpenFlow
    bool match(const of13::InPort& val);
    bool match(const of13::InPhyPort& val);
    bool match(const of13::Metadata& val);
    // Ethernet
    bool match(const of13::EthSrc& val);
    bool match(const of13::EthDst& val);
    bool match(const of13::EthType& val);
    // ARP
    bool match(const of13::ARPOp& val);
    bool match(const of13::ARPTHA& val);
    bool match(const of13::ARPTPA& val);
    bool match(const of13::ARPSHA& val);
    bool match(const of13::ARPSPA& val);
    // IP
    bool match(const of13::IPv4Src& val);
    bool match(const of13::IPv4Dst& val);
    bool match(const of13::IPDSCP& val);
    bool match(const of13::IPECN& val);
    bool match(const of13::IPProto& val);
    //@}

    void add_action(Action* action);
    void add_action(Action& action);

    /**
     * Checks if flow is outdated and should be destroyed.
     */
    bool outdated();

    //@{
    /** Getters */
    FlowState state() const;
    FlowFlags flags() const;
    Trace& trace();
    void toTrace(TraceEntry::Type type, of13::OXMTLV* tlv);
    ActionList& get_action();
    //@}

signals:
    void stateChanged(FlowState new_state, FlowState old_state);

protected:
    friend class SwitchScope;
    friend class TraceTreeNode;
    friend class FlowManager;
    friend class PathVerifier;

    Flow(Packet* pkt, QObject* parent = 0);
    ~Flow();

    void initFlowMod(of13::FlowMod* fm);
    void initPacketOut(of13::PacketOut* po);

    void setLive();
    void setShadow();
    void setDestroy();
    uint16_t hardTimeout();

private:
    struct FlowImpl *m;
};
