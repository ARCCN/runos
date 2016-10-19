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

#define BOOST_TEST_MODULE OXM query expressions testcases

#include <boost/test/unit_test.hpp>
#include <boost/proto/literal.hpp>

#include "oxm/query.hh"
#include "oxm/openflow_basic.hh"
#include "types/ethaddr.hh"

namespace proto = boost::proto;
using namespace runos::oxm;
using runos::ethaddr;

BOOST_AUTO_TEST_SUITE( runos_oxm_tests )

struct Fixture {
    proto::literal<in_port_meta> in_port;
    proto::literal<eth_src_meta> eth_src;
    proto::literal<eth_dst_meta> eth_dst;
    proto::literal<int>          int_;
};

#define CHECK_MATCH(expr, grammar) \
{ BOOST_CHECK( (proto::matches< decltype( expr ), grammar >()) ); }

#define CHECK_NOT_MATCH(expr, grammar) \
{ BOOST_CHECK( not (proto::matches< decltype( expr ), grammar >()) ); }

BOOST_FIXTURE_TEST_CASE( lvalue_grammar_test, Fixture) {
    CHECK_NOT_MATCH( int_, lvalue_grammar );
    CHECK_MATCH( in_port, lvalue_grammar );
    CHECK_NOT_MATCH( in_port & 10, lvalue_grammar );
    CHECK_NOT_MATCH( in_port & (uint32_t) 10, lvalue_grammar );
    CHECK_MATCH( eth_src & ethaddr(0x1234), lvalue_grammar );
    CHECK_NOT_MATCH( eth_src & 123, lvalue_grammar );
    CHECK_NOT_MATCH( eth_src & std::string("11:22:33:44:55:66"), lvalue_grammar );
    CHECK_NOT_MATCH( eth_src & eth_src, lvalue_grammar );
    // CHECK_NOT_MATCH( eth_src & "11:22:33:44:55:66", lvalue_grammar ); FIXME: compile error
}

BOOST_FIXTURE_TEST_CASE( predicate_grammar_test, Fixture) {
    CHECK_MATCH( in_port == 3, predicate_grammar );
    CHECK_NOT_MATCH( (in_port & 1) == (uint32_t) 2, predicate_grammar );
    CHECK_MATCH( (eth_src & ethaddr()) == ethaddr(), predicate_grammar );
    CHECK_MATCH( (eth_dst & ethaddr()) != ethaddr(), predicate_grammar );
    CHECK_NOT_MATCH( (eth_dst & ethaddr()) != 123, predicate_grammar );
    CHECK_MATCH( (eth_dst & ethaddr()) != ethaddr() && in_port == 3 ||
                 ! ((eth_src & ethaddr()) != ethaddr()), predicate_grammar );
    CHECK_NOT_MATCH( eth_src == eth_dst , predicate_grammar); // Allow this in future?
    CHECK_NOT_MATCH( in_port, predicate_grammar );
    CHECK_MATCH( in_port == (3 + 3), predicate_grammar );
    CHECK_NOT_MATCH( in_port || eth_src, predicate_grammar );
}

BOOST_FIXTURE_TEST_CASE( query_grammar_test, Fixture) {
    CHECK_MATCH( in_port, query_grammar );
    CHECK_MATCH( in_port == 3, query_grammar );
}

BOOST_AUTO_TEST_SUITE_END()
