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

/** @file */
#pragma once

#include <QtCore>
#include <vector>

#include "Application.hh"
#include "Loader.hh"
#include "Common.hh"
#include "ILinkDiscovery.hh"
#include "Rest.hh"
#include "RestListener.hh"
#include "AppObject.hh"
#include "json11.hpp"

typedef std::vector< switch_and_port > data_link_route;

/**
 * This application tracking network for its topology.
 *
 * You may get description of networks topology by REST request : GET /api/topology/links
 *
 */
class Topology : public Application, RestHandler {
    Q_OBJECT
    SIMPLE_APPLICATION(Topology, "topology")
public:
    Topology();
    void init(Loader* provider, const Config& config) override;
    ~Topology();

    bool eventable() override {return true;}
    std::string displayedName() override { return "Topology"; }
    std::string page() override { return "topology.html"; }
    AppType type() override { return AppType::Application; }
    json11::Json handleGET(std::vector<std::string> params, std::string body) override;

    /**
     * compute route between two switchs
     * @param from switch route will be computed
     * @param to switch be computed
     *
     * @return computed route. Hop by hop.
     */
    data_link_route computeRoute(uint64_t from, uint64_t to);

protected slots:
    void linkDiscovered(switch_and_port from, switch_and_port to);
    void linkBroken(switch_and_port from, switch_and_port to);

private:
    struct TopologyImpl* m;
    std::vector<class Link*> topo;

    Link* getLink(switch_and_port from, switch_and_port to);
};
