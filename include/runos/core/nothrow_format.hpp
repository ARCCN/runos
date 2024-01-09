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

#include <fmt/format.h>

namespace runos {

/**
 * Nothrow string formatting.
 */
template<class ... Args>
std::string nothrow_format(const char* format_str, Args const& ... args) noexcept
try {
    return fmt::format(format_str, args...);
} catch (fmt::format_error& ex) {
    try {
        return fmt::format("{} (FormatError: {})", format_str, ex.what());
    } catch (std::bad_alloc&) {
        return std::string();
    }
} catch (std::bad_alloc&) {
    return std::string();
}

} //  runos
