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

#define BOOST_TEST_MODULE ethaddr tests

#include <unordered_set>
#include <sstream>
#include <array>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "types/ethaddr.hh"

using runos::ethaddr;
using bytes_type = ethaddr::bytes_type;

BOOST_AUTO_TEST_SUITE( runos_types_tests )

BOOST_AUTO_TEST_CASE( constructors_test ) {
    BOOST_CHECK_EQUAL(ethaddr(), ethaddr("00:00:00:00:00:00"));
    //BOOST_CHECK_EQUAL(ethaddr(), ethaddr(0ULL));
    BOOST_CHECK_EQUAL(
            ethaddr("11:22:33:44:55:66"),
            ethaddr("11-22-33-44-55-66"));
    //BOOST_CHECK_EQUAL(
    //        ethaddr("\t 11:22:33:44:55:66 "),
    //        ethaddr("  11-22-33-44-55-66\t\t"));
    BOOST_CHECK_EQUAL(
            ethaddr("aa:bb:cc:dd:ee:ff"),
            ethaddr("AA:BB:CC:DD:EE:FF"));
    BOOST_CHECK_EQUAL(
            ethaddr("aa:bb:cc:dd:ee:ff"),
            ethaddr("AABB.CCDD.EEFF"));

    BOOST_CHECK_EQUAL(ethaddr("11:22:33:44:55:66"),
            ethaddr(bytes_type{{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}}));

    BOOST_CHECK_EQUAL(
            ethaddr("11:aa:22:bb:33:cc"),
            ethaddr(0x11aa22bb33ccUL));
}

BOOST_AUTO_TEST_CASE( error_handling_test ) {
    BOOST_CHECK_THROW(ethaddr(""), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("11-22:33:44:55:66"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("11-22-33-44-55"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("11 :22:33:44:55:66"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("gg:11:22:33:44:55:66"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("abacaba"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("     "), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("11:22:33:44:55:66:77"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr("aaaaa.bbbb.cccc"), ethaddr::bad_representation);
    BOOST_CHECK_THROW(ethaddr(0xffff11aa22bb33ccUL), ethaddr::bad_representation);
}

BOOST_AUTO_TEST_CASE( operators_test ) {
    BOOST_CHECK_NE(ethaddr(), ethaddr("11-11-11-11-11-11"));

    // check for hash
    std::unordered_set<ethaddr> set;
    set.insert(ethaddr());
    set.emplace("00-11-22-33-44-55");
    BOOST_CHECK_EQUAL(set.size(), 2);

    ethaddr a = ethaddr("11:22:33:44:55:66");
    ethaddr b = ethaddr("aa:bb:cc:dd:ee:ff");
    ethaddr c(a);
    BOOST_CHECK_NE(a, b);
    BOOST_CHECK_EQUAL(a, c);
    a = b;
    BOOST_CHECK_EQUAL(a, b);
}

BOOST_AUTO_TEST_CASE( properties_test ) {
    ethaddr broadcast("ff:ff:ff:ff:ff:ff");
    ethaddr multicast("01:80:c2:00:00:0e");
    ethaddr unique("3c:07:54:05:33:49");
    ethaddr spoofed("06-00-00-00-00-00");

    BOOST_CHECK(is_broadcast(broadcast));
    BOOST_CHECK(!is_broadcast(multicast));
    BOOST_CHECK(!is_broadcast(unique));
    BOOST_CHECK(!is_broadcast(spoofed));

    BOOST_CHECK(!is_unicast(broadcast));
    BOOST_CHECK(!is_unicast(multicast));
    BOOST_CHECK(is_unicast(unique));
    BOOST_CHECK(is_unicast(spoofed));

    BOOST_CHECK(is_multicast(broadcast));
    BOOST_CHECK(is_multicast(multicast));
    BOOST_CHECK(!is_multicast(unique));
    BOOST_CHECK(!is_multicast(spoofed));

    BOOST_CHECK(!unique.is_locally_administered());
    BOOST_CHECK(spoofed.is_locally_administered());
}

BOOST_AUTO_TEST_CASE( stringstream_cast_test ) {
    ethaddr a(0xaabbccddeeff);
    std::ostringstream oss;
    oss << a;
    BOOST_CHECK_EQUAL(oss.str(), "aa:bb:cc:dd:ee:ff");

    std::istringstream iss("   11:22:33:44:55:66    aa:bb@cc:dd:ee:ff ");
    iss >> a;
    BOOST_CHECK_EQUAL(a, ethaddr(0x112233445566));
    iss >> a;
    BOOST_CHECK(iss.fail());

    BOOST_CHECK_EQUAL(
            boost::lexical_cast<std::string>(ethaddr(0xaabbccddeeff)),
            "aa:bb:cc:dd:ee:ff");
    BOOST_CHECK_EQUAL(
            boost::lexical_cast<ethaddr>("aa:bb:cc:dd:ee:ff"),
            ethaddr(0xaabbccddeeff));
}

BOOST_AUTO_TEST_SUITE_END()
