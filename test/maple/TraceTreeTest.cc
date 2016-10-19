#define BOOST_TEST_MODULE Trace tree testcases

#include <limits>
#include <memory>
#include <forward_list>
#include <functional> // bind

#include <boost/test/unit_test.hpp>

#include "oxm/type.hh"
#include "oxm/field.hh"
#include "oxm/field_set.hh"
#include "api/Packet.hh"
#include "maple/Backend.hh"
#include "maple/Runtime.hh"
#include "MockSwitch.hh"

using namespace runos;

template<size_t N>
struct F : oxm::define_type< F<N>, 0, N, 32, uint32_t, uint32_t, true >
{ };

template<class Decision, Decision undefined>
class MockFlow final : public maple::Flow {
    Decision m_decision {undefined};
public:
    MockFlow() = default;

    explicit MockFlow(Decision other)
        : m_decision(other)
    { }

    Decision decision()
    { return m_decision; }
    void decision(Decision other)
    { m_decision = other; }
    bool disposable()
    { return false; }
    friend bool operator==(const MockFlow& lhs, const MockFlow& rhs)
    { return lhs.m_decision == rhs.m_decision; }

    MockFlow& operator=(const MockFlow& other)
    { m_decision = other.m_decision; return *this; }

    Flow& operator=(const Flow& other) override
    { return *this = dynamic_cast<const MockFlow&>(other); }

    friend std::ostream& operator<<(std::ostream& out, MockFlow flow)
    { return out << "Flow{ id=" << flow.id << ", decision = " << flow.m_decision; }
};

int static_policy(Packet& pkt) {
    uint32_t f1 = pkt.load(F<1>());
    if (f1 % 2 == 0) {
        if (pkt.test((F<2>() & 0xf) == 0xa)) {
            return f1 + 1;
        } else {
            return f1 + 2;
        }
    } else {
        if (pkt.test((F<3>() & 0xff) == 0xaa)) {
            return f1 - 1;
        } else {
            return f1 - 2;
        }
    }
}

BOOST_AUTO_TEST_SUITE( trace_tree_tests )

BOOST_AUTO_TEST_CASE( static_policy_test ) {
    constexpr int miss = std::numeric_limits<int>::min();
    constexpr int undefined = miss + 1;
    using Flow = MockFlow<int, undefined>;

    std::forward_list<std::shared_ptr<Flow>> flows;
    auto make_flow = [&flows]() {
        auto ret = std::make_shared<Flow>();
        flows.push_front(ret);
        return ret;
    };

    auto miss_flow = std::make_shared<Flow>( miss );
    MockSwitch switch_ { miss_flow };
    maple::Runtime<int, Flow> runtime {
        std::bind(static_policy, std::placeholders::_1),
        switch_,
        miss_flow
    };

    oxm::field_set pkt = {
        { F<1>() == 100 },
        { F<2>() == 0xA }
    };

    BOOST_CHECK_EQUAL( runtime(pkt), miss_flow );
    auto flow100_A = runtime.augment(pkt, make_flow());

    BOOST_CHECK_EQUAL( runtime(pkt), flow100_A );
    BOOST_CHECK_EQUAL( flow100_A->decision(), static_policy(pkt) );

    pkt = oxm::field_set {
        { F<1>() == 101 },
        { F<3>() == 0xBB }
    };
    auto flow101_NA = runtime.augment(pkt, make_flow());

    for (uint32_t f3 = 0; f3 <= 0xFF; ++f3) {
        pkt.modify(F<3>() << f3);
        if ((f3 & 0xff) == 0xaa) {
            BOOST_CHECK_EQUAL( runtime(pkt), miss_flow );
        } else {
            BOOST_CHECK_EQUAL( runtime(pkt)->decision(), static_policy(pkt) );
            BOOST_CHECK_EQUAL( runtime(pkt), flow101_NA );
        }
    }

    pkt = oxm::field_set {
        { F<1>() == 100 },
        { F<2>() == 0xB }
    };
    auto flow100_NA = runtime.augment(pkt, make_flow());

    for (uint32_t f2 = 0; f2 <= 0xFF; ++f2) {
        pkt.modify(F<2>() << f2);
        BOOST_CHECK_EQUAL( runtime(pkt)->decision(), static_policy(pkt) );
        if ((f2 & 0xf) == 0xa)
            BOOST_CHECK_EQUAL( runtime(pkt), flow100_A );
        else
            BOOST_CHECK_EQUAL( runtime(pkt), flow100_NA );
    }

    pkt = oxm::field_set {
        { F<1>() == 101 },
        { F<3>() == 0xAA }
    };
    auto flow101_A = runtime.augment(pkt, make_flow());

    for (unsigned f1 = 100; f1 <= 101; ++f1) {
        for (unsigned f2 = 0; f2 <= 0xf; ++f2) {
            for (unsigned f3 = 0; f3 <= 0xff; ++f3) {
                pkt.modify(F<1>() << f1);
                pkt.modify(F<2>() << f2);
                pkt.modify(F<3>() << f3);
                BOOST_CHECK_EQUAL(static_policy(pkt),
                                  runtime(pkt)->decision());
            }
        }
    }

    for (unsigned f1 = 50; f1 <= 99; ++f1) {
        oxm::field_set pkt;
        if (f1 % 2 == 0) {
            // True branch
            runtime.augment( pkt = oxm::field_set {
                { F<1>() << f1 },
                { F<2>() << 0x2a }
            }, make_flow());
            // False branch
            runtime.augment( pkt = oxm::field_set {
                { F<1>() << f1 },
                { F<2>() << 0x2b }
            }, make_flow());
        } else {
            // True branch
            runtime.augment( pkt = oxm::field_set {
                { F<1>() << f1 },
                { F<3>() << 0xaabb }
            }, make_flow());
            // False branch
            runtime.augment( pkt = oxm::field_set {
                { F<1>() << f1 },
                { F<3>() << 0xbbaa }
            }, make_flow());
        }
    }

    runtime.commit();
    
    BOOST_TEST_MESSAGE("Checking compiled flow table");
    for (unsigned f1 = 0; f1 <= 101; ++f1) {
        for (unsigned f2 : {0, 2, 10, 0xa, 0x2a, 0xaad}) {
            for (unsigned f3 : {0, 0xaabb, 0xbbaa, 0x1010}) {
                pkt.modify(F<1>() << f1);
                pkt.modify(F<2>() << f2);
                pkt.modify(F<3>() << f3);

                auto flow_ = switch_(pkt);
                BOOST_REQUIRE(flow_);
                auto flow = std::dynamic_pointer_cast<Flow>(flow_);
                BOOST_REQUIRE(flow);

                BOOST_CHECK_MESSAGE(runtime(pkt) == flow,
                        runtime(pkt)->decision() << " != " << flow->decision() <<
                        " for F={" << f1 << ", " << f2 << ", " << f3 << "}");

                if (f1 >= 50 && f1 <= 101) {
                    BOOST_CHECK_MESSAGE( flow->decision() == static_policy(pkt),
                        flow->decision() << " != " << static_policy(pkt) <<
                        " for F={" << f1 << ", " << f2 << ", " << f3 << "}");
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END( )
