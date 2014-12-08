#pragma once

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "OFTransaction.hh"
#include "Rest.hh"

/**
* Implements OpenFlow 1.3 controller.
*/
class Controller : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(Controller, "controller")
public:
    Controller();
    ~Controller();

    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

    /**
    * Registers new message handler for each worker thread.
    * Used for performance-critical message processing, such as packet-in's.
    */
    void registerHandler(OFMessageHandlerFactory* hf);

    /**
     * Allocate unique OFMsg::xid and return's a wrapper class
     * to handle this transaction responses.
     * You can use this to make non-overlapped at time queries.
     *
     * @param caller Parent object.
     */
    OFTransaction* registerStaticTransaction(Application* caller);

signals:

    /**
    * New switch connection is ready to use.
    */
    void switchUp(OFConnection* ofconn, of13::FeaturesReply fr);

    /**
    * Switch reports about port status changes.
    */
    void portStatus(OFConnection* ofconn, of13::PortStatus ps);

    /**
    * Switch connection failed or closed.
    * @param ofconn OpenFlow connection. You should drop references to it to free memory.
    */
    void switchDown(OFConnection* ofconn);

    /**
     * Controller information is generated in startUp() method.
     * Controller Manager (see ControllerApp.hh) catches this signal
     * @param address Controller's address
     * @param port Controller's port
     * @param nthreads Working threads
     * @param secure Secure
     */
    void sendInfo(std::string address, int port, int nthreads, bool secure);
private:
    class ControllerImpl *impl;
};
