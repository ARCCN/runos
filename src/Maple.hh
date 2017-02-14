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
#include "Application.hh"
#include "Loader.hh"
#include "Controller.hh"
#include "Common.hh"

namespace runos {

class Maple : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(Maple, "maple")
public:

    /**
    * Registers new message handler for each worker thread.
    * Used for performance-critical message processing, such as packet-in's.
    */
    void registerHandler(const char* name, PacketMissHandlerFactory factory);

    /**
     *  Get number of Maple's table
     */
    uint8_t handler_table() const;

    /**
      * Deleting all TraceTrees
      */
    void invalidateTraceTree();

    ~Maple();
    void init(Loader *loader, const Config& config) override;
    void startUp(Loader *loader) override;
    void process(const of13::PacketIn &pi, SwitchConnectionPtr conn);
public slots:
    void onSwitchUp(SwitchConnectionPtr conn, of13::FeaturesReply fr);
private:
    std::unique_ptr<class MapleImpl> impl;
};

} // namespace runos
