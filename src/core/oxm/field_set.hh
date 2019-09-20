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

#include <initializer_list>
#include <unordered_set>
#include <utility>
#include <ostream>

#include "field_set_fwd.hh"
#include "field.hh"
#include "../api/Packet.hpp"

namespace runos {
namespace oxm {

class field_set : public Packet {
    // stores only non-wildcarded fields
    // all other fields implies to wildcard
    struct HashByType {
        size_t operator()(const field<> f) const {
            return std::hash<type>()(f.type());
        }
    };

    struct EqualByType {
        bool operator()(const field<>& lhs, const field<>& rhs) const
        { return lhs.type() == rhs.type(); }
    };

    using Container = std::unordered_set< field<>, HashByType, EqualByType >;
    Container entries;

public:
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    field_set() = default;

    // types of all fields should be different.
    // otherwise behaviour is undefined.
    field_set(std::initializer_list<field<>> content)
        : entries(content)
    { }

    field<> load(mask<> mask) const override
    {
        auto t = mask.type();
        auto it = entries.find(field<>{t});
        if (it == entries.end())
            return field<>{t} & mask;
        return *it & mask;
    }

    // modifiers
    void modify(field<> patch) override
    {
        auto t = patch.type();
        auto it = entries.find(field<>{t});
        if (it == entries.end())
            it = entries.emplace(patch).first;

        // it's safe because it->type() stay unchanged
        const_cast<field<>&>(*it) = *it >> patch;
    }

    void erase(mask<> mask)
    {
        auto t = mask.type();
        auto it = entries.find(field<>{t});
        if (it == entries.end())
            return;

        // it's safe because it->type() stay unchanged
        const_cast<field<>&>(*it) = *it & ~mask;
        if (it->wildcard())
            entries.erase(it);
    }

    void clear()
    {
        entries.clear();
    }
    
    // iterators
    iterator begin()
    { return entries.begin(); }
    const_iterator begin() const
    { return entries.begin(); }
    const_iterator cbegin() const
    { return entries.cbegin(); }

    iterator end()
    { return entries.end(); }
    const_iterator end() const
    { return entries.end(); }
    const_iterator cend() const
    { return entries.cend(); }

    friend bool operator==(const field_set& lhs, const field_set& rhs)
    { return lhs.entries == rhs.entries; }

    friend bool operator&(const field_set& lhs, const Packet& pkt)
    {
        return std::all_of(lhs.begin(), lhs.end(), [&pkt](const field<>& f){
            return pkt.test(f);
        });
    }

    friend bool operator&(const Packet& pkt, const field_set& lhs)
    { return lhs & pkt; }

    friend std::ostream& operator<<(std::ostream& out, const field_set& fs)
    {
        // use std::ostream_joiner when it will be available
        bool delim = false;
        for (const field<>& f : fs) {
            out << (delim ? " && " : "") << f;
            delim = true;
        }
        return out;
    }
};

} // namespace oxm
} // namespace runos
