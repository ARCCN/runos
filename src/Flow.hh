#pragma once

#include "Common.hh"
#include <tins/ethernetII.h>

/**
* @brief Represents installed or future flow on a switch.
*
* This class provides an abstract way for OFMessageHandler to read and modify flow information.
* Using this abstraction multiple applications can handle overlapped traffic classes without worring
* about how flow-mod will affect other applications.
*
* When flow has established you can use this class to monitor flow lifecycle and stats.
*
* This class currently is in active development and may be changed significantly in the future.
*
*/
class Flow: public QObject {
    Q_OBJECT
public:
    /* This object should be constructed by Controller */
    Flow() = delete;
    Flow(Flow &other) = delete;
    Flow& operator=(const Flow& other) = delete;
    friend class ControllerImpl;

    /*
        New: table-miss packet-in received, but no flow mod issued yet
        Live: flow installed on the switch
        Dead: flow removed from the switch

        New --------> Live --------> Dead ------> (destruction)
                       ^               |
                       |               |
                       +---------------+
     */
    enum FlowState {
        New, Live, Dead
    };

    enum FlowFlags {
        TrackFlowState = 1,
        TrackFlowStats = 2
    };

    void setFlags(FlowFlags flags);

    /**
    * Use it with restrictions described in the `read` method.
    * @return Raw packet-in data limited to `miss_send_len` bytes.
    */
    const void* packetData() const;

    /**
    * Use it with restrictions described in the `read` method.
    * @return Raw packet-in data length.
    */
    size_t packetLen() const;

    /*
     * Extracts packet field, applying all modifications.
     *  This method doesn't add new match field, so you SHOULD NOT use it
     *  to decide what to do with this flow. Use it to collect stats,
     *  to learn from slow path and other situations that can not affect
     *  decision and program's control flow.
     */
    void read(of13::InPort &in_port);
    void read(of13::EthSrc &eth_src);
    void read(of13::EthDst &eth_dst);
    void read(of13::EthType &eth_type);

    /*
     * Loads packet field and add it to flow match.
     *  In the future, it will accept masks to load only subset of fields.
     *  You should use this data to make decisions about flow.
    */
    void load(of13::InPort &in_port);
    void load(of13::EthSrc &eth_src);
    void load(of13::EthDst &eth_dst);
    void load(of13::EthType &eth_type);

    /**
     * Tests if a packet field matches specified value.
     *  This allows you to implement if-else checks on a packet fields.
     *  Unlike `load`, this methods provide you a way to implement handlers
     *  for the particular value of packet field, but delegate decision
     *  logic for all others values.
     */
    bool test(of13::InPort in_port);
    bool test(of13::EthSrc eth_src);
    bool test(of13::EthDst eth_dst);
    bool test(of13::EthType eth_type);

    void forward(uint32_t port_no);
    void mirror(uint32_t port_no);
    //void modify(const of13::OXMTLV* tlv);

    /* Getters */
    inline FlowState state() const { return m_state; }

signals:
    void statsUpdated();
    void flowInstalled();
    void flowRemoved();

protected:
    Flow(shared_ptr<of13::PacketIn> pi);

    void setInstalled(uint64_t cookie);
    void setRemoved();

    of13::FlowMod compile();
private:
    // TODO: short task: test if old connection is usable after control link fails
    // TODO: long task: replace it with switch abstraction
    shared_ptr<of13::PacketIn>       m_pi;
    Tins::EthernetII                 m_packet;

    FlowState             m_state;
    FlowFlags             m_flags;

    /* Generated cookie for a installed flow */
    uint64_t              m_cookie;
    /* Pending match */
    of13::Match           m_match;
    uint32_t              m_forward_port;
    std::vector<uint32_t> m_mirror_ports;
};

