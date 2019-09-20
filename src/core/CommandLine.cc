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

#include "CommandLine.hpp"

#include <thread>
#include <iostream>
#include <utility> // pair
#include <cstdio>
#include <cctype> // isspace

#include <QCoreApplication>
#include <histedit.h>

#include "Config.hpp"

namespace runos {

REGISTER_APPLICATION(CommandLine, {""})

struct CommandLine::implementation {
    std::unique_ptr<EditLine, decltype(&el_end)> el
        { nullptr, &el_end };

    std::unique_ptr<History, decltype(&history_end)> hist
        { history_init(), &history_end };

    std::vector< std::pair<cli_pattern, cli_command> >
        commands;

    HistEvent hev;
    bool keep_reading { true };

    bool handle(const char* cmd, int len);
    void run();
};

struct command_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct skip_command : public std::exception { };

CommandLine::CommandLine()
    : impl(new implementation)
{ }

CommandLine::~CommandLine() = default;

static const char* prompt(EditLine*)
{
    return "runos> ";
}

void CommandLine::init(Loader*, const Config& rootConfig)
{
    const auto& config = config_cd(rootConfig, "cli");

    history(impl->hist.get(), &impl->hev, H_SETSIZE,
            config_get(config, "history-size", 800));

    const char* argv0 = QCoreApplication::arguments().at(0).toLatin1().data();
    impl->el.reset(el_init(argv0, stdin, stdout, stderr));

    el_set(impl->el.get(), EL_PROMPT,
            &prompt);
    el_set(impl->el.get(), EL_EDITOR,
            config_get(config, "editor", "emacs").c_str());
    el_set(impl->el.get(), EL_HIST, history,
            impl->hist.get());
}

void CommandLine::startUp(Loader*)
{
    std::thread {&implementation::run, impl.get()}.detach();
}

void CommandLine::register_command(cli_pattern&& spec, cli_command&& fn)
{
    impl->commands.emplace_back(std::move(spec), std::move(fn));
}

void CommandLine::error_impl(std::string && msg)
{
    throw command_error(msg);
}

void CommandLine::skip()
{
    throw skip_command();
}

void CommandLine::implementation::run()
{
    for (;keep_reading;) {
        int len;
        const char* line = el_gets(el.get(), &len);

        if (line == nullptr)
            break;

        if (handle(line, len)) {
            if (not std::isspace(*line)) {
                history(hist.get(), &hev, H_ENTER, line);
            }
        }
    }
}

bool CommandLine::implementation::handle(const char* line, int len)
{
    if (line == nullptr)
        return false;

    const char* end = line + len;
    // skip whitespace
    while (line < end && std::isspace(*line))
        ++line;
    while (line < end && std::isspace(*(end-1)))
        --end;

    // empty line
    if (line == end)
        return false;

    for (const auto & cmd : commands) {
        std::cmatch match;
        if (not std::regex_match(line, end, match, cmd.first))
            continue;

        try {
            cmd.second(match);
            return true;
        } catch (skip_command&) {
            continue;
        } catch (command_error& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            return false;
        } catch (std::exception& ex) {
            std::cerr << "Uncaught exception: " << ex.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Uncaught exception";
            return false;
        }
    }

    std::cerr << std::string(line, end)
              << ": no such command" << std::endl;
    return false;
}

} // namespace runos
