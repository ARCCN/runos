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

#include "Flow.hh"

#include <atomic>
#include <boost/assert.hpp>

namespace runos {

constexpr uint64_t COOKIE_BASE = 1ULL << 63ULL;
constexpr uint64_t COOKIE_MASK = 1ULL << 63ULL;
constexpr uint64_t FIRST_COOKIE = COOKIE_BASE;
static std::atomic_uint_least64_t next_cookie { FIRST_COOKIE };

Flow::Flow() noexcept
    : m_cookie(next_cookie++)
{
    BOOST_ASSERT( m_cookie >= FIRST_COOKIE );
}

std::pair<uint64_t, uint64_t> Flow::cookie_space()
{
    return std::make_pair(COOKIE_BASE, COOKIE_MASK);
}

}
