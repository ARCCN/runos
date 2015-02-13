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

#include <exception>
#include "Common.hh"

struct OFMsgParseError : std::exception { };

class OFMsgUnhandledType : std::exception {
public:
    explicit OFMsgUnhandledType(uint8_t type)
        : std::exception(), m_type(type)
    { }
    uint8_t msg_type() const { return m_type; }
private:
    uint8_t m_type;
};

/**
 * @brief Wraps all OFMsg types in a single-sized class.
 * This allows to allocate dynamic-typed OFMsg on stack.
 */
struct OFMsgUnion {
    union {
        of13::Error error;
        of13::FeaturesReply featuresReply;
        of13::GetConfigReply getConfigReply;
        of13::PacketIn packetIn;
        of13::FlowRemoved flowRemoved;
        of13::PortStatus portStatus;
        of13::MultipartReply multipartReply;
        of13::BarrierReply barrierReply;
        of13::QueueGetConfigReply queueGetConfigReply;
        of13::RoleReply roleReply;
        of13::GetAsyncReply getAsyncReply;

        // concrete mpart reply
        of13::MultipartReplyDesc multipartReplyDesc;
        of13::MultipartReplyFlow multipartReplyFlow;
        of13::MultipartReplyAggregate multipartReplyAggregate;
        of13::MultipartReplyTable multipartReplyTable;
        of13::MultipartReplyTableFeatures multipartReplyTableFeatures;
        of13::MultipartReplyPortStats multipartReplyPortStats;
        of13::MultipartReplyPortDescription multipartReplyPortDescription;
        of13::MultipartReplyQueue multipartReplyQueue;
        of13::MultipartReplyGroup multipartReplyGroup;
        of13::MultipartReplyGroupDesc multipartReplyGroupDesc;
        of13::MultipartReplyGroupFeatures multipartReplyGroupFeatures;
        of13::MultipartReplyMeter multipartReplyMeter;
        of13::MultipartReplyMeterConfig multipartReplyMeterConfig;
        of13::MultipartReplyMeterFeatures multipartReplyMeterFeatures;
    };

    /** Construct empty message (base() == nullptr) */
    OFMsgUnion();
    
    /** Construct from data */
    OFMsgUnion(uint8_t type, void* data, size_t len);
    //OFMsgUnion(OFMsgUnion&& other);
    ~OFMsgUnion();

    OFMsg* base() const { return m_base; }

private:
    OFMsg* m_base;
    void reparseMultipartReply(void* data, size_t len);
};

Q_DECLARE_METATYPE(std::shared_ptr<OFMsgUnion>)
