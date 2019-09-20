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

#include <runos/PropertySheet.hpp>

#include <runos/core/assert.hpp>
#include <runos/core/logging.hpp>

#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/map.hpp>

#include <functional>
#include <map>

namespace runos {

namespace property_sheet {
    
    // Parses version number like "2.3.1" into vector [2,3,1]
    Version::Version(std::string_view s) {
        while (not s.empty()) {
            size_t dot = s.find('.');
            std::string_view part = s.substr(0, dot);
            if (dot != std::string_view::npos) {
                s.remove_prefix(dot+1);
            } else {
                s.remove_prefix(s.size());
            }
            parts.push_back(std::stoll(std::string(part)));
        }

        // Remove trailing zeros
        while (parts.size() > 1 && parts.back() == 0)
            parts.pop_back();
        parts.shrink_to_fit();
    }

    bool VersionChecker::operator()(std::string_view s) const {
        Version v{ s };
        if (not v) return true;
        bool low_ok = true;
        bool high_ok = true;
        if (low) low_ok = low_strict ? (v > low) : (v >= low);
        if (high) high_ok = high_strict ? (v < high) : (v <= high);
        return low_ok && high_ok;
    }

    VersionChecker FromJson<VersionChecker>::operator()(Json const& jobj) const {
        auto& j = jobj.object_items();

        VersionChecker ret;

        if (j.count("==")) {
            ret.low_strict = false;
            ret.low = Version(j.at("==").string_value());
            ret.high_strict = false;
            ret.high = Version(j.at("==").string_value());
            return ret;
        }

        if (j.count(">")) {
            ret.low_strict = true;
            ret.low = Version(j.at(">").string_value());
        } else if (j.count(">=")) {
            ret.low_strict = false;
            ret.low = Version(j.at(">=").string_value());
        }

        if (j.count("<")) {
            ret.high_strict = true;
            ret.high = Version(j.at("<").string_value());
        } else if (j.count("<=")) {
            ret.high_strict = false;
            ret.high = Version(j.at("<=").string_value());
        }

        return ret;
    }

    constexpr char FromJson<AnyMatch>::json_type[];
    constexpr char FromJson<ExactMatch>::json_type[];
    constexpr char FromJson<FuzzyMatch>::json_type[];

    Value FromJson<Value>::operator()(Json const& obj) const {
        switch (obj.type()) {
            case Json::STRING:
                return obj.string_value();
            case Json::NUMBER:
                return (Num) obj.number_value();
            case Json::BOOL:
                return obj.bool_value();
            default:
                THROW(JsonLoadError(), "Unsupported property type");
        }
    }

    ExactMatch FromJson<ExactMatch>::operator()(Json const& jobj) const {
        std::string err;
        auto& obj = jobj.object_items();
        THROW_IF(not jobj.has_shape({{"value", Json::STRING}}, err),
                 JsonLoadError(), err);
        return ExactMatch{ obj.at("value").string_value() };
    }

    FuzzyMatch FromJson<FuzzyMatch>::operator()(Json const& jobj) const {
        std::string err;
        auto& obj = jobj.object_items();
        THROW_IF(not jobj.has_shape({{"regex", Json::STRING},
                                     {"smatch", Json::OBJECT}}, err),
                 JsonLoadError(), err);

        try {
            std::string re_string = obj.at("regex").string_value();
            std::regex re{ re_string };

            FuzzyMatch::CheckerList smatch;
            for (auto& p : obj.at("smatch").object_items()) {
                long grp = std::stol(p.first);
                auto& j = p.second;
                THROW_IF(grp > re.mark_count(), JsonLoadError(),
                         "Bad submatch group {} in /{}/", grp, re_string);
                THROW_IF(j.object_items().count("check") == 0,
                         JsonLoadError(), "Bad smatch shape: no type id");

                auto& type = j.object_items().at("check").string_value();
                if (type == "version") {
                    smatch.emplace_back(grp, FromJson<VersionChecker>{}(j));
                } else {
                    THROW(JsonLoadError(), "Unknown smatch checker: {}", type);
                }
            }

            return FuzzyMatch{ std::move(re), std::move(smatch) };
        } catch (std::regex_error const& e) {
            THROW_WITH_NESTED(JsonLoadError(), "Bad regex: {}", e.what());
        }
    }

    size_t DoMatch<FuzzyMatch>::operator()(FuzzyMatch const& m,
                                           std::string_view s) const {
        using namespace ranges;

        std::match_results<std::string_view::const_iterator> res;
        if (not std::regex_search(s.begin(), s.end(), res, m.re))
            return 0;

        return all_of(m.smatch | view::transform([&](auto& x) {
            size_t grp = x.first;
            auto& chk = x.second;

            std::string_view submatch = s.substr(res.position(grp),
                                                 res.length(grp));
            return chk(submatch);
        }), [](bool b) { return b == true; }) ? score : 0;
    }

}

void PropertySheet::append(Entry e)
{
    RUNOS_ASSERT(e.selector.size() == columns);
    entries.push_back(std::move(e));
}

void PropertySheet::append(std::vector<Match> selector,
                           std::vector<Property> props)
{
    append(Entry{ std::move(selector), std::move(props) });
}

auto PropertySheet::match(std::vector<Match> const& selector,
                          const std::string_view* fields,
                          const std::string_view* end) const
    -> std::optional<Score>
{
    RUNOS_ASSERT((size_t) std::distance(fields, end) == selector.size());

    static property_sheet::DoMatch<> do_match{};
	Score score(columns);
	for (size_t i = 0; i < columns; ++i) {
        score[i] = do_match(selector[i], fields[i]);
        if (score[i] == 0)
            return std::nullopt;
	}
    return score;
}

struct MatchedProperty {
    const PropertySheet::Property* prop;
    PropertySheet::Score score;
};

using MatchedProperties = std::map<std::string_view,
                                  MatchedProperty>;

static void acceptProps(PropertySheet::Entry const& e,
                        PropertySheet::Score const& score,
                        MatchedProperties& out)
{
    for (auto& prop : e.props) {
        auto old = out.find(prop.name);
        if (old != out.end()) {
            auto& mp = old->second;
            if (mp.score <= score) {
                mp.prop = &prop;
                mp.score = score;
            }
        } else {
            out.emplace(prop.name, MatchedProperty{&prop, score});
        }
    }
}

// Stupid O(N) algo for now..
PropertySheet::PropSet
PropertySheet::query(const std::string_view* begin,
                     const std::string_view* end) const
{
    RUNOS_ASSERT((size_t) std::distance(begin, end) == columns);

    using namespace ranges;

    MatchedProperties results;
    for (auto& e: entries) {
        if (auto score = match(e.selector, begin, end)) {
            acceptProps(e, *score, results);
        }
    }

    return results | view::values
                   | view::transform([](auto& mp) { return *mp.prop; });
}

} // runos
