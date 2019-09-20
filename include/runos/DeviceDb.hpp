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

#include <runos/ResourceLocator.hpp>

#include <runos/core/exception.hpp>
#include <runos/core/ptr.hpp>

#include <any>
#include <array>
#include <string>
#include <type_traits>
#include <string_view>

namespace runos {

class DeviceDb;
class QueryBuilder;

namespace devicedb {

using ResultSet = std::vector<std::pair<std::string_view, std::any>>;

class QueryBuilder final
{
    friend class ::runos::DeviceDb;
public:
    static constexpr const char* columns[] = {
        "dpid",
        "manufacturer",
        "hwVersion",
        "swVersion",
        "serialNum",
        "description"
    };

    static constexpr size_t ncolumns =
        std::extent<decltype(columns), 0>::value;

    QueryBuilder& dpid(uint64_t u);
    QueryBuilder& manufacturer(std::string_view s);
    QueryBuilder& hwVersion(std::string_view s);
    QueryBuilder& swVersion(std::string_view s);
    QueryBuilder& serialNum(std::string_view s);
    QueryBuilder& description(std::string_view s);

    ResultSet execute();

private:
    Rc<const DeviceDb> m_db;

    QueryBuilder(Rc<const DeviceDb> db)
        : m_db(db)
    { }

    std::string m_dpid;
    std::array<std::string_view, 6> m_q;
};

struct LoadError : exception_root, runtime_error_tag
{
    friend class ::runos::DeviceDb;
};

} // devicedb

class DeviceDb final
    : public std::enable_shared_from_this<DeviceDb>
{
public:
    using ResultSet = devicedb::ResultSet;

    explicit
    DeviceDb( Rc<ResourceLocator> locator = ResourceLocator::get() );
    ~DeviceDb( );

    size_t add_default();
    size_t add_props_file(std::string_view filename);
    size_t add_json_file(std::string_view filepath);
    size_t add_json(std::string name, std::string const& json);

    devicedb::QueryBuilder query() const;

private:
    friend class devicedb::QueryBuilder;

    ResultSet query(std::string_view* begin, std::string_view* end) const;

    struct impl;
    Unique<impl> m;
};

}
