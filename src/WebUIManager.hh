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

#include <vector>
#include <string>

#include "Common.hh"
#include "Application.hh"
#include "Rest.hh"
#include "Controller.hh"
#include "AppObject.hh"
#include "json11.hpp"

struct Position {
    int x;
    int y;
};

class WebObject: public AppObject {
    struct WebObjectImpl* m;
public:
    WebObject(uint64_t _id, bool is_router);
    ~WebObject();

    uint64_t id() const override;
    json11::Json to_json() const override;
    Position pos() const;
    std::string icon() const;
    std::string display_name() const;

    void pos(int x, int y);
    void display_name(std::string name);
};

class WebUIManager: public Application, private RestHandler {
    Q_OBJECT
    SIMPLE_APPLICATION(WebUIManager, "webui")
    struct WebUIManagerImpl* m;
    class SwitchManager* m_switch_manager;
    Controller* ctrl;
public:
    bool eventable() override {return false; }
    AppType type() override { return AppType::None; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;
    json11::Json handlePUT(std::vector<std::string> params, std::string body) override;


    void init(Loader* loader, const Config& config) override;
    std::vector<WebObject*> getObjects();
    WebObject* getObject(uint64_t _id);
    WebUIManager();
    ~WebUIManager();
private slots:
    void newSwitch(class Switch* dp);
    void newHost(class Host* dev);
};
