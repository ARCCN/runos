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

#include <memory>
#include <functional> // function
#include <regex>
#include <utility>
#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "Application.hpp"
#include "Loader.hpp"

namespace runos {

// TODO: Replace regex to custom pattern string
//       with autodoc and autocompletion system.
typedef std::regex cli_pattern;
typedef std::cmatch cli_match;
typedef std::function<void(const cli_match&)> cli_command;

class CommandLine final : public Application
{
SIMPLE_APPLICATION(CommandLine, "command-line")
public:
    CommandLine();
    ~CommandLine();

    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

    void register_command(cli_pattern&& spec, cli_command&& fn);

    /**
     * Prints formatted message to the user and inserts newline.
     */
    template<class... Args>
    void print(std::string_view fmt, Args&& ... args)
    {
        fmt::print(std::cout, fmt, std::forward<Args>(args)...);
        std::cout << std::endl;
    }

    /**
     * Prints a string to the user. No newlines inserted.
     */
    void echo(std::string const& msg)
    {
        std::cout << msg;
    }

    /**
     * Reports error to the user.
     * @throws Implementation-specific exception containing error message.
     */
    template<class... Args>
    [[ noreturn ]] void error(std::string_view fmt, Args&& ... args)
    {
        error_impl(fmt::format(fmt, std::forward<Args>(args)...));
    }

    /**
     * Skip this command and try another.
     * @throws Implementation-specific exception.
     */
    [[ noreturn ]] void skip(); // throws

private:
    [[ noreturn ]] void error_impl(std::string&&);
    
    struct implementation;
    std::unique_ptr<implementation> impl;
};

} // namespace runos
