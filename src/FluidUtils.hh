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
