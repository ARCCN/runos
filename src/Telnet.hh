#pragma once

#include "types/ipv4addr.hh"

#include <memory>

namespace telnet {

class Telnet {
public:
    Telnet();
    ~Telnet();

    struct implementation;
    std::unique_ptr<implementation> m_impl;
};

} // namespace telnet
