#pragma once

#include <stdexcept>
#include <boost/exception/error_info.hpp>

#include "type.hh"
#include "types/exception.hh"

namespace runos {
namespace oxm {

typedef boost::error_info< struct tag_type, type >
    errinfo_type;
struct field_error : virtual boost::exception
                   , virtual std::exception
{ };

typedef boost::error_info< struct tag_actual_type, type >
    errinfo_actual_type;
typedef boost::error_info< struct tag_requested_type, type >
    errinfo_requested_type;

struct bad_cast: virtual field_error
{ };

typedef boost::error_info< struct tag_lhs_type, type >
    errinfo_lhs_type;
typedef boost::error_info< struct tag_rhs_type, type >
    errinfo_rhs_type;

struct bad_operands: virtual field_error
                   , virtual invalid_argument
{ };

typedef boost::error_info< struct tag_actual_bit_length, size_t >
    errinfo_actual_bit_length;
typedef boost::error_info< struct tag_good_bit_length, size_t >
    errinfo_good_bit_length;

struct bad_bit_length: virtual field_error
                     , virtual invalid_argument
{ };

struct bad_mask: virtual field_error 
               , virtual range_error
{ };

} // namespace oxm
} // namespace runos
