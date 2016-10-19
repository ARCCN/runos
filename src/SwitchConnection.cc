#include "SwitchConnection.hh"

#include <fluid/OFConnection.hh>
#include <fluid/ofcommon/msg.hh>

using fluid_base::OFConnection;

namespace runos {

bool SwitchConnection::alive() const
{
    return m_ofconn ? m_ofconn->is_alive() : false;
}

uint8_t SwitchConnection::version() const
{ 
    return m_ofconn ? m_ofconn->get_version() : 0;
}

void SwitchConnection::send(const fluid_msg::OFMsg& cmsg)
{
    if (not m_ofconn || not m_ofconn->is_alive()) return;

    auto& msg = const_cast<fluid_msg::OFMsg&>(cmsg);
    auto buf = msg.pack();
    m_ofconn->send(buf, msg.length());
    fluid_msg::OFMsg::free_buffer(buf);
}

void SwitchConnection::close()
{ 
    if (m_ofconn) m_ofconn->close(), m_ofconn = nullptr;
}

SwitchConnection::SwitchConnection(OFConnection* ofconn, uint64_t dpid)
    : m_dpid(dpid), m_ofconn(ofconn)
{ }

} // namespace runos
