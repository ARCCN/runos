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

#include "Topology.hpp"

#include "RestListener.hpp"
#include <json.hpp>
#include <runos/core/logging.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>

namespace runos {

using json = nlohmann::json;

template <typename T>
std::vector<T> toVector(rest::ptree const& pt, rest::ptree::key_type const& key)
{
    std::vector<T> ret;
    for (auto& item : pt.get_child(key))
        ret.push_back(item.second.get_value<T>());
    return ret;
}

struct TopologyDump : rest::resource {
    Topology* app;

    explicit TopologyDump(Topology* app)
        : app(app) {}

    rest::ptree Get() const override {
        rest::ptree ret;

        auto dump = app->dumpWeights();
        rest::ptree lpt_arr;
        for (auto link_iter : dump) {
            rest::ptree lpt;
            lpt.put("source.dpid", link_iter.source.dpid);
            lpt.put("source.port", link_iter.source.port);
            lpt.put("target.dpid", link_iter.target.dpid);
            lpt.put("target.port", link_iter.target.port);
            lpt.put("metrics.hop", link_iter.hop_metrics);
            lpt.put("metrics.port-speed", link_iter.ps_metrics);
            lpt.put("metrics.port-loading", link_iter.pl_metrics);
            lpt_arr.push_back(std::make_pair("", lpt));
        }
        ret.add_child("array", lpt_arr);
        ret.put("_size", lpt_arr.size());

        return ret;
    }
};

struct RouteCollection : rest::resource {
    Topology* app;
    uint32_t id;
    std::string service;

    explicit RouteCollection(Topology* app, uint32_t id)
        : app(app), id(id), service("none")
    { }

    explicit RouteCollection(Topology* app, std::string service)
        : app(app), id(0), service(service)
    { }

    rest::ptree Get() const override {
        rest::ptree ret;

        if (id > 0) {
            auto route_str = std::move(app->getRouteDump(id));
            THROW_IF(route_str == "", rest::http_error(400), "Route not found");

            std::istringstream is(route_str);
            read_json(is, ret);
        } else if (service != "none") {
            std::vector<uint32_t> ids;
            if (service == "all")
                ids = std::move(app->getRoutes());
            else {
                auto sf = ServiceFlag::_from_string(service.c_str());
                ids = std::move(app->getRoutes(sf));
            }

            rest::ptree arr;
            for (auto id : ids) {
                rest::ptree pt;
                std::istringstream is(app->getRouteDump(id));
                read_json(is, pt);
                arr.push_back(std::make_pair("", pt));
            }
            ret.add_child("array", arr);
            ret.put("_size", arr.size());
        } else {
            THROW(rest::http_error(400), "Invalid arguments");
        }

        return ret;
    }
};

enum class RouteMod {
    AddPath,
    DeletePath,
    MovePath,
    ModifyPath,
    AddDynamic,
    DeleteDynamic,
    PredictPath
};

struct RouteResourceMods : rest::resource {
    Topology* app;
    uint32_t id;
    uint8_t path_id;
    RouteMod mod;

    explicit RouteResourceMods(Topology* app, uint32_t id, RouteMod mod)
        : app(app), id(id), mod(mod)
    { }

    explicit RouteResourceMods(Topology* app, uint32_t id, uint8_t path_id, RouteMod mod)
        : app(app), id(id), path_id(path_id), mod(mod)
    { }

    rest::ptree Get() const override {
        rest::ptree ret;

        if (mod == RouteMod::PredictPath) {
            auto path = std::move(app->predictPath(id));

            if (path.size() == 0) {
                ret.put("error", "Cannot predict path or dynamic is disabled");
            } else {
                json jpath;
                std::for_each(path.begin(), path.end(), [&jpath](const auto& sp) {
                    jpath["m_path"].push_back({sp.dpid, sp.port});
                });

                std::istringstream is(jpath.dump());
                read_json(is, ret);
             }
        }

        return ret;
    }

    rest::ptree Post(rest::ptree const& pt) override {
        using namespace route_selector;

        auto get_dpids = [&pt](std::string type)
        {
            std::vector<uint64_t> ret;
            rest::ptree::const_assoc_iterator it;
            it = pt.find(type);
            if (it != pt.not_found())
                ret = std::move(toVector<uint64_t>(pt, type));
            return ret;
        };

        // RVO?
        auto exact = get_dpids("exact"),
             include = get_dpids("include"),
             exclude = get_dpids("exclude");

        RouteSelector sel;

        if (auto brok = pt.get_optional<bool>("broken_flag")) {
            sel.set(broken_trigger, *brok);
        }
        if (auto flap = pt.get_optional<int>("flapping")) {
            THROW_IF(flap < 0, rest::http_error(400),
                    "Incorrect flapping: {}", *flap);
            sel.set(flapping, static_cast<uint16_t>(*flap));
        }
        if (auto drop = pt.get_optional<int>("drop_threshold")) {
            THROW_IF(*drop < 0 or *drop > 99, rest::http_error(400),
                    "Incorrect drop-threshold: {}", *drop);
            sel.set(drop_trigger, static_cast<uint8_t>(*drop));
        }
        if (auto util = pt.get_optional<int>("util_threshold")) {
            THROW_IF(*util < 0 or *util > 99, rest::http_error(400),
                    "Incorrect drop-threshold: {}", *util);
            sel.set(util_trigger, static_cast<uint8_t>(*util));
        }

        MetricsFlag metr {MetricsFlag::Hop};
        try {
            metr = MetricsFlag::_from_string(
                                 pt.get<std::string>("metrics", "Hop").c_str());
        } catch (const std::runtime_error& ex) {
            THROW(rest::http_error(400), "Incorrect metrics: {}",
                                         pt.get<std::string>("metrics"));
        }
        sel.set(metrics, metr);

        if (exact.size() > 0) {
            sel.set(metrics, +MetricsFlag::Manual);
            sel.set(exact_dpid, std::move(exact));
        } else {
            if (include.size() > 0) {
                sel.set(include_dpid, std::move(include));
            }
            if (exclude.size() > 0) {
                sel.set(exclude_dpid, std::move(exclude));
            }
        }

        rest::ptree ret;

        if (mod == RouteMod::AddPath) {
            auto path_id = app->newPath(id, sel);

            if (path_id == max_path_id) {
                ret.put("error", "Cannot create path");
            } else {
                ret.put("act", "path created");
                ret.put("route_id", id);
                ret.put("path_id", path_id);
            }
        } else if (mod == RouteMod::MovePath) {
            auto new_pos = pt.get<uint8_t>("new_pos", 0);
            auto moved = app->movePath(id, path_id, new_pos);
            moved ? ret.put("act", "Path moved")
                  : ret.put("error", "Cannot move path");
        } else if (mod == RouteMod::AddDynamic) {
            THROW_IF(!app->addDynamic(id, sel),
                     rest::http_error(404), "Route not found");
            ret.put("act", "Dynamic was added");
        } else if (mod == RouteMod::ModifyPath) {
            THROW_IF(!app->modifyPath(id, path_id, sel),
                     rest::http_error(404), "Route or path not found");
            ret.put("act", "Path settings were modified");
        }

        return ret;
    }

    rest::ptree Delete() override {
        rest::ptree ret;

        if (mod == RouteMod::DeletePath) {
            THROW_IF(!app->deletePath(id, path_id), rest::http_error(400),
                                    "Cannot remove path for route {}", id);
            ret.put("act", "Path was removed");
        }
        else if (mod == RouteMod::DeleteDynamic) {
            THROW_IF(!app->delDynamic(id), rest::http_error(404), "Route not found");
            ret.put("act", "Dynamic was removed");
        }

        return ret;
    }
};

struct RouteResource : rest::resource {
    Topology* app;

    explicit RouteResource(Topology* app)
        : app(app)
    { }

    rest::ptree Get() const override {
        RouteCollection col{app, "all"};
        return col.Get();
    }

    rest::ptree Post(rest::ptree const& pt) override {
        //create route
        rest::ptree ret;
        rest::ptree::const_assoc_iterator it;

        auto owner { ServiceFlag::None };
        auto metrics { MetricsFlag::None };
        uint64_t from, to;
        bool fail = false;

        it = pt.find("owner");
        if (it != pt.not_found())
            owner = ServiceFlag::_from_string(it->second.get_value<std::string>().c_str());
        else
            fail = true;
        it = pt.find("metrics");
        if (it != pt.not_found())
            metrics = MetricsFlag::_from_string(it->second.get_value<std::string>().c_str());
        else
            fail = true;
        it = pt.find("from");
        if (it != pt.not_found())
            from = it->second.get_value<uint64_t>();
        else
            fail = true;
        it = pt.find("to");
        if (it != pt.not_found())
            to = it->second.get_value<uint64_t>();
        else
            fail = true;
        //TODO: include, exclude, exact

        if (fail) {
            ret.put("error", "incorrect parameters");
            return ret;
        }

        uint32_t route_id = app->newRoute(from, to, 
                                          route_selector::app=owner,
                                          route_selector::metrics=metrics);
        RouteCollection col{app, route_id};
        return col.Get();
    }
};

class TopologyRest : public Application
{
    SIMPLE_APPLICATION(TopologyRest, "topology-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = Topology::get(loader);
        auto rest_ = RestListener::get(loader);

        rest_->mount(path_spec("/routes/id/(\\d+)/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                return RouteCollection { app, route_id };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });

        /* Path API */
        rest_->mount(path_spec("/routes/id/(\\d+)/add-path/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                return RouteResourceMods { app, route_id, RouteMod::AddPath };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/delete-path/(\\d+)/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                auto path_id = (uint8_t)boost::lexical_cast<uint32_t>(m[2].str());
                return RouteResourceMods { app, route_id, path_id, RouteMod::DeletePath };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/move-path/(\\d+)/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                auto path_id = (uint8_t)boost::lexical_cast<uint32_t>(m[2].str());
                return RouteResourceMods { app, route_id, path_id, RouteMod::MovePath };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/modify-path/(\\d+)/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                auto path_id = (uint8_t)boost::lexical_cast<uint32_t>(m[2].str());
                return RouteResourceMods { app, route_id, path_id, RouteMod::ModifyPath };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/add-dynamic/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                return RouteResourceMods { app, route_id, RouteMod::AddDynamic };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/delete-dynamic/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                return RouteResourceMods { app, route_id, RouteMod::DeleteDynamic };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });
        rest_->mount(path_spec("/routes/id/(\\d+)/predict-path/"), [=](const path_match& m) {
            try {
                auto route_id = boost::lexical_cast<uint32_t>(m[1].str());
                return RouteResourceMods { app, route_id, RouteMod::PredictPath };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            }
        });


        rest_->mount(path_spec("/routes/service/(\\S+)/"), [=](const path_match& m) {
            std::string service = m[1].str();
            return RouteCollection { app, service };
        });
        rest_->mount(path_spec("/routes/"), [=](const path_match& m) {
            return RouteResource { app };
        });
        rest_->mount(path_spec("/dump/topology/"), [=](const path_match& m) {
            return TopologyDump { app };
        });
    }
};

REGISTER_APPLICATION(TopologyRest, {"rest-listener", "topology", ""})

}
