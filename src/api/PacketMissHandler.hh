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

#include <functional> // function

#include "SwitchConnectionFwd.hh"
#include "FlowFwd.hh"
#include "Decision.hh"
#include "Common.hh"

namespace runos {

class Packet;

/**
* Handles creation of a new flow on a switch.
* Typically called when table-miss packet-in message received.
*
* @param pkt    An interface for reading and writing packet
*               header fields.
* @param flow   An interface for configuring and tracking
*               flow state.
* @return Whether or not flow processing should be stopped.
*/
using PacketMissHandler =
    std::function< Decision(Packet&, FlowPtr, Decision decision) >;

/**
* Factory for OFMessageHandler. Used to create processing pipelines on multiple threads.
*/
using PacketMissHandlerFactory =
    std::function< PacketMissHandler(SwitchConnectionPtr) >;


// TODO : handle this
using FloodImplementation =
	std::function< Action*(uint64_t dpid) >;

}
