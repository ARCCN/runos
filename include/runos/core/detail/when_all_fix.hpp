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

#include <runos/core/future-decl.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/executors/inline_executor.hpp>

#include <memory>
#include <atomic>
#include <vector>

namespace runos {

template<class InputIterator>
struct when_all_impl
{
    using element_type
        = future< typename std::iterator_traits<InputIterator>::value_type::value_type >;
    using container_type
        = std::vector< element_type >;
    using return_type
        = future< container_type >;

    struct context
    {
        container_type ret;
        boost::inline_executor executor;

        context(size_t N)
            : ret(N)
        {
            std::atomic_init(&pending, N);
        }

        return_type get_future()
        {
            return p.get_future();
        }

        void bump()
        {
            if ((--pending) == 0) {
                p.set_value(std::move(ret));
            }
        }

    private:
        std::atomic<size_t> pending;
        promise<container_type> p;
    };

    return_type operator()(InputIterator begin, InputIterator end)
    {
        auto ctx = std::make_shared<context>( std::distance(begin, end) );

        for (size_t i = 0; begin != end; ++i, ++begin) {
            begin->then(ctx->executor, [ctx, i](element_type e) mutable {
                ctx->ret[i] = std::move(e);
                ctx->bump();
            });
        }

        return ctx->get_future();
    }
};

template<class InputIterator>
auto when_all_fix(InputIterator begin, InputIterator end)
    -> typename when_all_impl<InputIterator>::return_type
{
    return when_all_impl<InputIterator>{}(begin, end);
}

} // namespace runos
