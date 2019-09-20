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

#include "ILinkDiscovery.hpp"

#include "RestListener.hpp"
#include <fluid/of13msg.hh>

#include <boost/property_tree/json_parser.hpp>
#include <QObject>
#include <sstream>

namespace runos {

namespace of13 = fluid_msg::of13;

struct LinkCollection : rest::resource {
    ILinkDiscovery* app;

    explicit LinkCollection(QObject* _app)
    { app = dynamic_cast<ILinkDiscovery*>(_app);  }

    rest::ptree Get() const override {
        rest::ptree root;
        rest::ptree links;

        for (const auto& l : app->links()) {
            rest::ptree lpt;
            std::istringstream is(l.to_json().dump());
            read_json(is, lpt);
            links.push_back(std::make_pair("", std::move(lpt)));
        }
        root.add_child("array", links);
        root.put("_size", links.size());
        return root;
    }
};

class LinkDiscoveryRest : public Application
{
    SIMPLE_APPLICATION(LinkDiscoveryRest, "link-discovery-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = ILinkDiscovery::get(loader);
        auto rest_ = RestListener::get(loader);

        rest_->mount(path_spec("/links/"), [=](const path_match&) {
            return LinkCollection {app};
        });
    }
};

REGISTER_APPLICATION(LinkDiscoveryRest, {"rest-listener", "link-discovery", ""})

}
