#pragma once

#include "Common.hh"
#include "Application.hh"
#include "OFMessageHandler.hh"
#include "OFTransaction.hh"

/**
* Implements OpenFlow 1.3 controller.
*/
class Controller : public Application {
    Q_OBJECT
public:
    APPLICATION(Controller);

    void init(AppProvider* provider, const Config& config) override;
    void startUp(AppProvider* provider) override;

    /**
    * Registers new message handler for each worker thread.
    * Used for performance-critical message processing, such as packet-in's.
    */
    void registerHandler(OFMessageHandlerFactory* hf);

    /**
     * Allocate unique OFMsg::xid and return's a wrapper class
     * to handle this transaction responses.
     * You can use this to make non-overlapped at time queries.
     */
    OFTransaction* registerStaticTransaction();

signals:

    /**
    * New switch connection is ready to use.
    */
    void switchUp(shared_ptr<OFConnection> ofconn, of13::FeaturesReply fr);

    /**
    * Switch reports about port status changes.
    */
    void portStatus(shared_ptr<OFConnection> ofconn, of13::PortStatus ps);

    /**
    * Switch connection failed or closed.
    * @param ofconn OpenFlow connection. You should drop references to it to free memory.
    */
    void switchDown(shared_ptr<OFConnection> ofconn);
private:
    class ControllerImpl *impl;
};

Q_DECLARE_METATYPE(of13::FeaturesReply)
Q_DECLARE_METATYPE(of13::PortStatus)
Q_DECLARE_METATYPE(shared_ptr<of13::Error>)
Q_DECLARE_METATYPE(shared_ptr<OFConnection>)
