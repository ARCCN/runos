#pragma once

#include "Flow.hh"

/**
* Handles performance-critical messages from the OpenFlow switch.
*/
class OFMessageHandler {
public:
    enum Action {
        /** Stop flow processing step and forward packet to a specified port. */
        Stop,
        /** Continue flow processing with next processors. */
        Continue
    };

    /**
    * Handles creation of a new flow on a switch.
    * Typically called when table-miss packet-in message received.
    *
    * @param ofconn Connection where the message was received.
    * @param flow   Flow interface used to read and modify flow data.
    * @return Whether or not flow processing should be stopped.
    */
    virtual Action processMiss(shared_ptr<OFConnection> ofconn, Flow* flow) = 0;

    virtual ~OFMessageHandler() { }
};

/**
* Factory for OFMessageHandler. Used to create processing pipelines on multiple threads.
*/
class OFMessageHandlerFactory {
public:
    /**
    * Handler id, passed to the isPrereq() and isPostreq().
    */
    virtual std::string orderingName() const = 0;

    /**
    * @return True, if handler `name` is prerequirement for this handler.
    */
    virtual bool isPrereq(const std::string &name) const { return false; }

    /**
    * @returns True, if processor `stageId` is postrequirement for this handler.
    */
    virtual bool isPostreq(const std::string &name) const { return false; }

    /**
    * @return Newly-created OFMessageHandler. Membership moves to the caller.
    */
    virtual OFMessageHandler* makeOFMessageHandler() = 0;
};