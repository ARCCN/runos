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

#include <runos/core/exception.hpp>
#include <runos/core/crash_reporter.hpp>

namespace runos {

/*
 * std_exception_proxy
 */

const char* std_exception_proxy::what() const noexcept
{
    auto self = dynamic_cast<const std::exception*>(this);
    return self ? self->what() : "";
}

/*
 * exception_root
 */

const source_location& exception_root::where() const noexcept
{
    if (auto self = dynamic_cast<const extended_exception*>(this)) {
        return self->where();
    } else {
        static const auto unspecified
            = source_location{"unknown", "unknown", 0, 0};
        return unspecified;
    }
}

const std::type_info& exception_root::type() const noexcept
{
    if (auto self = dynamic_cast<const extended_exception*>(this)) {
        return self->type();
    } else {
        return typeid(*this);
    }
}

const char* exception_root::condition() const noexcept
{
    if (auto self = dynamic_cast<const extended_exception*>(this)) {
        return self->condition();
    } else {
        return "";
    }
}

const std::string& exception_root::condition_explained() const noexcept
{
    if (auto self = dynamic_cast<const extended_exception*>(this)) {
        return self->condition_explained();
    } else {
        static const std::string empty;
        return empty;
    }
}

} // runos
