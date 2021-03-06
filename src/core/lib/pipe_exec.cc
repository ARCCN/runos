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

#include "pipe_exec.hpp"

#include <runos/core/throw.hpp>

#include <memory>
#include <cstdio>

std::string runos::pipe_exec(std::string const& cmd) {
    FILE* handle = popen(cmd.c_str(), "r");
    if (handle == NULL) THROW(exec_error(cmd));

    std::unique_ptr<std::FILE, decltype(&pclose)> pipe(handle, pclose);

    char buffer[128];
    std::string result;
    while (!feof(handle)) {
        if (fgets(buffer, 128, handle) != NULL)
            result += buffer;
    }
    return result;
}
