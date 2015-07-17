#pragma once

#include "Common.hh"
#include "Application.hh"
#include "Controller.hh"
#include "Switch.hh"
#include "HostManager.hh"
#include "AppObject.hh"
#include "RestListener.hh"
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
    json11::Json to_json() const;
    Position pos() const;
    std::string icon() const;
    std::string display_name() const;

    void pos(int x, int y);
    void display_name(std::string name);
};

class WebUIManagerRest: public Rest {
    class WebUIManager* m;
    friend class WebUIManager;
public:
    std::string handle(std::vector<std::string> params) override;
    WebUIManagerRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
};

class WebUIManager: public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(WebUIManager, "webui")
    struct WebUIManagerImpl* m;
    SwitchManager* m_switch_manager;
    WebUIManagerRest* r;
    Controller* ctrl;
public:
    void init(Loader* loader, const Config& config) override;
    std::vector<WebObject*> getObjects();
    WebObject* id(uint64_t _id);
    WebUIManager();
    ~WebUIManager();
private slots:
    void newSwitch(Switch* dp);
    void newHost(Host* dev);
};
