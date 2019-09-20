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

#include <boost/lexical_cast.hpp>

#include "Application.hpp"
#include "Loader.hpp"
#include "RestListener.hpp"
#include "StatsBucket.hpp"

namespace runos {

struct BucketResource : rest::resource {
    UnsafeFlowStatsBucketPtr bucket;

    explicit BucketResource(FlowStatsBucketPtr bucket)
        : bucket(bucket.not_null())
    { }

    rest::ptree Get() const override {
        rest::ptree ret;

        auto stats = bucket->stats();
        auto& speed = stats.current_speed;
        auto& max = stats.max_speed;
        auto& integral = stats.integral;

        ret.put("current-speed.packets", speed.packets());
        ret.put("current-speed.bytes", speed.bytes());
        ret.put("current-speed.flows", speed.flows());

        ret.put("integral.packets", integral.packets());
        ret.put("integral.bytes", integral.bytes());
        ret.put("integral.flows", integral.flows());

        ret.put("max.packets", max.packets());
        ret.put("max.bytes", max.bytes());
        ret.put("max.flows", max.flows());

        return ret;
    }
};

class StatsBucketRest : public Application
{
    SIMPLE_APPLICATION(StatsBucketRest, "stats-bucket-rest")
public:
    void init(Loader* loader, const Config&) override
    {
        using rest::path_spec;
        using rest::path_match;

        auto app = StatsBucketManager::get(loader);
        auto rest_ = RestListener::get(loader);

        rest_->mount(path_spec("/bucket/(\\d+)/"), [=](const path_match& m) {
            try {
                auto id = boost::lexical_cast<int>(m[1].str());
                return BucketResource { app->bucket(id) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Bucket not found" );
            }
        });

        rest_->mount(path_spec("/bucket/([-_\\w]+)/"), [=](const path_match& m) {
            try {
                auto name = m[1].str();
                return BucketResource { app->bucket(name) };
            } catch (const boost::bad_lexical_cast& e) {
                THROW( rest::http_error(400), "Bad request: {}", e.what() ); // bad request
            } catch (const bad_pointer_access& e) {
                THROW( rest::http_error(404), "Bucket not found" );
            }
        });

    }
};

REGISTER_APPLICATION(StatsBucketRest, {"rest-listener", "stats-bucket-manager", ""})
}
