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

#pragma once

#include <utility>
#include <tuple>
#include <optional>

namespace runos {

template<class Tag, class BaseMessage, class Return, class... Args>
struct DoubleDispatcher {
    using tag_type = Tag;
    using base_message_type = BaseMessage;
    using return_type = Return;
    using arguments_type = std::tuple<Args...>;
    using result_type = std::optional<Return>;

    template<class Message>
    struct Handler;

    struct HandlerBase
    {
        template<class Message, class... Args_>
        result_type dispatch(Message& msg, Args_&&... args)
        {
            static_assert( std::is_base_of<BaseMessage, Message>::value, "" );

            if (auto self = dynamic_cast< Handler<Message>* >(this)) {
                return self->process(msg, std::forward<Args>(args)...);
            } else if (auto self = dynamic_cast< Handler<BaseMessage>* >(this)) {
                return self->process(msg, std::forward<Args>(args)...);
            }

            return std::nullopt;
        }

        virtual ~HandlerBase() = default;
    };

    template<class Message>
    struct Handler
        : public virtual HandlerBase
    {
        static_assert( std::is_base_of< BaseMessage, Message >::value,
                       "Handler message type must derive from domain base type");

        virtual return_type process(Message&, Args...) = 0;
    };

    struct Dispatchable
    {
        virtual result_type dispatch(HandlerBase& hbase, Args... args) = 0;
        virtual ~Dispatchable() = default;
    };

    template<class Message>
    struct DispatchableMessage : virtual Message, Dispatchable
    {
        //using Message::Messsage;

        result_type dispatch(HandlerBase& hbase, Args... args) override
        {
            return hbase.dispatch(*this, args...);
        }
    };

    template<class Message>
    class DispatchableWrapper : public Dispatchable
    {
    public:
        explicit DispatchableWrapper(Message& msg)
            : msg(msg)
        { }

        result_type dispatch(HandlerBase& hbase, Args... args) override
        {
            return hbase.dispatch(msg, args...);
        }
    private:
        Message& msg;
    };

};

// FIXME: DON'T COPY-PASTE HERE
template<class Tag, class BaseMessage, class... Args>
struct DoubleDispatcher<Tag, BaseMessage, void, Args...> {
    using tag_type = Tag;
    using base_message_type = BaseMessage;
    using return_type = void;
    using arguments_type = std::tuple<Args...>;
    using result_type = bool;

    template<class Message>
    struct Handler;

    struct HandlerBase
    {
        template<class Message, class... Args_>
        result_type dispatch(Message& msg, Args_&&... args)
        {
            static_assert( std::is_base_of<BaseMessage, Message>::value, "" );

            if (auto self = dynamic_cast< Handler<Message>* >(this)) {
                self->process(msg, std::forward<Args>(args)...);
                return true;
            } else if (auto self = dynamic_cast< Handler<BaseMessage>* >(this)) {
                self->process(msg, std::forward<Args>(args)...);
                return true;
            }

            return false;
        }

        virtual ~HandlerBase() = default;
    };

    template<class Message>
    struct Handler
        : public virtual HandlerBase
    {
        static_assert( std::is_base_of< BaseMessage, Message >::value,
                       "Handler message type must derive from domain base type");

        virtual return_type process(Message&, Args...) = 0;
    };

    struct Dispatchable
    {
        virtual result_type dispatch(HandlerBase& hbase, Args... args) = 0;
        virtual ~Dispatchable() = default;
    };

    template<class Message>
    struct DispatchableMessage : virtual Message, Dispatchable
    {
        //using Message::Messsage;

        result_type dispatch(HandlerBase& hbase, Args... args) override
        {
            return hbase.dispatch(static_cast<Message&>(*this), args...);
        }
    };

    template<class Message>
    class DispatchableWrapper : public Dispatchable
    {
    public:
        explicit DispatchableWrapper(Message& msg)
            : msg(msg)
        { }

        result_type dispatch(HandlerBase& hbase, Args... args) override
        {
            return hbase.dispatch(msg, args...);
        }
    private:
        Message& msg;
    };

};

} // namespace runos
