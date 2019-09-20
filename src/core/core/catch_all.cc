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

#include <runos/core/catch_all.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/literals.hpp>
#include <runos/core/demangle.hpp>

#include <boost/exception/all.hpp>

#include <utility>
#include <exception>
#include <cstring>

namespace runos {

diagnostic_information::diagnostic_information() noexcept
{ }

diagnostic_information::diagnostic_information(unknown_exception_tag)
    : what("Unknown exception")
{ }

template<class E>
diagnostic_information* from_nested( E const& e )
{
    auto nested = catch_all([&]() {
        std::rethrow_if_nested(e);
    }, false);

    return nested ? new diagnostic_information{std::move(nested.value())}
                  : nullptr;
}

static auto make_with( extended_exception const& e )
{
    std::forward_list<std::string> ret;
    for (auto& p : e.with()) {
        ret.push_front("{} = {}"_format(p.first, p.second));
    }
    return ret;
}

diagnostic_information::diagnostic_information( std::exception_ptr e )
    : diagnostic_information{ std::move(
        catch_all([e = std::move(e)]() {
            if (e) std::rethrow_exception(e);
        }, false)).value_or(diagnostic_information{})
    }
{ }

diagnostic_information::diagnostic_information( extended_exception const& e)
    : what(e.what())
    , where(fmt::format("{} @ {}:{}", e.where().function_name()
                                    , e.where().file_name()
                                    , e.where().line()))
    , exception_type(demangle(e.type().name()))
    , condition((strlen(e.condition()) != 0)
                ? "{} [{}]"_format(e.condition(), e.condition_explained())
                : std::string())
    , with( make_with(e) )
    , nested( from_nested(e) )
{ }

diagnostic_information::diagnostic_information( std::exception const& e)
    : what(e.what())
    , exception_type(demangle(typeid(e).name()))
    , nested( from_nested(e) )
{ }

static std::string what_from_boost( boost::exception const& e)
{
    if (auto stdex = dynamic_cast<const std::exception*>(&e)) {
        return stdex->what();
    } else {
        return std::string();
    }
}

static std::string where_from_boost_errinfo( boost::exception const& e )
{
    auto fun = boost::get_error_info<boost::throw_function>(e);
    auto file = boost::get_error_info<boost::throw_file>(e);
    auto line = boost::get_error_info<boost::throw_line>(e);
    return (fun && file && line) ?
        fmt::format("{} @ {}:{}", *fun, *file, *line)
      : std::string();
}

static std::string type_from_boost_errinfo( boost::exception const& e )
{
    auto name = boost::get_error_info<boost::errinfo_type_info_name>(e);
    return name ? std::move(*name) : demangle(typeid(e).name());
}

static diagnostic_information* from_boost_errinfo_nested( boost::exception const& e )
{
    auto nested = boost::get_error_info<boost::errinfo_nested_exception>(e);
    if (nested) {
        auto diag = catch_all([&]() { boost::rethrow_exception(*nested); }, false);
        return diag ? new diagnostic_information{std::move(diag.value())} : nullptr;
    } else {
        return nullptr;
    }
}

static auto with_from_boost_errinfo( boost::exception const& e )
{
    std::forward_list<std::string> ret;

    auto api_function = boost::get_error_info<boost::errinfo_api_function>(e);
    if (api_function) ret.push_front("api_function = {}"_format(*api_function));

    auto at_line = boost::get_error_info<boost::errinfo_at_line>(e);
    if (at_line) ret.push_front("at_line = {}"_format(*at_line));

    auto file_name = boost::get_error_info<boost::errinfo_file_name>(e);
    if (file_name) ret.push_front("file_name = {}"_format(*file_name));

    auto file_open_mode = boost::get_error_info<boost::errinfo_file_open_mode>(e);
    if (file_open_mode) ret.push_front("file_open_mode = {}"_format(*file_open_mode));

    return ret;
}

diagnostic_information::diagnostic_information( boost::exception const& e)
    : what( what_from_boost(e) )
    , where( where_from_boost_errinfo(e) )
    , exception_type( type_from_boost_errinfo(e) )
    , with( with_from_boost_errinfo(e) )
    , nested( from_boost_errinfo_nested(e) ) // use errinfo?
{ }

static void log_extra_info(diagnostic_information const& self)
{
    if (!self.condition.empty())
        LOG(ERROR) << "  condition: " << self.condition;
    if (!self.what.empty())
        LOG(ERROR) << "  what: " << self.what;
    if (!self.where.empty())
        LOG(ERROR) << "  thrown at: " << self.where;
    for (auto& data : self.with) {
        LOG(ERROR) << "  with: " << data;
    }
    if (self.nested) {
        auto& nested = *self.nested;
        LOG(ERROR) << "after nested exception of type `" << nested.exception_type << "`:";
        log_extra_info(nested);
    }
}

void diagnostic_information::log() const
{
    LOG(ERROR) << "Unhandled exception of type `" << exception_type << "`:";
    log_extra_info(*this);
}

diagnostic_information diagnostic_information::get() noexcept
try {
    return diagnostic_information{ std::current_exception() };
} catch (std::bad_alloc& e) {
    return diagnostic_information{};
}

} // runos
