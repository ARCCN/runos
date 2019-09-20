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

#include "meta.hpp"

namespace traits {

template <class F> struct function_traits;

template <class R, class... Args>
struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> { };

template <class C, class R>
struct function_traits<R(C::*)> : function_traits<R(C&)> { };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : function_traits<R(C&, Args...)> { };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const volatile> :
  function_traits<R(C volatile const&, Args...)>
{ };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) volatile> :
  function_traits<R(C volatile&, Args...)>
{ };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> :
  function_traits<R(C const&, Args...)>
{ };

template <class R, class... Args>
struct function_traits<R(Args...)> {
  using typelist = core::v2::meta::list<Args...>;
  using return_type = R;

  using pointer = std::add_pointer_t<return_type(Args...)>;
  static constexpr auto arity = typelist::size();

  template <::std::size_t N> using argument = core::v2::meta::get<typelist, N>;
};

template <class F> struct function_traits {
  using functor_type = function_traits<decltype(&std::decay_t<F>::operator())>;
  using return_type = typename functor_type::return_type;
  using pointer = typename functor_type::pointer;
  static constexpr auto arity = functor_type::arity - 1;
  template <::std::size_t N>
  using argument = typename functor_type::template argument<N>;
};

}

