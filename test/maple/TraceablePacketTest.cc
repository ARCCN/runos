#define BOOST_TEST_MODULE Trace tree compressor testcases

#include <boost/test/unit_test.hpp>

#include "oxm/field.hh"
#include "oxm/field_set.hh"
#include "oxm/openflow_basic.hh"

#include "api/Packet.hh"
#include "api/TraceablePacket.hh"
#include "maple/Tracer.hh"
#include "maple/TraceablePacketImpl.hh"

#include "gmock-adapter.hh"

using ::testing::Return;
using ::testing::InSequence;
using ::testing::_;
using ::testing::Eq;

using runos::Packet;
using runos::TraceablePacket;
using runos::TraceableProxy;
using runos::packet_cast;
using runos::maple::TraceablePacketImpl;
using runos::maple::Tracer;
namespace oxm = runos::oxm;

struct Fields {
    oxm::in_port   in_port;
    oxm::eth_type  eth_type;
    oxm::eth_src   eth_src;
    oxm::eth_dst   eth_dst;
    oxm::ip_proto  ip_proto;
    //oxm::ipv4_src  ipv4_src;
    //oxm::ipv4_dst  ipv4_dst;
    oxm::tcp_src   tcp_src;
    oxm::tcp_dst   tcp_dst;
    oxm::udp_src   udp_src;
    oxm::udp_dst   udp_dst;
};

struct PacketA : Fields {
    oxm::field_set raw_pkt;

    PacketA()
    {
        raw_pkt.modify( oxm::value<oxm::in_port>(3) );
        raw_pkt.modify( oxm::value<oxm::eth_type>(0x0800) ); // ipv4
        raw_pkt.modify( oxm::value<oxm::eth_src>("11:22:33:44:55:66") );
        raw_pkt.modify( oxm::value<oxm::eth_dst>("aa:bb:cc:dd:ee:ff") );
        raw_pkt.modify( oxm::value<oxm::ip_proto>(6) ); // tcp
        raw_pkt.modify( oxm::value<oxm::tcp_src>(45678) );
        raw_pkt.modify( oxm::value<oxm::tcp_dst>(80) );
    }
};

struct PacketB : Fields {
    oxm::field_set raw_pkt;

    PacketB()
    {
        raw_pkt.modify( oxm::value<oxm::in_port>(4) );
        raw_pkt.modify( oxm::value<oxm::eth_type>(0x0800) ); // ipv4
        raw_pkt.modify( oxm::value<oxm::eth_src>("66:55:44:33:22:11") );
        raw_pkt.modify( oxm::value<oxm::eth_dst>("ff:ee:dd:cc:bb:aa") );
        raw_pkt.modify( oxm::value<oxm::ip_proto>(17) ); // udp
        raw_pkt.modify( oxm::value<oxm::udp_src>(34567) );
        raw_pkt.modify( oxm::value<oxm::udp_dst>(53) ); // dns
    }
};

struct MockTracer : Tracer {
    MOCK_METHOD1(load, void(oxm::field<>));
    MOCK_METHOD2(test, void(oxm::field<>, bool));
    MOCK_METHOD1(finish, void(std::shared_ptr<runos::maple::Flow>));
};

BOOST_AUTO_TEST_SUITE( runos_maple_traceable_tests )

BOOST_FIXTURE_TEST_CASE( test1, PacketA ) {
    // expectations
    MockTracer tracer;
    {
        InSequence seq;
        // #1
        EXPECT_CALL( tracer,
                load(Eq(oxm::field<oxm::eth_src>{0x60, 0xf0})) );
        // #2
        EXPECT_CALL( tracer,
                test(Eq(oxm::field<oxm::eth_src>{0x06, 0x0f}), true) );
        // #3
        EXPECT_CALL( tracer,
                test(Eq(oxm::field<oxm::eth_dst>{0xee00, 0xff00}), true) );
        // #4
        EXPECT_CALL( tracer,
                test(Eq(oxm::field<oxm::eth_dst>{0x20000, 0xf0000}), false) );
        // #5
        EXPECT_CALL( tracer,
                load(Eq(oxm::field<oxm::eth_dst>{0xd0000, 0xf0000})) );
        // #6
        EXPECT_CALL( tracer,
                load(Eq(oxm::field<oxm::eth_dst>{"aa:bb:cc:d0:00:00",
                                                 "ff:ff:ff:f0:00:00"})) );
        // #7
        EXPECT_CALL( tracer,
                load(Eq(oxm::field<oxm::tcp_src>{45678})) );
        // #8
        EXPECT_CALL( tracer,
                test(_, false) )
            .WillRepeatedly(Return());
        // #9
        EXPECT_CALL( tracer,
                test(Eq(oxm::field<oxm::tcp_dst>{80}), true) );
    }

    // tests
    TraceablePacketImpl tpkt_impl( raw_pkt, tracer );
    // cast to use template methods
    Packet& pkt = tpkt_impl;
    auto tpkt = packet_cast<TraceablePacket>(pkt);

    BOOST_CHECK_EQUAL( pkt.load(eth_src & 0xf0),
                       oxm::field<oxm::eth_src>(0x60, 0xf0) ); // #1
    
    // tpkt must compute result without touching iterator
    BOOST_CHECK( not pkt.test((eth_src & 0xff0) == 0x570) );
    // however, this will load next chunk
    BOOST_CHECK( pkt.test((eth_src & 0xff) == 0x66) ); // #2

    pkt.modify( (eth_dst & 0xff) << 0xee );
    BOOST_CHECK_EQUAL( raw_pkt.Packet::load(eth_dst), 
                       oxm::value<oxm::eth_dst>("aa:bb:cc:dd:ee:ee") );

    // should load only one octet
    BOOST_CHECK( pkt.test((eth_dst & 0xffff) == 0xeeee) ); // #3

    BOOST_CHECK( not pkt.test((eth_dst & 0xff) == 0x11) ); // deduce
    BOOST_CHECK( not pkt.test((eth_dst & 0xff000) == 0xd2000) ); // deduce
    BOOST_CHECK( not pkt.test((eth_dst & 0xff000) == 0x2e000) ); // #4

    // should load, because we only know that this bits aren't equal to 2
    BOOST_CHECK_EQUAL( pkt.load(eth_dst & 0xf0000),
            oxm::field<oxm::eth_dst>(0xd0000, 0xf0000) ); // #5

    // lood remaining bits
    BOOST_CHECK_EQUAL( pkt.load(eth_dst),
                       oxm::value<oxm::eth_dst>("aa:bb:cc:dd:ee:ee") ); // #6

    BOOST_CHECK_EQUAL( pkt.load(tcp_src),
                       oxm::value<oxm::tcp_src>(45678) ); // #7

    for (int dport = 0; dport < 160; ++dport) {
        BOOST_CHECK( (dport == 80) == pkt.test(tcp_dst == dport) ); // #8, #9
    }

    // watch
    BOOST_CHECK_EQUAL( tpkt.watch(in_port), oxm::value<oxm::in_port>(3) );
    pkt.modify(in_port << 4);
    BOOST_CHECK_EQUAL( raw_pkt.Packet::load(in_port),
                       oxm::value<oxm::in_port>(4) );
    BOOST_CHECK_EQUAL( tpkt.watch(in_port), oxm::value<oxm::in_port>(4) );
    BOOST_CHECK_EQUAL( pkt.load(in_port), oxm::value<oxm::in_port>(4) );
}

BOOST_AUTO_TEST_SUITE_END( )
