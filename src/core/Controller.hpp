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

#include <memory>
#include <type_traits>

#include <fluid/of13msg.hh>

#include "runos/core/logging.hpp"
#include "runos/core/demangle.hpp"

#include <runos/core/future-decl.hpp>
#include "api/FunctionalTraits.hpp"
#include "api/DoubleDispatcher.hpp"
#include "api/OFConnection.hpp"

#include "Application.hpp"
#include "Loader.hpp"

namespace runos {

using LinearDispatch = DoubleDispatcher<
    struct linear_dispatch_tag,
    fluid_msg::OFMsg,
    bool,
    OFConnectionPtr
>;

template<class Message>
using OFMessageHandler = LinearDispatch::Handler<Message>;
using OFMessageHandlerPtr = std::shared_ptr<LinearDispatch::HandlerBase>;
using OFMessageHandlerWeakPtr = std::weak_ptr<LinearDispatch::HandlerBase>;

class Controller : public Application
{
    Q_OBJECT
    SIMPLE_APPLICATION(Controller, "controller")
public:
    Controller();
    ~Controller();

    void init(Loader* loader, const Config& config) override;

    /**
     * Register your application as a handler for a specific OF message type
     */
    void register_handler(OFMessageHandlerPtr handler, int priority = 0);

    template<class Callable,
             class = typename std::enable_if<
                        not std::is_convertible<Callable, OFMessageHandlerPtr>
                               ::value
                     >::type>
    OFMessageHandlerPtr register_handler(Callable&& callable, int priority= 0)
    {
        using Arg0 = typename traits::function_traits<Callable>
                                    ::template argument<1>;
        using Arg0_T = typename std::decay<Arg0>::type;

        struct HandlerImpl final : OFMessageHandler< Arg0_T >, Callable
        {
            explicit HandlerImpl(Callable&& callable)
                : Callable(callable)
            { }

            bool process(Arg0_T& msg, OFConnectionPtr conn)
            {
                return this->operator()(msg, conn);
            }
        };

        auto ret = std::make_shared<HandlerImpl>(std::move(callable));
        register_handler(ret, priority);
        return ret;
    }
    
    bool dispatch(fluid_msg::OFMsg& msg, OFConnectionPtr conn);

protected slots:
    void onSwitchDiscovered(OFConnectionPtr conn);

private:
    struct implementation;
    std::unique_ptr<implementation> impl;
};

} // namespace runos
