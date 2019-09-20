/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include <runos/core/exception.hpp>
#include <runos/core/assert.hpp>
#include <runos/core/throw.hpp>

#include <fluid/of13msg.hh>

#include <memory>
#include <utility>

namespace runos {

namespace of {

struct dispatch_error : exception_root, runtime_error_tag
{ };

template<template<class> class Continuator, class Return, class... Args>
Return dispatch_multipart_reply(uint16_t mpart, Args&&... args)
{
    namespace of13 = fluid_msg::of13;
    switch (mpart) {
        case of13::OFPMP_DESC:
            return Continuator<of13::MultipartReplyDesc>()(std::forward<Args>(args)...);
        case of13::OFPMP_FLOW:
            return Continuator<of13::MultipartReplyFlow>()(std::forward<Args>(args)...);
        case of13::OFPMP_AGGREGATE:
            return Continuator<of13::MultipartReplyAggregate>()(std::forward<Args>(args)...);
        case of13::OFPMP_TABLE:
            return Continuator<of13::MultipartReplyTable>()(std::forward<Args>(args)...);
        case of13::OFPMP_TABLE_FEATURES:
            return Continuator<of13::MultipartReplyTableFeatures>()(std::forward<Args>(args)...);
        case of13::OFPMP_PORT_STATS:
            return Continuator<of13::MultipartReplyPortStats>()(std::forward<Args>(args)...);
        case of13::OFPMP_PORT_DESC:
            return Continuator<of13::MultipartReplyPortDescription>()(std::forward<Args>(args)...);
        case of13::OFPMP_QUEUE:
            return Continuator<of13::MultipartReplyQueue>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP:
            return Continuator<of13::MultipartReplyGroup>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP_DESC:
            return Continuator<of13::MultipartReplyGroupDesc>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP_FEATURES:
            return Continuator<of13::MultipartReplyGroupFeatures>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER:
            return Continuator<of13::MultipartReplyMeter>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER_CONFIG:
            return Continuator<of13::MultipartReplyMeterConfig>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER_FEATURES:
            return Continuator<of13::MultipartReplyMeterFeatures>()(std::forward<Args>(args)...);
        default:
            THROW(dispatch_error(), "Unknown multipart type: {}", mpart);
    }
}

template<template<class> class Continuator, class Return, class... Args>
Return dispatch_multipart_request(uint16_t mpart, Args&&... args)
{
    namespace of13 = fluid_msg::of13;
    switch (mpart) {
        case of13::OFPMP_DESC:
            return Continuator<of13::MultipartRequestDesc>()(std::forward<Args>(args)...);
        case of13::OFPMP_FLOW:
            return Continuator<of13::MultipartRequestFlow>()(std::forward<Args>(args)...);
        case of13::OFPMP_AGGREGATE:
            return Continuator<of13::MultipartRequestAggregate>()(std::forward<Args>(args)...);
        case of13::OFPMP_TABLE:
            return Continuator<of13::MultipartRequestTable>()(std::forward<Args>(args)...);
        case of13::OFPMP_TABLE_FEATURES:
            return Continuator<of13::MultipartRequestTableFeatures>()(std::forward<Args>(args)...);
        case of13::OFPMP_PORT_STATS:
            return Continuator<of13::MultipartRequestPortStats>()(std::forward<Args>(args)...);
        case of13::OFPMP_PORT_DESC:
            return Continuator<of13::MultipartRequestPortDescription>()(std::forward<Args>(args)...);
        case of13::OFPMP_QUEUE:
            return Continuator<of13::MultipartRequestQueue>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP:
            return Continuator<of13::MultipartRequestGroup>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP_DESC:
            return Continuator<of13::MultipartRequestGroupDesc>()(std::forward<Args>(args)...);
        case of13::OFPMP_GROUP_FEATURES:
            return Continuator<of13::MultipartRequestGroupFeatures>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER:
            return Continuator<of13::MultipartRequestMeter>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER_CONFIG:
            return Continuator<of13::MultipartRequestMeterConfig>()(std::forward<Args>(args)...);
        case of13::OFPMP_METER_FEATURES:
            return Continuator<of13::MultipartRequestMeterFeatures>()(std::forward<Args>(args)...);
        default:
            THROW(dispatch_error(), "Unknown multipart type: {}", mpart);
    }
}

template<template<class> class Continuator, class Return, class... Args>
Return dispatch_message(uint8_t type, Args&&... args)
{
    namespace of13 = fluid_msg::of13;

    switch (type) {
        // reply
        case of13::OFPT_ERROR:
            return Continuator<of13::Error>()(std::forward<Args>(args)...);
        case of13::OFPT_FEATURES_REPLY:
            return Continuator<of13::FeaturesReply>()(std::forward<Args>(args)...);
        case of13::OFPT_GET_CONFIG_REPLY:
            return Continuator<of13::GetConfigReply>()(std::forward<Args>(args)...);
        case of13::OFPT_EXPERIMENTER:
            return Continuator<of13::Experimenter>()(std::forward<Args>(args)...);
        case of13::OFPT_PACKET_IN:
            return Continuator<of13::PacketIn>()(std::forward<Args>(args)...);
        case of13::OFPT_FLOW_REMOVED:
            return Continuator<of13::FlowRemoved>()(std::forward<Args>(args)...);
        case of13::OFPT_PORT_STATUS:
            return Continuator<of13::PortStatus>()(std::forward<Args>(args)...);
        case of13::OFPT_BARRIER_REPLY:
            return Continuator<of13::BarrierReply>()(std::forward<Args>(args)...);
        case of13::OFPT_QUEUE_GET_CONFIG_REPLY:
            return Continuator<of13::QueueGetConfigReply>()(std::forward<Args>(args)...);
        case of13::OFPT_ROLE_REPLY:
            return Continuator<of13::RoleReply>()(std::forward<Args>(args)...);
        case of13::OFPT_GET_ASYNC_REPLY:
            return Continuator<of13::GetAsyncReply>()(std::forward<Args>(args)...);
        case of13::OFPT_MULTIPART_REPLY:
            return Continuator<of13::MultipartReply>()(std::forward<Args>(args)...);

        // request
        case of13::OFPT_FEATURES_REQUEST:
            return Continuator<of13::FeaturesRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_GET_CONFIG_REQUEST:
            return Continuator<of13::GetConfigRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_FLOW_MOD:
            return Continuator<of13::FlowMod>()(std::forward<Args>(args)...);
        case of13::OFPT_PORT_MOD:
            return Continuator<of13::PortMod>()(std::forward<Args>(args)...);
        case of13::OFPT_METER_MOD:
            return Continuator<of13::MeterMod>()(std::forward<Args>(args)...);
        case of13::OFPT_GROUP_MOD:
            return Continuator<of13::GroupMod>()(std::forward<Args>(args)...);
        case of13::OFPT_PACKET_OUT:
            return Continuator<of13::PacketOut>()(std::forward<Args>(args)...);
        case of13::OFPT_BARRIER_REQUEST:
            return Continuator<of13::BarrierRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_QUEUE_GET_CONFIG_REQUEST:
            return Continuator<of13::QueueGetConfigRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_ROLE_REQUEST:
            return Continuator<of13::RoleRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_GET_ASYNC_REQUEST:
            return Continuator<of13::GetAsyncRequest>()(std::forward<Args>(args)...);
        case of13::OFPT_MULTIPART_REQUEST:
            return Continuator<of13::MultipartRequest>()(std::forward<Args>(args)...);
    default:
        THROW(dispatch_error(), "Unknown message type: {}", type);
    };
}

namespace detail {

template<class Message,
         template<class> class Continuator,
         class Return,
         class... Args>
struct UpcastContinuator
{
    Return operator()(fluid_msg::OFMsg& msg, Args&&... args) const
    {
        ASSERT(msg.type() == Message().fluid_msg::OFMsg::type());
        return Continuator<Message>()(static_cast<Message&>(msg),
                                      std::forward<Args>(args)...);
    }
};

template<template<class> class Continuator,
         class Return,
         class... Args>
struct UpcastContinuator<fluid_msg::of13::MultipartReply, Continuator, Return, Args...>
{
    template<class Message>
    using Continue = UpcastContinuator<Message, Continuator, Return, Args...>;

    Return operator()(fluid_msg::OFMsg& ofmsg, Args&&... args) const
    {
        ASSERT(ofmsg.type() == fluid_msg::of13::OFPT_MULTIPART_REPLY);
        auto& msg = static_cast<fluid_msg::of13::MultipartReply&>(ofmsg);
        return dispatch_multipart_reply<
                    Continue, Return
               >(msg.mpart_type(), msg, std::forward<Args>(args)...);
    }
};

template<template<class> class Continuator,
         class Return,
         class... Args>
struct UpcastContinuator<fluid_msg::of13::MultipartRequest, Continuator, Return, Args...>
{
    template<class Message>
    using Continue = UpcastContinuator<Message, Continuator, Return, Args...>;

    Return operator()(fluid_msg::OFMsg& ofmsg, Args&&... args) const
    {
        ASSERT(ofmsg.type() == fluid_msg::of13::OFPT_MULTIPART_REQUEST);
        auto& msg = static_cast<fluid_msg::of13::MultipartRequest&>(ofmsg);
        return dispatch_multipart_request<
                    Continue, Return
               >(msg.mpart_type(), msg, std::forward<Args>(args)...);
    }
};

template<template<class> class Continuator, class Return, class... Args>
struct Upcast {
    template<class Message>
    using Continue = UpcastContinuator<Message, Continuator, Return, Args...>;

    Return operator()(fluid_msg::OFMsg& msg, Args&&... args)
    {
        return dispatch_message<Continue, Return>(msg.type(), msg, std::forward<Args>(args)...);
    }
};

} // detail

template<template<class> class Continuator, class Return, class... Args>
Return upcast(fluid_msg::OFMsg& msg, Args&&... args)
{
    return detail::Upcast<Continuator, Return, Args...>()(msg, std::forward<Args>(args)...);
}

} // namespace of

template<class Dispatcher>
struct MakeDispatchableWrapper
{

    template<class Message>
    struct Continuator
    {
        using wrapper_type 
            = typename Dispatcher::template DispatchableWrapper<Message>;
        std::unique_ptr<wrapper_type> operator()(Message& msg) const
        {
            return std::make_unique<wrapper_type>(msg);
        }
    };

};

template<class Dispatcher,
         class Return = std::unique_ptr<typename Dispatcher::Dispatchable>>
Return make_dispatchable(fluid_msg::OFMsg& msg)
{
    return of::upcast<
                MakeDispatchableWrapper<Dispatcher>::template Continuator,
                Return
           >(msg);
}

template<class Dispatcher>
struct MakeDispatchableMessage
{

    template<class Message>
    struct Continuator
    {
        using Msg = typename Dispatcher::template DispatchableMessage<Message>;
        std::unique_ptr<Msg> operator()() const
        {
            return std::make_unique<Msg>();
        }
    };

};
template<class Dispatcher,
         class Return = std::unique_ptr<typename Dispatcher::Dispatchable>>
Return make_dispatchable(uint8_t type, uint16_t mpart)
{
    switch (type) {
        case fluid_msg::of13::OFPT_MULTIPART_REPLY:
            return of::dispatch_multipart_reply<
                        MakeDispatchableMessage<Dispatcher>::template Continuator,
                        Return
                   >(mpart);
        case fluid_msg::of13::OFPT_MULTIPART_REQUEST:
            return of::dispatch_multipart_request<
                        MakeDispatchableMessage<Dispatcher>::template Continuator,
                        Return
                   >(mpart);
        default:
            return of::dispatch_message<
                        MakeDispatchableMessage<Dispatcher>::template Continuator,
                        Return
                   >(type);
    }
}

} // namespace runos
