#pragma once

#include <initializer_list>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <ostream>

#include "field_set_fwd.hh"
#include "field.hh"
#include "api/Packet.hh"

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

    bool empty() const { return entries.empty(); }

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


namespace expirementer{
// expirementer block
// needed to proof correct and to cover unit tests

// lhs included in rhs
static bool operator < (oxm::field<> lhs, oxm::field<> rhs)
{
    auto rhs_bits = rhs.mask_bits();
    return (lhs.mask_bits() & rhs_bits) == rhs_bits && lhs & rhs;
}

// This field set cat contain many field with the same types
struct multi_field_set
{
private:
    using Container = std::unordered_multimap< oxm::type, oxm::field<>>;
    Container elements;
    std::unordered_set<oxm::type> used_types;
    typedef typename std::unordered_set<oxm::type>::iterator types_iterator;
    typedef typename std::unordered_set<oxm::type>::const_iterator const_types_iterator;
public:

    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    explicit multi_field_set() = default;

    explicit multi_field_set(std::initializer_list<field<>> content)
    {
        for (auto& f : content){
            add(f);
        }
    }

    void add(field<> f)
    {
        oxm::type t = f.type();
        auto its = elements.equal_range(t);
        for (auto it = its.first; it != its.second; )
        {
            if ( f < it->second) return; // return if our set cover this field
            if ( it->second < f){
                it = elements.erase(it); // delete element if new fileld cover it
            } else {
                it++;
            }
        }
        elements.insert({t, f});
        used_types.insert(t);
    }

    void erase(mask<> m)
    {
        auto t = m.type();
        auto its = elements.equal_range(t);
        for (auto it = its.first; it != its.second; /*iterate in body*/)
        {
            it->second = it->second & ~m;
            if (it->second.wildcard()){
                it = elements.erase(it);
            } else {
                it++;
            }
        }
    }

    std::vector<oxm::field_set> fields() const
    {
        std::vector<oxm::field_set> ret;
        auto type_it_end = used_types.end();
        std::function<void(const_types_iterator,
                           oxm::field_set)> recursive =
            [&](const_types_iterator type_it,
                                        oxm::field_set fs) -> void {

            if (type_it == type_it_end){
                ret.push_back(fs);
                return;
            }
            auto tmp_type_it = type_it;
            auto its = elements.equal_range(*type_it);
            for (auto it = its.first; it != its.second; it++){
                fs.modify(it->second);
                recursive(++tmp_type_it , fs);
                tmp_type_it = type_it;
                fs.erase(oxm::mask<>(it->second));
            }
            if (its.first == its.second){
                recursive(++tmp_type_it, fs);
            }
        };

        recursive(used_types.begin(), {});
        return ret;
    }


    std::pair<iterator, iterator> equal_range(oxm::type t)
    { return elements.equal_range(t); }
    std::pair<const_iterator, const_iterator> equal_range(oxm::type t) const
    { return elements.equal_range(t); }

    iterator begin()
    { return elements.begin(); }
    const_iterator begin() const
    { return elements.begin(); }
    const_iterator cbegin() const
    { return elements.cbegin(); }

    iterator end()
    { return elements.end(); }
    const_iterator end() const
    { return elements.end(); }
    const_iterator cend() const
    { return elements.cend(); }
};

// contain included and excluded fields
class full_field_set{
    multi_field_set _included;
    multi_field_set _excluded;

public:
    explicit full_field_set() = default;
    explicit full_field_set(std::initializer_list<field<>> include)
        : _included(include)
    { }
    explicit full_field_set(std::initializer_list<field<>> include,
                            std::initializer_list<field<>> exclude)
        : _included(include), _excluded(exclude)
    { }
    multi_field_set& included() {return _included;}
    const multi_field_set& included () const {return _included;}
    multi_field_set& excluded() {return _excluded;}
    const multi_field_set& excluded () const {return _excluded;}

    void add(oxm::field<> f) {_included.add(f);}
    void erase(oxm::mask<> m) {_included.erase(m);}
    void exclude(oxm::field<> f) {_excluded.add(f);}
    void include(oxm::mask<> m) {_excluded.erase(m);}
};

} // namespace expirementer


} // namespace oxm
} // namespace runos
