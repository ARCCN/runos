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

#include "SwitchOrdering.hpp"

#include "SwitchManager.hpp"
#include "api/OFAgent.hpp"

#include <runos/core/logging.hpp>
#include <runos/core/catch_all.hpp>

#include <algorithm>

namespace runos {

using std::chrono::duration_cast;
using std::chrono::milliseconds;

REGISTER_APPLICATION(SwitchOrderingManager, {"switch-manager", ""}) 

// This class in used for correct handling of simultaneous
// connects and disconnects for some switch
class SwitchLocker {
public:
    SwitchLocker(SwitchOrderingManager* app):
        app_(app) {}

    enum class SwitchAction { UP, DOWN };

    bool canHandleAction(SwitchAction act, SwitchPtr sw,
                         class MainHandler* emitter);
    
private:
    void add_and_lock_switch_mutex(uint64_t dpid);
    void unlock_switch_mutex(uint64_t dpid);

    SwitchOrderingManager* app_;
    std::unordered_map<uint64_t, std::unique_ptr<std::mutex>> sw_mutexes_;
    std::mutex mut_for_map_;
};

// This class is used for handling switch events from other applications.
// When some event occurs, SwitchOrderingManager creates MainHandler 
// and QThread for handling.
// MainHandler is looking for applications for this events and
// call overrided methods in created thread.
// When handling was finished, MainHandler and QThread is destroyed.
class MainHandler : public QObject {
    Q_OBJECT
public:
    MainHandler() = delete;
    MainHandler(SwitchLocker* locker, SwitchPtr sw, EventType type):
        locker_(locker), type(type), sw(sw) {}

    SwitchLocker* locker_;
    HandlersMap subscribed;
    EventType type;
    SwitchPtr sw;
signals:
    void finished(uint64_t dpid);
public slots:
    void onSwitchUp();
    void onSwitchDown();
};

void SwitchOrderingManager::init(Loader* loader, const Config& rootConfig)
{
    // listening all switch and link events
    auto sw_mgr = SwitchManager::get(loader);
    connect(sw_mgr, &SwitchManager::switchUp,
            this, &SwitchOrderingManager::onSwitchUp);
    connect(sw_mgr, &SwitchManager::switchDown,
            this, &SwitchOrderingManager::onSwitchDown);
    connect(sw_mgr, &SwitchManager::linkUp,
            this, &SwitchOrderingManager::onLinkUp);
    connect(sw_mgr, &SwitchManager::linkDown,
            this, &SwitchOrderingManager::onLinkDown);

    locker_ = std::make_unique<SwitchLocker>(this);

    // info barrier_timers
    const Config& config = config_cd(rootConfig, "of-server");
    echo_interval_ = std::chrono::seconds(config_get(config, "echo-interval", 5));
}

void SwitchOrderingManager::registerHandler(SwitchEventHandler* handler,
                                            int prio, uint8_t types)
{
    handlers_.emplace(prio, handler);
    handler->types = types;
}

void SwitchOrderingManager::run_handling(SwitchPtr sw, EventType type)
{
    auto handler = new MainHandler(locker_.get(), sw, type);
    auto thread = new QThread;

    HandlersMap subscribed;
    for (auto& it: handlers_) {
        auto& act = it.second;
        if (act->types & type)
            subscribed.emplace(it);
    }
    handler->subscribed = subscribed;

    handler->moveToThread(thread);
    switch (type) {
    case AT_SWITCH_UP:
        connect(thread, &QThread::started, handler, &MainHandler::onSwitchUp);
        // when switchUp handling finished
        connect(handler, &MainHandler::finished, [this, sw, thread](){
            std::lock_guard<std::mutex> lock(mut_);
            connected_.push_back(sw->dpid());

            // if we have cached links which we must handle
            if (cached_.count(sw->dpid()) > 0) {
                auto up_links = std::move(cached_.at(sw->dpid()));
                std::for_each(up_links.begin(), up_links.end(),
                    [this](auto port) {
                        for (auto& it: this->handlers_) {
                            auto& handler = it.second;
                            if (handler->types & AT_LINK_UP) {
                                catch_all_and_log([&](){
                                    handler->linkUp(port);
                                });
                            }
                        }
                        LOG(WARNING) << "link up - " << port->switch_()->dpid() 
                                                     << ":" << port->number();
                    });
                CHECK(cached_.at(sw->dpid()).size() == 0);
            }

            // finish working thread
            thread->quit();
        });
        break;
    case AT_SWITCH_DOWN:
        connect(thread, &QThread::started, handler, &MainHandler::onSwitchDown);
        connect(handler, &MainHandler::finished, [this, sw, thread](){
            std::lock_guard<std::mutex> lock(mut_);
            connected_.erase(std::remove(connected_.begin(), connected_.end(), 
                                                 sw->dpid()), connected_.end());
            // finish working thread
            thread->quit();
        });
        break;
    default:
        LOG(ERROR) << "Unexpected event type!";
    }

    // destructor events
    connect(thread, &QThread::finished, handler, &MainHandler::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // start handling
    thread->start();
}

bool SwitchOrderingManager::isConnected(uint64_t dpid) const
{
    std::lock_guard<std::mutex> lock(mut_);

    auto it = std::find(connected_.begin(), connected_.end(), dpid);
    return it != connected_.end();
}

void SwitchOrderingManager::onSwitchUp(SwitchPtr sw)
{
    run_handling(sw, AT_SWITCH_UP);

    // start barrier timer to send BarrierRequest
    auto barrier_timer = new QTimer(this);
    auto interval = duration_cast<milliseconds>(echo_interval_);
    barrier_timer->setInterval(interval.count());
    QObject::connect(barrier_timer, &QTimer::timeout, [sw] (){
        try {
            sw->connection()->agent()->barrier();
        } catch (const OFAgent::request_error& e) {
            LOG(ERROR) << "[SwitchOrderingManager] - " << e.what();
        }
    });
    barrier_timer->start();

    barrier_timers_.insert(std::make_pair(sw->dpid(), barrier_timer));
}

void SwitchOrderingManager::onSwitchDown(SwitchPtr sw)
{
    run_handling(sw, AT_SWITCH_DOWN);

    // stop and remove barrier timer
    auto barrier_timer_it = barrier_timers_.find(sw->dpid());
    CHECK(barrier_timers_.end() != barrier_timer_it);
    barrier_timer_it->second->disconnect();
    barrier_timer_it->second->deleteLater();
    barrier_timers_.erase(barrier_timer_it);
}

void SwitchOrderingManager::onLinkUp(PortPtr port)
{
    auto dpid = port->switch_()->dpid();

    if (isConnected(dpid)) {
        for (auto& it: handlers_) {
            auto& handler = it.second;
            if (handler->types & AT_LINK_UP) {
                catch_all_and_log([&](){
                    handler->linkUp(port);
                });
            }
        }
        LOG(WARNING) << "link up - " << dpid << ":" << port->number();
    } else {
        std::lock_guard<std::mutex> lock(mut_);
        cached_[dpid].push_back(port);
    }
}

void SwitchOrderingManager::onLinkDown(PortPtr port)
{
    auto dpid = port->switch_()->dpid();

    { // lock

    std::lock_guard<std::mutex> lock(mut_);

    // Not empty cached[dpid] means that we have not handled switchUp yet,
    // but we have already gotten linkDown -- must remove it from cached
    if (cached_.count(dpid) && not cached_[dpid].empty()) {
        auto& ports = cached_[dpid];
        ports.erase(std::remove(ports.begin(), ports.end(), port), ports.end());
        return;
    }

    } // unlock

    for (auto& it: handlers_) {
        auto& handler = it.second;
        if (handler->types & AT_LINK_DOWN) {
            catch_all_and_log([&](){
                handler->linkDown(port);
            });
        }
    }
    LOG(WARNING) << "link down - " << dpid << ":" << port->number();
}

// Handler's methods
void MainHandler::onSwitchUp()
{
    if (locker_->canHandleAction(SwitchLocker::SwitchAction::UP, sw, this)) {
        for (auto& it : subscribed) {
            auto& handler = it.second;
            catch_all_and_log([&]() {
                handler->switchUp(sw);
            });
        }

        LOG(WARNING) << "switch up, dpid: " << sw->dpid();
        emit finished(sw->dpid());
    }
}

void MainHandler::onSwitchDown()
{
    if (locker_->canHandleAction(SwitchLocker::SwitchAction::DOWN, sw, this)) {
        for (auto& it : subscribed) {
            auto& handler = it.second;
            catch_all_and_log([&]() {
                handler->switchDown(sw);
            });
        }

        LOG(WARNING) << "switch down, dpid: " << sw->dpid();
        emit finished(sw->dpid());
    }
}

// SwitchLocker's methods
void SwitchLocker::add_and_lock_switch_mutex(uint64_t dpid)
{
    try {
        mut_for_map_.lock();
        if (not sw_mutexes_.count(dpid)) {
            sw_mutexes_.emplace(dpid, std::make_unique<std::mutex>());
        }

        auto& sw_mut = sw_mutexes_[dpid];
        mut_for_map_.unlock();

        sw_mut->lock();
    } catch (const std::bad_alloc& ex) { // incorrect std::make_unique
        LOG(ERROR) << "Allocation failed: " << ex.what();
        mut_for_map_.unlock();
    }
}

void SwitchLocker::unlock_switch_mutex(uint64_t dpid)
{
    std::lock_guard<std::mutex> lock(mut_for_map_);
    sw_mutexes_[dpid]->unlock();
}

bool SwitchLocker::canHandleAction(SwitchAction act,
                                   SwitchPtr sw,
                                   MainHandler* emitter)
{
    add_and_lock_switch_mutex(sw->dpid());

    auto switch_up_handled = app_->isConnected(sw->dpid());
    auto alive = sw->connection()->alive();

    bool ret;
    switch (act) {
    case SwitchAction::UP:
        ret = alive and not switch_up_handled;
        break;
    case SwitchAction::DOWN:
        ret = switch_up_handled and not alive;
        break;
    default:
        CHECK(false);
    }

    if (not ret) {
        unlock_switch_mutex(sw->dpid());
    } else {
        QObject::connect(emitter, &MainHandler::finished,
            [this](uint64_t dpid){
                unlock_switch_mutex(dpid);
            });
    }

    return ret;
}

} // namespace runos

#include "SwitchOrdering.moc"
