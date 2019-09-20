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

#include <runos/Utility.hpp>

#include <runos/core/throw.hpp>
#include <process.hpp>

#include <sys/wait.h>

namespace runos {

struct ExecutionContext {
    promise<std::string> out;

    std::string stdout;
    std::string stderr;
};

std::string Utility::quote(std::string_view arg)
{
    std::string ret;
    ret.reserve(arg.size() + 10);

    ret += "\"";
    for (auto c : arg) {
        if (c == '"') {
            ret += "\\\"";
        } else if (c == '\\') {
            ret += "\\\\";
        } else {
            ret += c;
        }
    }
    ret += "\"";

    return ret;
}

future<std::string>
Utility::exec(std::initializer_list<std::string_view> args) const
{
    if (m_executable.empty()) {
        THROW(UtilityError(""), "Executable is not set");
    }

    auto ctx = std::make_shared<ExecutionContext>();

    std::string cmd = m_executable.string();
    for (auto& arg : m_args) { cmd += " "; cmd += arg; }
    for (auto& arg : args) { cmd += " "; cmd += quote(arg); }

    std::thread([cmd = std::move(cmd), ctx]() {
        auto on_stdout = [ctx](const char* data, size_t size) {
            ctx->stdout.append(data, size);
        };

        auto on_stderr = [ctx](const char* data, size_t size) {
            ctx->stderr.append(data, size);
        };

        Process p {cmd, "", on_stdout, on_stderr};

        auto status = p.get_exit_status();
        if (WIFEXITED(status)) {
            auto code = WEXITSTATUS(status);
            if (code == 0) {
                ctx->out.set_value(std::move(ctx->stdout));
            } else if (code == 126) {
                ctx->out.set_exception(MAKE_EXCEPTION(
                    UtilityOSError(cmd), "Command invoked cannot execute"));
            } else if (code == 127) {
                ctx->out.set_exception(MAKE_EXCEPTION(
                    UtilityOSError(cmd), "Command not found"));
            } else if (code == 128) {
                ctx->out.set_exception(MAKE_EXCEPTION(
                    UtilityOSError(cmd), "Invalid argument to exit"));
            } else {
                ctx->out.set_exception(MAKE_EXCEPTION(
                    UtilityProcessError(cmd), "Exited with code {}: {}",
                    code, std::move(ctx->stderr)));
            }
        } else if (WIFSIGNALED(status)) {
            auto sig = WTERMSIG(status);
            ctx->out.set_exception(MAKE_EXCEPTION(
                UtilityProcessError(cmd), "Killed by signal {}", sig));
        } else {
            ctx->out.set_exception(MAKE_EXCEPTION(
                UtilityError(cmd), "Unknown exit status: {]", status));
        }
    }).detach();

    return ctx->out.get_future();;
}

} // runos
