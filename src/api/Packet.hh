/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include <exception>
#include <type_traits>

#include <boost/assert.hpp>

#include "oxm/field.hh"

namespace runos {

// TODO: push: outport, vlan, pbb, virtual field
class Packet {
public:
    // throws std::out_of_range if field doesn't exists
    virtual oxm::field<> load(oxm::mask<> mask) const = 0;
    // TODO: don't throw, return false

    virtual bool test(oxm::field<> need) const
    { return load(oxm::mask<>(need)) & need; }

    virtual void modify(oxm::field<> patch) = 0;

    virtual ~Packet() noexcept = default;

    ////////////////////////
    //  Helper functions  //
    ////////////////////////

    // CONCEPT: ethaddr a = pkt.load(eth_src);
    //    eth_src (type) is implicitly convertible to oxm::mask<eth_src>
    //      with all bits set to 1
    //    oxm::field<> convertible to oxm::field<Type>
    //      with runtime check of size
    //    oxm::field<Type> convertible to Type::value_type
    //      with runtime check of mask (should be exact match)
    // CONCEPT: oxm::field<ethaddr> m = pkt.load(eth_src & "ff:ff:00:00:00:00");
    //      oxm::mask<eth_src_meta> operator& (eth_src_meta, eth_src_meta::mask_type)
    
    template<class Type>
    oxm::value<Type> load(Type type) const
    {
        auto field = load(oxm::mask<Type>{type});
        BOOST_ASSERT(field.exact());
        return oxm::value<Type>(field);
    }

    template<class Type>
    oxm::field<Type> load(oxm::mask<Type> mask) const
    {
        auto generic_mask = static_cast<oxm::mask<>>(mask);
        auto generic_ret = load(generic_mask);
        return static_cast<oxm::field<Type>>(generic_ret);
    }

    // CONCEPT: pkt.test(eth_src == "aa:bb:cc:dd:ee:ff")
    // CONCEPT: pkt.test((eth_src & "ff:ff:00:ff:ff:ff") == "aa:bb:00:dd:ee:ff")
    //      eth_src_meta & eth_src_meta::mask_type -> oxm::mask<eth_src_meta>
    //      oxm::mask<eth_src_meta> == eth_src_meta::value_type -> field
    template<typename Type>
    bool test(oxm::field<Type> need) const
    {
        return test(oxm::field<>(need));
    }

    // modify helpers
    // CONCEPT: pkt.modify(eth_src >> "aa:bb:cc:dd:ee:ff")
    //      eth_src_meta >> eth_src_meta::value_type -> oxm::value
    //      oxm::value implicitly convertible to oxm::field
    // CONCEPT: pkt.modify((eth_src & "ff:ff:00:ff:ff:ff") >> "aa:bb:00:dd:ee:ff")
    //      oxm::mask<T> >> T::value_type -> oxm::field
    template<typename Type>
    void modify(oxm::field<Type> patch)
    {
        modify(oxm::field<>(patch));
    }

    template<class T>
    friend typename std::enable_if<std::is_pointer<T>::value, T>::type
    packet_cast(Packet& pkt) noexcept;

    template<class T>
    friend typename std::enable_if<std::is_lvalue_reference<T>::value, T>::type
    packet_cast(Packet& pkt);

protected:
    virtual Packet& next_wrapper() noexcept
    { return *this; }

    virtual const Packet& next_wrapper() const noexcept
    { return *this; }
};

/////////////////////
//   Packet cast   //
/////////////////////

struct bad_packet_cast : std::exception { };

template<class T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
packet_cast(Packet& pkt) noexcept
{
    if (T try_this = dynamic_cast<T>(&pkt))
        return try_this;

    if (&pkt.next_wrapper() == &pkt)
        return nullptr;

    return packet_cast<T>(pkt.next_wrapper());
}

template<class T>
typename std::enable_if<std::is_lvalue_reference<T>::value, T>::type
packet_cast(Packet& pkt)
{
    using Tptr =
        typename std::remove_reference<T>::type*;

    if (Tptr ret = packet_cast<Tptr>(pkt))
        return *ret;
    else
        throw bad_packet_cast();
}

////////////////////
//    Proxy       //
////////////////////

class PacketProxy : public Packet {
protected:
    Packet& pkt;

    Packet& next_wrapper() noexcept override
    { return pkt; }

    const Packet& next_wrapper() const noexcept override
    { return pkt; }

public:
    PacketProxy(Packet& pkt)
        : pkt(pkt)
    { }

    oxm::field<> load(oxm::mask<> mask) const override
    { return pkt.load(mask); }

    bool test(oxm::field<> need) const override
    { return pkt.test(need); }

    void modify(oxm::field<> patch) override
    { pkt.modify(patch); }
};

} // namespace runos
