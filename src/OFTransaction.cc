#include "OFTransaction.hh"

OFTransaction::OFTransaction(uint32_t xid, QObject *parent)
    : m_xid(xid), QObject(parent)
{ }

void OFTransaction::request(shared_ptr <OFConnection> ofconn, OFMsg *msg)
{
    msg->xid(m_xid);
    uint8_t* buffer = msg->pack();
    ofconn->send(buffer, msg->length());
    delete[] buffer;
}