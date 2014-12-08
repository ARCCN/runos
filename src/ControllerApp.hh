#pragma once

#include <time.h>

#include "Controller.hh"
#include "Common.hh"
#include "Application.hh"
#include "Switch.hh"
#include "HostManager.hh"
#include "Topology.hh"
#include "Rest.hh"
#include "JsonParser.hh"
#include "RestListener.hh"

/**
 * REST-class provides REST-interface support for controller-manager
 */
class ControllerRest : public Rest {
    friend class ControllerApp;
    class ControllerApp* ctrl;
public:
    ControllerRest(std::string name, std::string page): Rest(name, page, Rest::Application) {}
    std::string handle(std::vector<std::string> params) override;
};

class ControllerApp : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(ControllerApp, "controller-manager")
    friend class ControllerRest;
public:
    ControllerApp();
    ~ControllerApp();

    void init(Loader* loader, const Config& config) override;
private slots:
    void getInfo(std::string, int, int, bool);
private:
    ControllerRest* r;
    Controller* ctrl;
    struct ControllerInfo* info;
};
