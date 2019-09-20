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
#include <runos/core/ptr.hpp>

#include <boost/filesystem/path.hpp>

#include <string_view>

namespace runos {

struct ResourceNotFoundError : exception_root, runtime_error_tag
{
    explicit ResourceNotFoundError(std::string_view res) {
        with("resource", std::string(res));
    }
};

struct ResourceLocator {
    virtual ~ResourceLocator() { }

    using path = boost::filesystem::path;

    virtual path find_file(std::string_view res) const = 0;
    virtual path find_executable(std::string_view res) const = 0;
    virtual path find_directory(std::string_view res) const = 0;

    static Rc<ResourceLocator> make(std::string const& etcdir,
                                    std::string const& tooldir);

    static Rc<ResourceLocator> get();
};

} // runos
