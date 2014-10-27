#pragma once

#include "Common.hh"

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
    void request(shared_ptr<OFConnection> ofconn, OFMsg* msg);

signals:
    /**
    * New response message received on this transaction.
    *
    * Note, that you get ownership of msg object and raw_msg array
    * and should delete them manually.
    *
    * @param ofconn     OpenFlow Connection.
    * @param msg        Unpacked OpenFlow message.
    * @param raw_msg    Raw message for MultipartResponse, nullptr otherwise.
    */
    void response(shared_ptr<OFConnection> ofconn, OFMsg* msg, uint8_t* raw_msg);

    /**
    * Indicates about error on the switch.
    *
    * @param ofconn     OpenFlow Connection.
    * @param error      Information about error.
    */
    void error(shared_ptr<OFConnection> ofconn, shared_ptr<of13::Error> error);

private:
    uint32_t m_xid;
};

