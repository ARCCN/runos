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
#include <runos/core/future-decl.hpp>
#include <boost/filesystem/path.hpp>

#include <vector>
#include <utility>
#include <string>
#include <string_view>

namespace runos {

struct UtilityError : exception_root, runtime_error_tag
{
    explicit UtilityError(const std::string& cmd) {
        with("cmd", cmd);
    }
};

struct UtilityProcessError : UtilityError
{
    using UtilityError::UtilityError;
};

struct UtilityOSError : UtilityError
{
    using UtilityError::UtilityError;
};

class Utility
{
public:
    using path = boost::filesystem::path;

    Utility() = default;
    Utility& setExecutable(path exe) { m_executable = std::move(exe); return *this; }
    Utility& bindArgument(std::string_view arg) { m_args.push_back(quote(arg)); return *this; }
    Utility& bindArgument(path file) { return bindArgument(std::string_view(file.string())); }
    Utility& bindArgument(const char* arg) { return bindArgument(std::string_view(arg)); }
    Utility& bindArgument(std::string const& arg) { return bindArgument(std::string_view(arg)); }

    future<std::string>
    exec(std::initializer_list<std::string_view> args) const;

    template<class... Args>
    future<std::string>
    operator()(Args const& ... args) const
    {
        return exec({std::string_view(args)...});
    }

private:
    path m_executable;
    std::vector<std::string> m_args;

    static std::string quote(std::string_view arg);
};

} // runos
