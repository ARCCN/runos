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

#include <runos/DeviceDb.hpp>

#include <runos/PropertySheet.hpp>
#include <runos/Utility.hpp>

#include <runos/core/throw.hpp>
#include <runos/core/logging.hpp>

#include <json11.hpp>
#include <boost/filesystem/fstream.hpp>

#include <any>
#include <map>
#include <sstream>

namespace json11 {
  auto format_as(Json::Type f) { return fmt::underlying(f); }} // json11
namespace fs = boost::filesystem;

namespace runos {

namespace devicedb {

constexpr const char* QueryBuilder::columns[];

QueryBuilder& QueryBuilder::dpid(uint64_t u)
{
    m_dpid = std::to_string(u);
    m_q[0] = m_dpid; return *this;
}

QueryBuilder& QueryBuilder::manufacturer(std::string_view s)
{
    m_q[1] = s; return *this;
}

QueryBuilder& QueryBuilder::hwVersion(std::string_view s)
{
    m_q[2] = s; return *this;
}

QueryBuilder& QueryBuilder::swVersion(std::string_view s)
{
    m_q[3] = s; return *this;
}

QueryBuilder& QueryBuilder::serialNum(std::string_view s)
{
    m_q[4] = s; return *this;
}

QueryBuilder& QueryBuilder::description(std::string_view s)
{
    m_q[5] = s; return *this;
}

ResultSet QueryBuilder::execute()
{
    return m_db->query(m_q.begin(), m_q.end());
}

} // devicedb

using namespace property_sheet;
using TEntry = PropertySheet::Entry;

struct DeviceDb::impl
{
    Rc<ResourceLocator> locator;
    Utility props2json;

    std::multimap<std::string, Json> files;
    FromJson<TEntry> load{ devicedb::QueryBuilder::columns };
    PropertySheet sheet{ devicedb::QueryBuilder::ncolumns };
};

DeviceDb::DeviceDb( Rc<ResourceLocator> locator )
    : m(new impl)
{
    m->locator = std::move(locator);
    m->props2json.setExecutable("python3")
                 .bindArgument(m->locator->find_directory("$tooldir/props2json"));
}

DeviceDb::~DeviceDb() = default;

size_t DeviceDb::add_props_file(std::string_view filename)
try {
    fs::path path = m->locator->find_file(filename);
    std::string json_str = m->props2json(path.string()).get();
    return add_json(std::string(filename), json_str);
} catch (ResourceNotFoundError const& e) {
    THROW_WITH_NESTED(devicedb::LoadError(), "Resource not found");
} catch (UtilityError const& e) {
    THROW_WITH_NESTED(devicedb::LoadError(), "Can't preprocess props file");
} catch (devicedb::LoadError& e) {
    e.with("file", std::string_view(filename.data(), filename.size()));
    throw;
}

size_t DeviceDb::add_default()
{
    return add_props_file("$etcdir/devicedb.props");
}

size_t DeviceDb::add_json_file(std::string_view filepath)
try {
    fs::ifstream fin;
    fs::path path(m->locator->find_file(filepath));

    fin.open(path, std::ios::in);
    THROW_IF(!fin, devicedb::LoadError(), "Can't open file");

    std::stringstream ss;
    ss << fin.rdbuf();
    return add_json(std::string(filepath), ss.str());
} catch (ResourceNotFoundError const& e) {
    THROW_WITH_NESTED(devicedb::LoadError(), "Resource not found");
} catch (devicedb::LoadError& e) {
    e.with("file", std::string_view(filepath.data(), filepath.size()));
    throw;
}

size_t DeviceDb::add_json(std::string name, std::string const& json_str)
{
    std::string err;
    Json json = Json::parse(json_str, err);

    if (not err.empty()) {
        THROW(devicedb::LoadError(), "Can't parse json: {}", err);
    }
    if (not json.is_array()) {
        THROW(devicedb::LoadError(),
              "Json array expected, Json::Type({}) found",
              json.type());
    }

    try {
        auto& items = json.array_items();
        size_t n = items.size();
        std::vector<TEntry> entries;
        entries.reserve(n);

        for (auto& e : json.array_items())
            entries.push_back(m->load(e));

        // Take ownership for inner string_views
        m->files.emplace(std::move(name), std::move(json)); 
        // Atomically apply changes (should not throw)
        for (auto& e : entries)
            m->sheet.append(std::move(e));

        return entries.size();
    } catch (property_sheet::JsonLoadError const& e) {
        THROW_WITH_NESTED(devicedb::LoadError(), "Bad json: {}", e.what());
    }
}

template<class... Types>
std::any to_any(std::variant<Types...> v) {
    return std::visit([](auto&& x) -> std::any {
        return std::move(x);
    }, v);
}

devicedb::QueryBuilder
DeviceDb::query() const
{
    return devicedb::QueryBuilder(shared_from_this());
}

devicedb::ResultSet
DeviceDb::query(std::string_view* begin, std::string_view* end) const
{
    devicedb::ResultSet ret;

    auto res = m->sheet.query(begin, end);
    ret.reserve(res.size());

    for (auto& p : res) {
        ret.emplace_back(p.name, to_any(p.val));
    }

    return ret;
}

} // runos
