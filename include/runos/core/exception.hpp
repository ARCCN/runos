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

#include <runos/core/source_location.hpp>

#include <fmt/format.h>

#include <exception>
#include <forward_list>
#include <string>
#include <typeinfo>

namespace runos {

/**
 * Semantic tags that help `make_exception` create good exception hierarchy.
 */
struct runtime_error_tag { };
struct logic_error_tag { };
struct invalid_argument_tag { };

/**
 * Proxy class used to propogate `std::exception` methods.
 */
struct std_exception_proxy
{
    /** Makes class polymorphic. */
    std_exception_proxy() noexcept = default;
    std_exception_proxy(std_exception_proxy const&) = default;
    std_exception_proxy(std_exception_proxy &&) noexcept = default;
    std_exception_proxy& operator=(std_exception_proxy const&) = delete;
    std_exception_proxy& operator=(std_exception_proxy &&) noexcept = delete;
    virtual ~std_exception_proxy() noexcept = default;

    /**
     * Same as `std::exception::what`.
     */
    const char* what() const noexcept;
};

/**
 * Proxy class used to propogate `std::exception` and `extended_exception`
 * methods to `catch (myexception& e)` blocks.
 */
class exception_root : public std_exception_proxy {
private:
    std::forward_list< std::pair<const char*, std::string> > with_;

protected:
    template<class T>
    void with(const char* name, T const& info, const char* format = "{}") noexcept
    try {
        with_.emplace_front(name, fmt::format(format, info));
    } catch (fmt::format_error& e) {
        try {
            with_.emplace_front(name, fmt::format("FormatError: {}", e.what()));
        } catch (std::bad_alloc&) { }
    } catch (std::bad_alloc&) { }

public:
    const source_location& where() const noexcept;
    const std::type_info& type() const noexcept;
    const char* condition() const noexcept;
    const std::string& condition_explained() const noexcept;

    decltype(auto) with() const noexcept { return with_; }
};

/**
 * Holds source location and original exception type inside exception.
 * Designed to be filled by `THROW` macro series.
 */
struct extended_exception : std_exception_proxy
{
    explicit extended_exception(source_location&& where,
                                std::type_info const& type,
                                const char* expr,
                                std::string&& explained) noexcept
        : where_(static_cast<source_location&&>(where)) // move
        , type_(type)
        , condition_(expr)
        , condition_explained_(static_cast<std::string&&>(explained)) // move
    { }

    explicit extended_exception(source_location&& where,
                                std::type_info const& type) noexcept
        : extended_exception{
            static_cast<source_location&&>(where), type, "", std::string()
          }
    { }

    /**
     * Location where exception thrown.
     */
    const source_location& where() const noexcept { return where_; }

    /**
     * Human-friendly exception type if possible.
     */
    const std::type_info& type() const noexcept { return type_; }

    /**
     * Expression caused exception to be thrown from THROW_IF.
     */
    const char* condition() const noexcept { return condition_; }

    /**
     * Like `condition()`, but with more details about values used in expression.
     */
    const std::string& condition_explained() const noexcept { return condition_explained_; }

    /**
     * Returns a serialized exception data as a const reference to the container
     * of (const char* name, std::string value) pairs.
     */
    decltype(auto) with() const noexcept
    {
        static std::forward_list< std::pair<const char*, std::string> > empty;
        if (auto self = dynamic_cast<const exception_root*>(this)) {
            return self->with();
        } else {
            return empty;
        }
    }

private:
    source_location where_;
    const std::type_info& type_;
    const char* condition_;
    std::string condition_explained_;
};

/**
 * invalid_argument is the only exception class useful as is without inheritance.
 */
struct invalid_argument : exception_root, invalid_argument_tag { };

} // runos
