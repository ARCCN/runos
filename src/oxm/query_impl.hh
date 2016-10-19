#pragma once

#include <type_traits>
#include <boost/proto/proto.hpp>

#include "types/ethaddr.hh"
#include "field_meta.hh"

namespace runos {
namespace oxm {
namespace impl {

using namespace boost;
using proto::_;

template<typename T>
struct is_terminal
    : boost::mpl::false_
{};

template<>
struct is_terminal<ethaddr>
    : boost::mpl::true_
{};

// mpl predicates for type checking
template<typename FieldMeta, typename Mask, typename Enable = void>
struct check_mask_type
    : mpl::false_
{};
template<typename FieldMeta, typename Mask>
struct check_mask_type<FieldMeta, Mask,
           typename std::enable_if<
               std::is_convertible< Mask,
                   typename FieldMeta::mask_type
               >::value
           >::type
       >
    : mpl::true_
{};

template<typename FieldMeta, typename Value, typename Enable = void>
struct check_value_type
    : mpl::false_
{};
template<typename FieldMeta, typename Value>
struct check_value_type<FieldMeta, Value,
           typename std::enable_if<
               std::is_convertible< Value,
                   typename FieldMeta::value_type
               >::value
           >::type
       >
    : mpl::true_
{};

//template<typename FieldMeta, typename _FieldMeta = FieldMeta>
//struct is_field_type : mpl::false_
//{};
//template<typename FieldMeta>
//struct is_field_type< FieldMeta,
//        typename FieldMeta::field_type > 
//    : mpl::true_
//{};
//

struct field_terminal
    : proto::terminal< proto::convertible_to< const field_meta& > >
{ };

// A grammar that matches all lvalue oxm queries
struct lvalue_grammar
    : proto::or_<
          field_terminal
        , proto::and_<
              proto::bitwise_and<
                  field_terminal
                , proto::terminal< _ >
              >
            , proto::if_< check_mask_type<
                  proto::_value(proto::_left),
                  proto::_value(proto::_right)
              >() >
          >
      >
{ };

struct extract_field_meta
    : proto::or_<
          proto::when<
              field_terminal
            , proto::_value
          >
        , proto::when<
              proto::bitwise_and< _, _ >
            , extract_field_meta(proto::_left)
          >
      >
{ };

struct predicate_grammar
    : proto::or_<
          proto::logical_or< predicate_grammar, predicate_grammar >
        , proto::logical_and< predicate_grammar, predicate_grammar >
        , proto::logical_not< predicate_grammar >
        , proto::and_<
              proto::or_<
                  proto::equal_to< lvalue_grammar, proto::terminal<_> >
                , proto::not_equal_to< lvalue_grammar, proto::terminal<_> >
              >
            , proto::if_< check_value_type<
                  extract_field_meta(proto::_left),
                  proto::_value(proto::_right)
              >() >
          >
    >
{ };

struct query_grammar
    : proto::or_<
          predicate_grammar,
          lvalue_grammar
      >
{ };

}
}
}
