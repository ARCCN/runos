#pragma once

#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/throw_exception.hpp>

namespace runos {

typedef boost::error_info< struct tag_error_msg, const char* >
    errinfo_msg;

#define RUNOS_THROW(x) BOOST_THROW_EXCEPTION(x)

struct exception : virtual boost::exception
                 , virtual std::exception
{
    const char* what() const noexcept override;
};

struct logic_error : virtual exception
{ };

struct invalid_argument : virtual logic_error
{ };

struct domain_error : virtual logic_error
{ };

struct length_error : virtual logic_error
{ };

struct out_of_range : virtual logic_error
{ };

struct runtime_error : virtual exception
{ };

struct range_error : virtual runtime_error
{ };

struct overflow_error : virtual runtime_error
{ };

struct underflow_error : virtual runtime_error
{ };

}
