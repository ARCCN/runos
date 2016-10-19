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
