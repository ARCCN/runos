#include "WebUIManager.hh"

REGISTER_APPLICATION(WebUIManager, {"switch-manager", "host-manager", "rest-listener", ""})

struct WebObjectImpl {
    uint64_t id;
    Position pos;
    std::string icon;
    std::string display_name;

    std::vector<class WebObject*> neighbors;
    int lvl;
};

struct WebUIManagerImpl {
    std::vector<WebObject*> objects;
};

WebObject::WebObject(uint64_t _id, bool is_router)
{
    m = new WebObjectImpl;
    m->id = _id;
    m->pos.x = -1;
    m->pos.y = -1;
    if (is_router)
        m->icon = "router";
    else
        m->icon = "aimac";
    m->display_name = AppObject::uint64_to_string(_id);
}

WebObject::~WebObject() {
    delete m;
}

uint64_t WebObject::id() const
{
    return m->id;
}

json11::Json WebObject::to_json() const
{
    return json11::Json::object {
        {"ID", id_str()},
        {"x_coord", pos().x},
        {"y_coord", pos().y},
        {"icon", icon()},
        {"display_name", display_name()}
    };
}

Position WebObject::pos() const
{ return m->pos; }

std::string WebObject::icon() const
{ return m->icon; }

std::string WebObject::display_name() const
{ return m->display_name; }

void WebObject::pos(int x, int y)
{
    m->pos.x = x;
    m->pos.y = y;
}

void WebObject::display_name(std::string name)
{ m->display_name = name; }

WebUIManager::WebUIManager()
{
    m = new WebUIManagerImpl;
    r = new WebUIManagerRest("Web UI Manager", "none");
    r->m = this;
}

WebUIManager::~WebUIManager()
{
    delete m;
    delete r;
}

std::vector<WebObject*> WebUIManager::getObjects()
{ return m->objects; }

WebObject* WebUIManager::id(uint64_t _id)
{
    for (auto it : m->objects) {
        if (it->id() == _id)
            return it;
    }
    return NULL;
}

void WebUIManager::init(Loader *loader, const Config &config)
{
    m_switch_manager = SwitchManager::get(loader);
    HostManager* host_manager = HostManager::get(loader);

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
                     this, &WebUIManager::newSwitch);

    QObject::connect(host_manager, &HostManager::hostDiscovered,
                     this, &WebUIManager::newHost);

    RestListener::get(loader)->newListener("webui", r);
}

void WebUIManager::newSwitch(Switch *dp)
{
    WebObject* obj = new WebObject(dp->id(), true);
    m->objects.push_back(obj);
}

void WebUIManager::newHost(Host* dev)
{
    WebObject* obj = new WebObject(dev->id(), false);
    obj->display_name(dev->mac());
    m->objects.push_back(obj);
}

std::string WebUIManagerRest::handle(std::vector<std::string> params)
{
    if (params[0] == "GET" && params[2] == "webinfo") {
        if (params[3] == "all") {
            if (m->getObjects().size())
                return json11::Json(m->getObjects()).dump();
            else
                return "{ \"error\": \"empty\" }";
        }
        else {
            uint64_t id = std::stoull(params[3]);
            WebObject* obj = m->id(id);
            if (obj) {
                return json11::Json(obj).dump();
            }
            else {
                return "{ \"error\": \"Object not found\" }";
            }
        }
    }

    if (params[0] == "PUT" && params[2] == "coord") {
        if (params[3] == "all") {

        }
        else {
            uint64_t id = std::stoull(params[3]);
            int x = atoi(params[5].c_str());
            int y = atoi(params[4].c_str());
            WebObject* obj = m->id(id);
            obj->pos(x, y);
            return "{ \"webui\": \"OK\" }";
        }
    }

    if (params[0] == "PUT" && params[2] == "name") {
        uint64_t id = std::stoull(params[3]);
        std::string new_name = params[4].c_str();
        WebObject* obj = m->id(id);
        obj->display_name(new_name);
        return "{ \"webui\": \"OK\" }";
    }

    return "{ \"error\": \"error\" }";
}
