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

#pragma once


#include <runos/core/exception.hpp>
#include <runos/core/throw.hpp>

#include "Application.hpp"
#include "Loader.hpp"

#undef foreach
#include <boost/property_tree/ptree.hpp>

#include <memory> // unique_ptr
#include <functional> // function
#include <regex>

namespace runos {

namespace rest {
    using boost::property_tree::ptree;

    struct http_error : exception_root, runtime_error_tag
    {
        explicit http_error(unsigned code)
            : code_(code)
        {
            with("code", code_);
        }

        unsigned code() const { return code_; }
    private:
        unsigned code_;
    };

    struct resource {
        virtual ptree Get() const
        { THROW(http_error(404), "Unimplemented"); }

        virtual bool Head() const
        try {
            (void) Get();
            return true;
        } catch (const http_error& err) {
            if (err.code() == 404)
                return false;
            throw;
        }

        virtual ptree Put(const ptree&)
        { THROW(http_error(405), "Unimplemented"); }

        virtual ptree Post(const ptree&)
        { THROW(http_error(405), "Unimplemented"); }

        virtual ptree Delete()
        { THROW(http_error(405), "Unimplemented"); }

        // TODO: subscribe
    };

    typedef std::regex path_spec;
    typedef std::smatch path_match;

    typedef std::function<void(resource&)>
        resource_continuation;
    
    typedef std::function<void(const path_match&, resource_continuation)>
        resource_mapper;
}

class RestListener : public Application
{
SIMPLE_APPLICATION(RestListener, "rest-listener")
public:
    RestListener();
    ~RestListener() noexcept;

    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

    template<class Mapper>
    void mount(const rest::path_spec& pathspec, Mapper mapper)
    {
        mount_impl(pathspec,
            [=](const rest::path_match& match, rest::resource_continuation c) {
                auto res = mapper(match); c(res);
            });
    }

private:
    void mount_impl(const rest::path_spec& pathspec,
                    rest::resource_mapper mapper);

    struct implementation;
    std::unique_ptr<implementation> impl;
};

} // namespace runos
