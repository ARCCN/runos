#include <QtDebug>
#include <tins/rawpdu.h>

#include "Flow.hh"

Flow::Flow(shared_ptr<of13::PacketIn> pi) :
    m_pi(pi),
    m_packet(Tins::RawPDU( (uint8_t*)packetData(), packetLen() )
                         .to<Tins::EthernetII>()),
    m_state(New)
{
}

const void* Flow::packetData() const
{
    return m_pi->data();
}

size_t Flow::packetLen() const
{
    return m_pi->data_len();
}

void Flow::setInstalled(uint64_t cookie)
{
    m_packet = Tins::EthernetII();
    m_pi.reset();

    m_state = Live;
    m_cookie = cookie;

    emit flowInstalled();
}

void Flow::setRemoved()
{
    m_state = Dead;
    m_cookie = 0;

    emit flowRemoved();
}


void Flow::read(of13::InPort &in_port)
{
    in_port.value(m_pi->match().in_port()->value());
}

void Flow::load(of13::InPort &in_port)
{
    read(in_port);
    if (m_match.in_port() == 0)
        m_match.add_oxm_field(in_port.clone());
}

void Flow::read(of13::EthSrc &eth_src)
{
    eth_src.value(EthAddress(m_packet.src_addr().begin()));
}

void Flow::load(of13::EthSrc &eth_src)
{
    // TODO: implement masking
    assert(!eth_src.has_mask());

    read(eth_src);
    if (m_match.eth_src() == nullptr)
        m_match.add_oxm_field(eth_src.clone());
}

void Flow::read(of13::EthDst &eth_dst)
{
    eth_dst.value(EthAddress(m_packet.dst_addr().begin()));
}

void Flow::load(of13::EthDst &eth_dst)
{
    // TODO: implement masking
    assert(!eth_dst.has_mask());

    read(eth_dst);
    if (m_match.eth_dst() == nullptr)
        m_match.add_oxm_field(eth_dst.clone());
}

void Flow::forward(uint32_t port_no)
{
    m_forward_port = port_no;
}

void Flow::mirror(uint32_t port_no)
{
    m_mirror_ports.push_back(port_no);
}

of13::FlowMod Flow::compile()
{
    ActionSet actions;
    actions.add_action(new of13::OutputAction(m_forward_port, 0));
    for (auto port : m_mirror_ports)
        actions.add_action(new of13::OutputAction(port, 0));

    of13::WriteActions writeActions;
    writeActions.actions(actions);

    of13::FlowMod fm;
    fm.match(m_match);
    fm.add_instruction(writeActions);

    return fm;
}