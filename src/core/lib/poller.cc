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

#include "poller.hpp"
#include "runos/core/logging.hpp"
#include <thread>

namespace runos {

Poller::Poller(Application* parent, uint16_t poll_interval) :
    Poller(parent, nullptr, poll_interval, PollerType::Application)
{ }

Poller::Poller(Polling* polling_parent, uint16_t poll_interval) :
    Poller(nullptr, polling_parent, poll_interval, PollerType::Polling)
{ }

Poller::Poller(Application* parent, Polling* polling_parent,
               uint16_t poll_interval, PollerType type) :
    wtimer(new QTimer),
    wthread(new QThread),
    executor(new qt_executor(wtimer)),
    parent(parent),
    polling_parent(polling_parent),
    type(type)
{
    wtimer->setInterval(poll_interval);
    wtimer->moveToThread(wthread);
}


Poller::~Poller()
{
    delete wtimer;
    delete wthread;
    delete executor;
}

void Poller::run()
{
    if (not wthread->isRunning()) {
        QObject::connect(wthread, &QThread::started, wtimer, qOverload<>(&QTimer::start));
        QObject::connect(wthread, &QThread::finished, wtimer, &QTimer::stop);
        switch(type) {
        case PollerType::Application:
            QObject::connect(wtimer, SIGNAL(timeout()),
                             parent, SLOT(polling()), Qt::DirectConnection);
            break;
        case PollerType::Polling:
            QObject::connect(wtimer, &QTimer::timeout,
                             polling_parent, &Polling::polling, Qt::DirectConnection);
            break;
        }
        wthread->start();
    } else {
        if (not wtimer->isActive()) {
            async(*executor, [wtimer = wtimer]() {
                wtimer->start();
            });
        }
    }
}

void Poller::stop()
{
    wthread->quit();
}

void Poller::pause()
{
    async(*executor, [wtimer = wtimer]() {
        wtimer->stop();
    });
}

void Poller::apply(const std::function<void()>& f)
{
    async(*executor, f);
}

} // namespace runos
