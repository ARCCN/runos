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

#include "query_impl.hh"

namespace runos {
namespace oxm {

struct lvalue_grammar
    : impl::lvalue_grammar
{ };

struct predicate_grammar
    : impl::predicate_grammar
{ };

struct query_grammar
    : impl::query_grammar
{ };

BOOST_PROTO_DEFINE_OPERATORS(impl::is_terminal, boost::proto::default_domain);

// TODO: evaluate on oxm set

} // namespace oxm
} // namespace runos
