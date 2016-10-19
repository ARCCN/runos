#define BOOST_TEST_MODULE Dynamic-typed OXM field operations testcases

#include <boost/test/unit_test.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

#include "types/bits.hh"
#include "oxm/field.hh"
#include "oxm/openflow_basic.hh"
#include "oxm/errors.hh"

using namespace runos;

struct Fixture {
    oxm::in_port  in_port;
    oxm::eth_src  eth_src;
    oxm::eth_dst  eth_dst;
    oxm::eth_type eth_type;
};

BOOST_FIXTURE_TEST_SUITE( runos_oxm_tests , Fixture )

BOOST_AUTO_TEST_CASE( binop_type_test ) {
    oxm::field<oxm::in_port> f1;
    oxm::field<oxm::eth_src> f2;
    oxm::field<> f3 = oxm::field<>(in_port);
    oxm::value<oxm::in_port> v1(10);
    oxm::value<oxm::eth_src> v2("aa:bb:cc:dd:ee:ff");
    oxm::value<> v3 = oxm::value<oxm::in_port>(20);
    oxm::mask<oxm::in_port> m1;
    oxm::mask<oxm::eth_src> m2;
    oxm::mask<> m3 = oxm::mask<oxm::in_port>(0);

    static_assert(
        std::is_same<
            decltype(oxm::detail::binop_type(f1, f1)),
            oxm::in_port
        >(), "test failed"
    );

    static_assert(
        std::is_same<
            decltype(oxm::detail::binop_type(f1, v1)),
            oxm::in_port
        >(), "test failed"
    );

    static_assert(
        std::is_same<
            decltype(oxm::detail::binop_type(v1, m3)),
            oxm::in_port
        >(), "test failed"
    );

    static_assert(
        std::is_same<
            decltype(oxm::detail::binop_type(f3, v1)),
            oxm::in_port
        >(), "test failed"
    );

    static_assert(
        not std::is_same<
            decltype(oxm::detail::binop_type(f3, m2)),
            oxm::in_port
        >(), "test failed"
    );

    static_assert(
        std::is_same<
            decltype(oxm::detail::binop_type(f3, v3)),
            oxm::type
        >(), "test failed"
    );
}
    
BOOST_AUTO_TEST_CASE( meta_test ) {
    BOOST_CHECK_EQUAL(in_port.nbits(), 32);
    BOOST_CHECK_EQUAL(in_port.nbytes(), 4);
    BOOST_CHECK(not in_port.maskable());

    BOOST_CHECK_EQUAL(eth_src.nbits(), 48);
    BOOST_CHECK_EQUAL(eth_src.nbytes(), 6);
    BOOST_CHECK(eth_src.maskable());

    BOOST_CHECK_EQUAL(eth_dst.nbits(), 48);
    BOOST_CHECK_EQUAL(eth_dst.nbytes(), 6);
    BOOST_CHECK(eth_dst.maskable());

    BOOST_CHECK_EQUAL(eth_type.nbits(), 16);
    BOOST_CHECK_EQUAL(eth_type.nbytes(), 2);
    BOOST_CHECK(not eth_type.maskable());
}

BOOST_AUTO_TEST_CASE( field_ctor_test ) {
    ////
    BOOST_TEST_MESSAGE( "Constructing some oxm::value`s" );
    ////
    oxm::value<oxm::in_port> v_in_port_10 { 10 };
    oxm::value<oxm::eth_src> v_eth_src_aabb { "aabb.ccdd.eeff" };
    oxm::value<oxm::in_port> v_copy_of_in_port_10 ( v_in_port_10 );
    oxm::value<oxm::eth_src> v_eth_src_aabb_ ( 0xaabbccddeeff );
    oxm::value<oxm::eth_dst> v_eth_dst_aabb =
        eth_dst == 0xaabbccddeeff;
    oxm::value<oxm::eth_src> v_eth_src_ddee =
        eth_src == "dd:ee:11:22:33:44";

    BOOST_CHECK_EQUAL(v_in_port_10, v_copy_of_in_port_10);
    BOOST_CHECK_EQUAL(v_eth_src_aabb, v_eth_src_aabb_);
    BOOST_CHECK_NE(v_eth_src_aabb, v_eth_dst_aabb);
    BOOST_CHECK_NE(v_eth_src_aabb, v_eth_src_ddee);
    
    BOOST_CHECK_THROW( oxm::value<>(in_port, bits<4>("1100")),
            oxm::bad_bit_length);

    ////
    BOOST_TEST_MESSAGE( "Constructing some oxm::mask`s" );
    ////
    oxm::mask<> m_in_port_wildcard ( in_port,
            bits<>(std::string("00000000000000000000000000000000")) );
    BOOST_CHECK( m_in_port_wildcard.wildcard() );
    BOOST_CHECK_EQUAL( m_in_port_wildcard, ~oxm::mask<oxm::in_port>() );

    oxm::mask<> m_in_port_exact ( in_port,
            bits<>(std::string("11111111111111111111111111111111")) );
    BOOST_CHECK( m_in_port_exact.exact() );

    BOOST_CHECK_THROW( oxm::mask<>( in_port, bits<>(std::string("00")) ),
            oxm::bad_bit_length );
    BOOST_CHECK_THROW( oxm::mask<>( in_port, bits<>(std::string("00000001")) ),
            oxm::bad_bit_length );

    oxm::mask<oxm::eth_src> m_eth_src_1 (eth_src, ethaddr("ff:00:00:00:00:00"));
    oxm::mask<oxm::eth_src> m_eth_src_12 {ethaddr("0f:f0:00:00:00:00")};
    oxm::mask<oxm::eth_src> m_eth_src_2 = eth_src & ethaddr("00:ff:00:00:00:00");

    oxm::mask<> m_eth_dst_3 
        = oxm::mask<oxm::eth_dst>(ethaddr("00:00:ff:00:00:00"));
    oxm::mask<oxm::eth_dst> m_eth_dst_34(ethaddr("00:00:0f:f0:00:00"));
    oxm::mask<oxm::eth_dst> m_eth_dst_4(ethaddr("00:00:00:ff:00:00"));

    oxm::mask<> m_copy_of_eth_src_1 ( m_eth_src_1 );

    BOOST_CHECK( m_eth_dst_3.fuzzy() );
    BOOST_CHECK( m_eth_dst_34.fuzzy() );
    BOOST_CHECK( m_eth_dst_4.fuzzy() );
    BOOST_CHECK_EQUAL( m_eth_src_1, m_copy_of_eth_src_1 );
    
    ////
    BOOST_TEST_MESSAGE( "Constructing some oxm::field`s" );
    ////
    oxm::field<oxm::eth_src> f_eth_src_1 (ethaddr("aa:bb:cc:dd:ee:ff"),
                                          ethaddr("00:ff:00:ff:00:ff"));
    oxm::field<> f_eth_dst_1 (eth_dst, ethaddr("aa:bb:cc:dd:ee:ff").to_bits(),
                                       ethaddr("00:ff:00:ff:00:ff").to_bits());
    oxm::field<> f_copy_of_eth_src_1 (f_eth_src_1);
    oxm::field<oxm::eth_src> f_exact_1 = eth_src == ethaddr("aa:bb:cc:dd:ee:ff");
    oxm::field<oxm::eth_src> f_exact_2 
        = (eth_src & ethaddr("ff:ff:ff:ff:ff:ff")) == ethaddr("aa:bb:cc:dd:ee:ff");

    BOOST_CHECK_EQUAL( oxm::value<oxm::eth_src>(f_eth_src_1),
                       oxm::value<oxm::eth_src>(ethaddr("00:bb:00:dd:00:ff")));
    BOOST_CHECK_EQUAL( oxm::mask<oxm::eth_src>(f_eth_src_1),
                       oxm::mask<oxm::eth_src>(ethaddr("00:ff:00:ff:00:ff")));
    BOOST_CHECK_EQUAL( f_eth_src_1.value_bits(), f_eth_dst_1.value_bits() );
    BOOST_CHECK_EQUAL( f_eth_src_1.mask_bits(), f_eth_dst_1.mask_bits() );
    BOOST_CHECK_NE( f_eth_src_1, f_eth_dst_1 );
    BOOST_CHECK_EQUAL( f_eth_src_1, f_copy_of_eth_src_1 );
    BOOST_CHECK_THROW( oxm::field<oxm::eth_type>(0x88cc, 3), oxm::bad_mask );
    BOOST_CHECK_EQUAL( f_exact_1, f_exact_2 );
    BOOST_CHECK( f_exact_1.exact() );

    ////
    BOOST_TEST_MESSAGE( "Casting oxm fields" );
    ////
    BOOST_CHECK_EQUAL( oxm::mask<>(f_eth_src_1),
                       oxm::mask<oxm::eth_src>( ethaddr("00:ff:00:ff:00:ff")) );
    BOOST_CHECK_EQUAL( oxm::value<>(f_eth_src_1),
                       oxm::value<oxm::eth_src>( ethaddr("00:bb:00:dd:00:ff")) );
    BOOST_CHECK_EQUAL( oxm::mask<>(oxm::field<>(
                    oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")) )),
            oxm::mask<oxm::eth_src>(ethaddr("ff:ff:ff:ff:ff:ff")) );
    BOOST_CHECK_EQUAL( oxm::mask<>(oxm::field<oxm::eth_type>(10)),
                       oxm::mask<oxm::eth_type>(0xffff) );
    BOOST_CHECK_EQUAL( oxm::mask<>(oxm::field<oxm::in_port>(10)),
                       oxm::mask<oxm::in_port>(0xffffffff) );
    BOOST_CHECK( oxm::field<>(eth_src).wildcard());
    BOOST_CHECK( oxm::field<oxm::eth_src>().wildcard());
    
    ////
    BOOST_TEST_MESSAGE( "Assignments" );
    ////
    
    oxm::field<> f_copied = f_eth_src_1 = f_exact_1;
    BOOST_CHECK_EQUAL( f_copied, f_exact_1 );
    BOOST_CHECK_EQUAL( f_eth_src_1, f_exact_1 );

    oxm::mask<> m_copied = m_eth_src_1 = m_eth_src_2;
    BOOST_CHECK_EQUAL( m_copied, m_eth_src_2 );
    BOOST_CHECK_EQUAL( m_copied, m_eth_src_2 );

    oxm::value<> v_copied = v_eth_src_aabb = v_eth_src_ddee;
    BOOST_CHECK_EQUAL( v_copied, v_eth_src_ddee );
    BOOST_CHECK_EQUAL( v_copied, v_eth_src_ddee );
}

BOOST_AUTO_TEST_CASE( value_and_mask_test ) {
    BOOST_CHECK_EQUAL(
            oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")) &
            oxm::mask<oxm::eth_src>(ethaddr("ff:00:ff:ff:00:00")),
            oxm::field<oxm::eth_src>(ethaddr("aa:00:cc:dd:00:00"),
                                     ethaddr("ff:00:ff:ff:00:00")) );

    BOOST_CHECK_EQUAL(
            oxm::mask<oxm::eth_src>(ethaddr("ff:00:ff:ff:00:00")) &
            oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")),
            oxm::field<oxm::eth_src>(ethaddr("aa:00:cc:dd:00:00"),
                                     ethaddr("ff:00:ff:ff:00:00")) );
    BOOST_CHECK_EQUAL(
            oxm::value<oxm::eth_type>(0x88cc) & oxm::mask<oxm::eth_type>(0xffff),
            oxm::field<oxm::eth_type>(0x88cc) );

    BOOST_CHECK_EQUAL(
            oxm::value<oxm::eth_type>(0x88cc) & oxm::mask<oxm::eth_type>(0),
            oxm::field<oxm::eth_type>(0, 0) );

    BOOST_CHECK_THROW( oxm::value<>(eth_src, ethaddr().to_bits()) &
                       oxm::mask<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands);

    BOOST_CHECK_THROW( oxm::mask<>(eth_src, ethaddr().to_bits()) &
                       oxm::value<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands);
}

BOOST_AUTO_TEST_CASE( match_value_value_test ) {
    BOOST_CHECK( oxm::value<oxm::in_port>(10) & oxm::value<oxm::in_port>(10) );
    BOOST_CHECK( not (oxm::value<oxm::in_port>(10) & oxm::value<oxm::in_port>(5)) );
    BOOST_CHECK_THROW( oxm::value<>(eth_src, ethaddr().to_bits()) &
                       oxm::value<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands );
}
    
BOOST_AUTO_TEST_CASE( match_value_field_test ) {
    BOOST_CHECK( oxm::value<oxm::in_port>(10) & oxm::field<oxm::in_port>(10) );
    BOOST_CHECK( not (oxm::value<oxm::in_port>(10) & oxm::field<oxm::in_port>(5)) );
    BOOST_CHECK( oxm::value<oxm::in_port>(10) & oxm::field<oxm::in_port>(0, 0) );

    BOOST_CHECK( oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")) &
                 oxm::field<oxm::eth_src>(ethaddr("aa:11:cc:dd:11:11"),
                                          ethaddr("ff:00:ff:ff:00:00")) );

    BOOST_CHECK( oxm::field<oxm::eth_src>(ethaddr("aa:11:cc:dd:11:11"),
                                          ethaddr("ff:00:ff:ff:00:00")) &
                 oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")) );

    BOOST_CHECK( not (oxm::field<oxm::eth_src>(ethaddr("a1:11:cc:dd:11:11"),
                                               ethaddr("ff:00:ff:ff:00:00")) &
                      oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff"))) );

    BOOST_CHECK_THROW( oxm::field<>(eth_src, ethaddr().to_bits()) &
                       oxm::value<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands );

    BOOST_CHECK_THROW( oxm::value<>(eth_src, ethaddr().to_bits()) &
                       oxm::field<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands );
}

BOOST_AUTO_TEST_CASE( match_field_field_test ) {
    BOOST_CHECK( oxm::field<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff"),
                                          ethaddr("ff:00:ff:00:ff:00")) &
                 oxm::field<oxm::eth_src>(ethaddr("11:22:33:44:55:66"),
                                          ethaddr("00:ff:00:ff:00:ff")) );

    BOOST_CHECK( oxm::field<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff"),
                                          ethaddr("ff:00:ff:00:ff:00")) &
                 oxm::field<oxm::eth_src>(ethaddr("11:22:3c:44:55:66"),
                                          ethaddr("00:ff:0f:ff:00:ff")) );

    BOOST_CHECK( oxm::field<oxm::eth_src>(ethaddr("aa:bb:cc:d4:ee:ff"),
                                          ethaddr("ff:00:ff:0f:ff:00")) &
                 oxm::field<oxm::eth_src>(ethaddr("11:22:33:44:55:66"),
                                          ethaddr("00:ff:00:ff:00:ff")) );

    BOOST_CHECK( not(oxm::field<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff"),
                                              ethaddr("ff:00:ff:00:ff:00")) &
                     oxm::field<oxm::eth_src>(ethaddr("11:22:33:44:55:66"),
                                              ethaddr("00:ff:0f:ff:00:ff"))) );

    BOOST_CHECK_THROW( oxm::field<>(eth_src, ethaddr().to_bits()) &
                       oxm::field<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands );
}

BOOST_AUTO_TEST_CASE( rewrite_value_value_test ) {
    BOOST_CHECK_EQUAL( oxm::value<oxm::eth_type>(10) >> oxm::value<oxm::eth_type>(20)
                     , oxm::value<oxm::eth_type>(20) );
    BOOST_CHECK_THROW( oxm::value<>(eth_src, ethaddr().to_bits()) >> 
                       oxm::value<>(eth_dst, ethaddr().to_bits()),
                       oxm::bad_operands );
}

BOOST_AUTO_TEST_CASE( rewrite_value_field_test ) {
    BOOST_CHECK_EQUAL( oxm::value<oxm::eth_src>(ethaddr("aa:bb:cc:dd:ee:ff")) >>
                       oxm::field<oxm::eth_src>(ethaddr("11:22:33:44:55:66"),
                                                ethaddr("ff:00:ff:00:ff:00")),
                       oxm::value<oxm::eth_src>(ethaddr("11:bb:33:dd:55:ff")) );
}

BOOST_AUTO_TEST_CASE( rewrite_field_field_test ) {
    BOOST_CHECK_EQUAL( oxm::field<oxm::eth_src>(ethaddr("aa:bb:00:00:00:00"),
                                                ethaddr("ff:ff:00:00:00:00")) >>
                       oxm::field<oxm::eth_src>(ethaddr("00:11:22:33:00:00"),
                                                ethaddr("00:ff:ff:ff:00:00"))
                     , oxm::field<oxm::eth_src>(ethaddr("aa:11:22:33:00:00"),
                                                ethaddr("ff:ff:ff:ff:00:00")) );
}

BOOST_AUTO_TEST_SUITE_END( )
