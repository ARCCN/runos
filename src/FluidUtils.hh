#pragma once

#include <functional>
#include <string>
#include <cstdlib>
#include <fluid/util/ethaddr.hh>

namespace std {

template<>
struct hash<EthAddress> : private hash<string> {
    typedef EthAddress argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& s) const {
        return std::hash<string>()(s.to_string());
    }
};

}
