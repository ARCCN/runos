#pragma once

#include <map>
#include <ostream>
#include <boost/test/unit_test.hpp>

#include "api/Packet.hh"
#include "oxm/field_set.hh"
#include "maple/Backend.hh"
#include "maple/Flow.hh"

namespace runos {

class MockSwitch : public maple::Backend {
    using Flow = maple::Flow;
    using FlowPtr = maple::FlowPtr;

    struct Classifier {
        unsigned prio;
        oxm::field_set match;

        friend bool operator== (const Classifier& lhs, const Classifier& rhs)
        { return lhs.prio == rhs.prio && lhs.match == rhs.match; }

        friend bool operator< (const Classifier& lhs, const Classifier& rhs)
        { return lhs.prio > rhs.prio; }
        
        friend std::ostream& operator<< (std::ostream& out, const Classifier& c)
        { return out << "prio=" << c.prio << ", " << c.match; }
    };

    typedef std::map< Classifier, FlowPtr >
        FlowTable;

    FlowPtr miss;
    FlowTable flow_table;

public:
    explicit MockSwitch(FlowPtr miss)
        : miss(miss)
    { }
   
    virtual void install(unsigned priority,
                         oxm::field_set const& fs,
                         FlowPtr flow) override
    {
        Classifier classifier {priority, fs};
        BOOST_TEST_MESSAGE("Installing flow " << classifier);
        BOOST_REQUIRE_MESSAGE(flow_table.emplace(classifier, flow).second,
                              "Overlapping flow " << classifier);
    }

    virtual void remove(FlowPtr flow) override
    {
        BOOST_TEST_MESSAGE("Removing flow " << flow);
        for (auto it = flow_table.begin(); it != flow_table.end(); ++it) {
            if (it->second == flow)
                flow_table.erase(it);
        }
    }

    virtual void remove(unsigned priority,
                        oxm::field_set const& match) override
    {
        Classifier key{ priority, std::move(match) };
        BOOST_TEST_MESSAGE("Removing flow " << key);
        for (auto it = flow_table.begin(); it != flow_table.end(); ++it) {
            if (it->first == key)
                flow_table.erase(it);
        }
    }

    virtual void remove(oxm::field_set const& match) override
    {
        BOOST_TEST_MESSAGE("Removing flow " << match);
        for (auto it = flow_table.begin(); it != flow_table.end(); ++it) {
            if (it->first.match == match)
                flow_table.erase(it);
        }
    }

    FlowPtr operator()(const Packet& pkt) const
    {
        auto it = flow_table.cbegin();
        auto match = flow_table.cend();

        for (; it != flow_table.cend(); ++it) {
            if (it->first.match & pkt) {
                match = it;
                break;
            }
        }

        if (it != flow_table.cend())
            ++it;

        for (; it != flow_table.cend() &&
               it->first.prio == match->first.prio;
               ++it)
        {
            BOOST_REQUIRE( not (it->first.match & pkt) );
        }

        if (match != flow_table.cend())
            return match->second;
        else
            return miss;
    }
};

}
