#include "exception.hh"

#include <boost/exception/diagnostic_information.hpp>

namespace runos {

struct assertion_failure : virtual logic_error
{ };

typedef boost::error_info< struct tag_assert_expr, const char* >
    assertion_expr;
typedef boost::error_info< struct tag_assert_msg, const char* >
    assertion_msg;

const char* exception::what() const noexcept
{
    return boost::diagnostic_information_what( *this );
}

}

namespace boost {

void assertion_failed(char const * expr, char const * function, char const * file, long line)
{
    throw_exception(
            runos::assertion_failure() <<
            runos::assertion_expr(expr) <<
            throw_function(function) <<
            throw_file(file) <<
            throw_line(line)
    );
}

void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line)
{
    throw_exception(
            runos::assertion_failure() <<
            runos::assertion_expr(expr) <<
            runos::assertion_msg(msg) <<
            throw_function(function) <<
            throw_file(file) <<
            throw_line(line)
    );
}

}
