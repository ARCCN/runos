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

#include <string>

namespace runos {

struct exec_error : exception_root, runtime_error_tag
{
    explicit exec_error(std::string cmd)
        : cmd_(static_cast<std::string&&>(cmd))
    {
        with("command", cmd);
    }

protected:
    std::string cmd_;
};

std::string pipe_exec(std::string const& cmd);

} // runos
