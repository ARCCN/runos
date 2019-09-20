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

#include <optional>
#include <utility>

namespace runos {

namespace detail {
template<class Tag, class T>
struct kwarg_box { T value; };
}

template<class Tag, class T>
struct kwarg {
    using type = T;
    using tag = Tag;

    constexpr kwarg() noexcept {}

    detail::kwarg_box<Tag, T> operator=(T val) const
    {
        return {std::move(val)};
    }
};

namespace detail {
template<class KwArg>
struct packed_kwarg
{
    using kwarg_type = KwArg;
    using value_type = typename KwArg::type;
    using tag = typename KwArg::tag;

    std::optional<value_type> value;

    packed_kwarg() = default;

    explicit packed_kwarg(kwarg_box<tag, value_type> box)
        : value{ std::move(box.value) }
    { }
};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif

template<class ...KwArgs>
struct packed_kwargs : private packed_kwarg<KwArgs>...
{
    template<class ...Tag, class ...T>
    packed_kwargs(kwarg_box<Tag, T> ...boxes)
        : packed_kwarg< kwarg<Tag,T> >{ std::move(boxes) }...
    { }

    template<class Tag, class T>
    auto get(kwarg<Tag, T>) -> std::optional<T> &
    {
        return packed_kwarg<kwarg<Tag, T>>::value;
    }

    template<class Tag, class T>
    auto set(kwarg<Tag, T>, T val) -> void
    {
        packed_kwarg<kwarg<Tag, T>>::value = val;
    }
};
} // detail

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

template<class ...Args>
auto kwargs(Args... args)
    -> detail::packed_kwargs<decltype(args)...>;

} // namespace runos
