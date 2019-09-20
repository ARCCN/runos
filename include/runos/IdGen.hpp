/*
 * Copyright 2019 Applied Research Center for Computer Networks
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

#include <runos/core/exception.hpp>
#include <json.hpp>

#include <cstdint>
#include <mutex>
#include <vector>

namespace runos {
using json = nlohmann::json;

/*! Builds a pool of identifiers over a continuous range of integers
 *  and provides safe concurent access to contents of this pool
 */
class IdGen {
public:
    enum class acquire_order {
        forward,
        backward
    };

    struct Error : exception_root {};
    struct PoolIsEmpty : Error, runtime_error_tag {};
    struct DoubleRelease : Error, runtime_error_tag {};

    //! use @capacity of integers, starting from @first
    //!  given range shall never include UINT64_MAX
    IdGen(uint64_t first, uint64_t capacity,
            acquire_order order = acquire_order::backward);
    const uint64_t first, capacity;

    //! get an unused identifier from the pool
    uint64_t acquire();
    
    //! recovery generator's configuration
    void recovery(const std::vector<uint64_t> &booked_ids);

    //! return granted identifier to the pool
    void release(uint64_t id);

    uint64_t unused() const;
    uint64_t used() const {
        return capacity - unused();
    }

    bool inside(uint64_t val) const {
        return (val >= first && val < first + capacity);
    }

    json to_json() const;
    void from_json(const json& obj);

private:
    /// leave backdoor for unit testing
    friend class IdGenTester;

    struct segment {
        segment(uint64_t first, uint64_t last) :
            first(first), last(last) {}
        uint64_t first, last;
    };

    acquire_order order;
    std::vector<segment> pool;
    mutable std::mutex mut;
};

} // runos
