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

#include "OFMsgUnion.hh"

OFMsgUnion::OFMsgUnion()
    : m_base(nullptr)
{
}

OFMsgUnion::OFMsgUnion(uint8_t type, void *data, size_t len)
{
    // Construct message object
    switch (type) {
    case of13::OFPT_ERROR:
        m_base = new (&error) of13::Error; break;
    case of13::OFPT_FEATURES_REPLY:
        m_base = new (&featuresReply) of13::FeaturesReply; break;
    case of13::OFPT_GET_CONFIG_REPLY:
        m_base = new (&getConfigReply) of13::GetConfigReply; break;
    case of13::OFPT_PACKET_IN:
        m_base = new (&packetIn) of13::PacketIn; break;
    case of13::OFPT_FLOW_REMOVED:
        m_base = new (&flowRemoved) of13::FlowRemoved; break;
    case of13::OFPT_PORT_STATUS:
        m_base = new (&portStatus) of13::PortStatus; break;
    case of13::OFPT_BARRIER_REPLY:
        m_base = new (&barrierReply) of13::BarrierReply; break;
    case of13::OFPT_QUEUE_GET_CONFIG_REPLY:
        m_base = new (&queueGetConfigReply) of13::QueueGetConfigReply; break;
    case of13::OFPT_ROLE_REPLY:
        m_base = new (&roleReply) of13::RoleReply; break;
    case of13::OFPT_GET_ASYNC_REPLY:
        m_base = new (&getAsyncReply) of13::GetAsyncReply; break;
    case of13::OFPT_MULTIPART_REPLY:
        m_base = new (&multipartReply) of13::MultipartReply;
        break;
    default:
        throw OFMsgUnhandledType(type);
    }

    if (m_base->unpack(static_cast<uint8_t*>(data)) != 0) {
        throw OFMsgParseError();
    }

    if (type == of13::OFPT_MULTIPART_REPLY)
        reparseMultipartReply(data, len);
}

void OFMsgUnion::reparseMultipartReply(void* data, size_t len)
{
    uint16_t type = multipartReply.mpart_type();

    m_base->~OFMsg();
    m_base = nullptr;

    switch (type) {
    case of13::OFPMP_DESC:
        m_base = new (&multipartReplyDesc) of13::MultipartReplyDesc; break;
    case of13::OFPMP_FLOW:
        m_base = new (&multipartReplyFlow) of13::MultipartReplyFlow; break;
    case of13::OFPMP_AGGREGATE:
        m_base = new (&multipartReplyAggregate) of13::MultipartReplyAggregate;
    case of13::OFPMP_TABLE:
        m_base = new (&multipartReplyTable) of13::MultipartReplyTable; break;
    case of13::OFPMP_TABLE_FEATURES:
        m_base = new (&multipartReplyTableFeatures) of13::MultipartReplyTableFeatures; break;
    case of13::OFPMP_PORT_STATS:
        m_base = new (&multipartReplyPortStats) of13::MultipartReplyPortStats; break;
    case of13::OFPMP_PORT_DESC:
        m_base = new (&multipartReplyPortDescription) of13::MultipartReplyPortDescription; break;
    case of13::OFPMP_QUEUE:
        m_base = new (&multipartReplyQueue) of13::MultipartReplyQueue; break;
    case of13::OFPMP_GROUP:
        m_base = new (&multipartReplyGroup) of13::MultipartReplyGroup; break;
    case of13::OFPMP_GROUP_DESC:
        m_base = new (&multipartReplyGroupDesc) of13::MultipartReplyGroupDesc; break;
    case of13::OFPMP_GROUP_FEATURES:
        m_base = new (&multipartReplyGroupFeatures) of13::MultipartReplyGroupFeatures; break;
    case of13::OFPMP_METER:
        m_base = new (&multipartReplyMeter) of13::MultipartReplyMeter; break;
    case of13::OFPMP_METER_CONFIG:
        m_base = new (&multipartReplyMeterConfig) of13::MultipartReplyMeterConfig; break;
    case of13::OFPMP_METER_FEATURES:
        m_base = new (&multipartReplyMeterFeatures) of13::MultipartReplyMeterFeatures; break;
    default:
        LOG(WARNING) << "Unknown multipart type " << (int) type;
        return;
    }

    m_base->unpack(static_cast<uint8_t*>(data));
}

//OFMsgUnion::OFMsgUnion(OFMsgUnion &&other)
//{
//    if (m_base) m_base->~OFMsg();
//
//    if (other.m_base != nullptr) {
//        // Hackish move constructor
//        memcpy(this, &other, sizeof(*this));
//        this->m_base = reinterpret_cast<OFMsg*>(this + ((uint8_t*)other.base() - (uint8_t*) &other));
//        other.m_base = nullptr;
//    } else {
//        m_base = nullptr;
//    }
//}

OFMsgUnion::~OFMsgUnion()
{
    if (m_base) m_base->~OFMsg();
}

static struct Init {
    Init() {
        qRegisterMetaType< std::shared_ptr<OFMsgUnion> >();
    }
} init;
