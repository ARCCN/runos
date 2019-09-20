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
#include <runos/core/throw.hpp>
#include <json11.hpp>

#include <regex>
#include <vector>
#include <variant>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <initializer_list>

namespace runos {

namespace property_sheet {
    using namespace std::rel_ops;
    using Num = int64_t;
    using Str = std::string_view;
    using Bool = bool;
    using Value = std::variant<Str, Num, Bool>;

    struct JsonLoadError : exception_root, runtime_error_tag
    { };

    template<class T>
    struct FromJson;

    template<>
    struct FromJson< Value > {
        Value operator()(Json const& obj) const;
    };

    // TODO: generalize with variadics when needed
    template<class V1, class V2, class V3>
    struct FromJson< std::variant<V1, V2, V3> > {
        std::variant<V1, V2, V3> operator()(Json const& jobj) const {
            std::string err;
            auto& obj = jobj.object_items();
            THROW_IF(not jobj.has_shape({{"type", Json::STRING}}, err),
                     JsonLoadError(), err);

            auto& type = obj.at("type").string_value();
            if (type == FromJson<V1>::json_type) {
                return FromJson<V1>{}(obj);
            } else if (type == FromJson<V2>::json_type) {
                return FromJson<V2>{}(obj);
            } else if (type == FromJson<V3>::json_type) {
                return FromJson<V3>{}(obj);
            } else {
                THROW(JsonLoadError(), "Unknown matcher type: {}", type);
            }
        }
    };

    template<class T = void>
    struct DoMatch;

    template<>
    struct DoMatch<void> {
        template<class T>
        size_t operator()(T const& m, std::string_view s) const {
            return DoMatch<T>{}(m, s);
        }
    };

    template<class... Types>
    struct DoMatch< std::variant<Types...> > {
        size_t operator()(std::variant<Types...> const& m, std::string_view s) const {
            return std::visit([&](auto& vm) { return DoMatch<>{}(vm, s); }, m);
        }
    };

    struct AnyMatch { };

    template<>
    struct DoMatch<AnyMatch> {
        constexpr static size_t score = 1;
        size_t operator()(AnyMatch const&, std::string_view) const {
            return score;
        }
    };

    template<>
    struct FromJson<AnyMatch> {
        static constexpr char json_type[] = "any";
        AnyMatch operator()(Json const&) const {
            return {};
        }
    };

    struct ExactMatch {
        std::string_view value;
    };

    template<>
    struct FromJson<ExactMatch> {
        static constexpr char json_type[] = "exact";
        ExactMatch operator()(Json const& jobj) const;
    };

    template<>
    struct DoMatch<ExactMatch> {
        constexpr static size_t score = 3;
        size_t operator()(ExactMatch const& m, std::string_view s) const {
            return (m.value == s) ? score : 0;
        }
    };

    struct Version {
        std::vector<long long> parts;

        Version() = default;
        explicit Version(std::string_view s);
        operator bool() const { return not parts.empty(); }
    };

    inline bool operator==(Version const& lhs, Version const& rhs) { return lhs.parts == rhs.parts; }
    inline bool operator<(Version const& lhs, Version const& rhs) { return lhs.parts < rhs.parts; }
    inline bool operator<=(Version const& lhs, Version const& rhs) { return lhs.parts <= rhs.parts; }
    inline bool operator>(Version const& lhs, Version const& rhs) { return lhs.parts > rhs.parts; }
    inline bool operator>=(Version const& lhs, Version const& rhs) { return lhs.parts >= rhs.parts; }

    struct VersionChecker {
        Version low;
        bool low_strict;
        Version high;
        bool high_strict;

        bool operator()(std::string_view s) const;
    };

    template<>
    struct FromJson<VersionChecker> {
        VersionChecker operator()(Json const& jobj) const;
    };

    struct FuzzyMatch {
        using SMatchChecker = std::function<bool(std::string_view)>;
        using CheckerList = std::vector< std::pair<size_t, SMatchChecker> >;

        std::regex re;
        CheckerList smatch;
    };

    template<>
    struct FromJson<FuzzyMatch> {
        static constexpr char json_type[] = "fuzzy";
        FuzzyMatch operator()(Json const& jobj) const;
    };

    template<>
    struct DoMatch<FuzzyMatch> {
        static constexpr size_t score = 2;
        size_t operator()(FuzzyMatch const& m, std::string_view s) const;
    };

    struct Property {
        std::string_view name;
        Value val;
    };

    inline bool operator==(Property const& lhs, Property const& rhs)
    {
        return std::tie(lhs.name, lhs.val) == std::tie(rhs.name, rhs.val);
    }

    template<class Match>
    struct Entry {
        std::vector<Match> selector;
        std::vector<Property> props;
    };

    template<class Match>
    struct FromJson< Entry<Match> > {
        using TEntry = Entry<Match>;

        template<class RangeOfStringViews>
        explicit FromJson( RangeOfStringViews const& colnames )
        {
            size_t i = 0;
            for (auto& v : colnames) {
                colname_[std::string_view(v)] = i++;
            }
        }

        explicit FromJson( std::initializer_list<std::string_view> colnames )
        {
            size_t i = 0;
            for (auto& v : colnames) {
                colname_[v] = i++;
            }
        }

        TEntry operator()(Json const& jobj) const {
            std::string err;
            auto& obj = jobj.object_items();
            THROW_IF(not jobj.has_shape({{"selector", Json::OBJECT},
                                         {"props", Json::OBJECT}}, err),
                     JsonLoadError(), err);

            auto& jselector = obj.at("selector").object_items();
            auto& jprops = obj.at("props").object_items();

            TEntry ret;
            ret.selector.resize( colname_.size() );

            for (auto& p : jselector) {
                auto& jcol = p.first;
                auto& jmatch = p.second;
                THROW_IF(not jmatch.is_object(), JsonLoadError());
                THROW_IF(colname_.count(jcol) == 0, JsonLoadError(),
                         "Unknown selector name: {}", jcol);
                size_t idx = colname_.at(jcol);
                ret.selector[idx] = FromJson<Match>{}(jmatch);
            }

            for (auto& p : jprops) {
                ret.props.push_back({std::string_view(p.first),
                                     FromJson<Value>{}(p.second)});
            }

            return ret;
        }
    private:
        std::unordered_map<std::string_view, size_t> colname_;
    };
}

class PropertySheet {
public:
    using Match = std::variant<property_sheet::AnyMatch,
                          property_sheet::ExactMatch,
                          property_sheet::FuzzyMatch>;
    using Entry = property_sheet::Entry<Match>;
    using Property = property_sheet::Property;
    using Score = std::vector<size_t>;
    using PropSet = std::vector<Property>;

    explicit PropertySheet(size_t columns)
        : columns(columns)
    { }

    void append(Entry e);
    void append(std::vector<Match> selector,
                std::vector<Property> props);

    // Returns a sorted (by name) list of best-matched properties
    template<class Array>
    PropSet query(Array const& fields) const
    {
        return query(&fields[0], &fields[fields.size()-1]);
    }

    PropSet query(std::initializer_list<std::string_view> fields) const
    {
        return query(fields.begin(), fields.end());
    }

    PropSet query(const std::string_view* begin,
                  const std::string_view* end) const;

    std::optional<Score> match(std::vector<Match> const& selector,
                          const std::string_view* begin,
                          const std::string_view* end) const;

private:
    size_t columns;
    std::vector<Entry> entries;
};

} // runos
