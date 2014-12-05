#pragma once

#include "Common.hh"
#include "OFMsgUnion.hh"

/**
* Used to serve response from the switch.
*/
class OFTransaction : public QObject {
    Q_OBJECT
public:
    explicit OFTransaction(uint32_t xid, QObject *parent = 0);

    /**
    * Sends an OpenFlow message.
    * @param ofconn Connection to use.
    * @param msg OpenFlow Message (xid will be overwritten).
    */
    void request(OFConnection* ofconn, OFMsg* msg);

signals:
    /**
    * New response message received on this transaction.
    *
    * @param ofconn     OpenFlow Connection.
    * @param reply        Unpacked OpenFlow message.
    */
    void response(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> reply);

    /**
    * Indicates about error on the switch.
    *
    * @param ofconn     OpenFlow Connection.
    * @param error      Information about error.
    */
    void error(OFConnection* ofconn, std::shared_ptr<OFMsgUnion> error);

private:
    uint32_t m_xid;
};

