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
    virtual Action processMiss(OFConnection* ofconn, Flow* flow) = 0;

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
    * @return True if handler `name` is prerequirement for this handler.
    */
    virtual bool isPrereq(const std::string &name) const { return false; }

    /**
    * @returns True if processor `stageId` is postrequirement for this handler.
    */
    virtual bool isPostreq(const std::string &name) const { return false; }

    /**
    * @return Newly-created OFMessageHandler. Membership moves to the caller.
    */
    virtual OFMessageHandler* makeOFMessageHandler() = 0;
};
