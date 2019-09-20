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

#include <runos/core/logging.hpp>
#include <runos/core/future.hpp>

#include <QtCore>

#include <boost/thread/concurrent_queues/queue_op_status.hpp>

#include <utility>
#include <mutex>

namespace runos {

struct qt_executor
{
    qt_executor(QObject* target)
        : target(target)
    {
    }

    template<class Closure>
    void submit(Closure&& closure)
    {
        //TODO: Commented due to possible race condition
        //      If this thread wants to submit task again inside.
        /*std::lock_guard< std::mutex > lk(m);
        if (closed(lk)) {
            throw boost::sync_queue_is_closed();
        }*/

        QObject signalSource;
        QObject::connect(
            &signalSource,
            &QObject::destroyed,
            target,
            [f = std::forward<Closure>(closure)]() mutable { f(); }
        );
    }

    void close()
    {
        target = nullptr;
    }

    bool closed(std::lock_guard< std::mutex > &)
    {
        return target == nullptr;
    }

    bool closed()
    {
        std::lock_guard<std::mutex> lk(m);
        return closed(lk);
    }

    bool try_executing_one()
    {
        return false;
    }

    template<class Pred>
    bool reschedule_until(Pred const& )
    {
        return false;
    }

private:
    std::mutex m;
    QObject* target;
};

} // namespace runos
