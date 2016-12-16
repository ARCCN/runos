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

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFTransaction.hh"

#include "api/PacketMissHandler.hh"
#include "SwitchConnectionFwd.hh"

using runos::SwitchConnectionPtr;
using runos::PacketMissHandlerFactory;
using runos::FloodImplementation;

/**
* Implements OpenFlow 1.3 controller.
*/
class Controller : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(Controller, "controller")
public:
    ~Controller();

    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

    /**
    * Registers new message handler for each worker thread.
    * Used for performance-critical message processing, such as packet-in's.
    */
    void registerHandler(const char* name, PacketMissHandlerFactory factory);

    /**
     *  Get number of Maple's table
     */
    uint8_t handler_table() const;

    /**
     * If your application needed in own table, use this method
     */
    uint8_t reserveTable();

    /**
      * Deleting all TraceTrees
      */

    void invalidateTraceTree();

    /**
     * Allocate unique OFMsg::xid and return's a wrapper class
     * to handle this transaction responses.
     * You can use this to make non-overlapped at time queries.
     *
     * @param caller Parent object.
     */
    OFTransaction* registerStaticTransaction(Application* caller);


    /**
    * register handler broadcast packets
    */
    void registerFlood(FloodImplementation flood);

signals:

    /**
    * New switch connection is ready to use.
    */
    void switchUp(SwitchConnectionPtr conn, of13::FeaturesReply fr);

    /**
    * Switch reports about port status changes.
    */
    void portStatus(SwitchConnectionPtr ofconn, of13::PortStatus ps);

    /**
    * Switch connection failed or closed.
    * @param ofconn OpenFlow connection. You should drop references to it to free memory.
    */
    void switchDown(SwitchConnectionPtr ofconn);

    /**
      * flow removed message, that recieved Controller.
      */
    void flowRemoved(SwitchConnectionPtr ofconnl, of13::FlowRemoved fr);

private:
    std::unique_ptr<class ControllerImpl> impl;
};
