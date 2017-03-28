#pragma once

#include <sstream>
#include "Tracer.hh"

namespace runos {
namespace maple {

class LoggableTracer : public Tracer {
    Tracer& m_wrapee;
    std::ostringstream m_log;
public:
    explicit LoggableTracer(Tracer& wrapee)
        : m_wrapee(wrapee)
    { }

    void load(oxm::field<> unexplored) override;
    void test(oxm::field<> pred, bool ret) override;
    void vload(oxm::field<> by, oxm::field<> what) override;
    Installer finish(FlowPtr flow) override;

    std::string log() const
    { return m_log.str(); }

    ~LoggableTracer();
};

} // namespace maple
} // namespace runos
