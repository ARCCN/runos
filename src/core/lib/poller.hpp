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

#include "../Application.hpp"
#include "qt_executor.hpp"

#include <QTimer>
#include <QThread>

namespace runos {

enum class PollerType {
    Application, Polling
};

class Polling : public QObject {
public:
    virtual void polling() = 0;
};

class Poller {
public:
    Poller(Application* parent, uint16_t poll_interval);
    Poller(Polling* parent, uint16_t poll_interval);
    ~Poller();

    void run();
    void stop();
    void pause();
    void apply(const std::function<void()>& f);

private:
    Poller(Application* parent, Polling* polling_parent,
           uint16_t poll_interval, PollerType type);

private:
    QTimer* wtimer;
    QThread* wthread;
    qt_executor* executor;
    Application* parent;
    Polling* polling_parent;
    PollerType type;
};

} // namespace runos
