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

#include <utility>
#include <boost/variant/static_visitor.hpp>

namespace runos {

template <typename ReturnType, typename... Lambdas>
struct lambda_visitor;

template <typename ReturnType, typename Lambda1, typename... Lambdas>
struct lambda_visitor< ReturnType, Lambda1 , Lambdas... > 
  : lambda_visitor<ReturnType, Lambdas...>, Lambda1 {

    using Lambda1::operator();
    using lambda_visitor< ReturnType , Lambdas...>::operator();
    lambda_visitor(Lambda1 l1, Lambdas&&... lambdas) 
      : lambda_visitor< ReturnType , Lambdas...> (std::move(lambdas)...)
      , Lambda1(std::move(l1))
    {}
};


template <typename ReturnType, typename Lambda1>
struct lambda_visitor<ReturnType, Lambda1> 
  : virtual boost::static_visitor<ReturnType>, Lambda1 {

    using Lambda1::operator();
    lambda_visitor(Lambda1&& l1) 
      : boost::static_visitor<ReturnType>(), Lambda1(std::move(l1))
    {}
};


template <typename ReturnType>
struct lambda_visitor<ReturnType> 
  : virtual boost::static_visitor<ReturnType> {

    lambda_visitor() : boost::static_visitor<ReturnType>() {}
};

} // namespace runso
