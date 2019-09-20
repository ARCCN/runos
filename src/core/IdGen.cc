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

#include <runos/IdGen.hpp>

#include <runos/core/throw.hpp>

namespace runos {

IdGen::IdGen(uint64_t first, uint64_t capacity, acquire_order order) :
    first(first), capacity(capacity), order(order), pool{{first, first + capacity}} {
    THROW_IF(!capacity || first + capacity < first, invalid_argument(), "invalid id range");
}

uint64_t IdGen::acquire() {
    std::lock_guard<std::mutex> lock(mut);
    THROW_IF(pool.empty(), PoolIsEmpty(), "no unused ids left");

    uint64_t id = 0;
    if (order == acquire_order::backward) {
        // get the element from the back
        auto &back = pool.back();
        id = --back.last;
        if (back.first == back.last)
            pool.pop_back();
    } else if (order == acquire_order::forward) {
        auto &front = pool.front();
        id = front.first++;
        if (front.first == front.last)
            pool.erase(pool.begin());
    }
    return id;
}

void IdGen::recovery(const std::vector<uint64_t> &booked_ids) {
    THROW_IF(booked_ids.size() > capacity, Error(),
             "generator's size smaller rerovering id's number");
    std::lock_guard<std::mutex> lock(mut);
    pool = {{first, first + capacity}};
    for (auto id: booked_ids) {
        THROW_IF(id < first || id >= first + capacity, invalid_argument(), "id is out of range");
        // find the upper-bound segment
        auto iter = pool.begin(), iter_end = pool.end();
        for (; iter != iter_end; ++iter) {
            if (iter->first <= id && iter->last > id) break;
        }
        THROW_IF(iter == iter_end, Error(), "id is already used");
    
        if (iter->first + 1 == iter->last) {
            pool.erase(iter);
            continue;
        }

        uint64_t segment_border = iter->last;
        iter->last = id;
        if (id + 1 != segment_border) {
            if (iter + 1 == iter_end)
                pool.push_back(segment(id + 1, segment_border));
            else
                pool.insert( iter + 1, segment( id + 1, segment_border));
        }
    }
}

void IdGen::release(uint64_t id) {
    THROW_IF(id < first || id >= first + capacity, invalid_argument(), "id is out of range");

    std::lock_guard<std::mutex> lock(mut);
    if (pool.empty()) {
        pool.emplace_back(id, id + 1);
        return;
    }

    // find the upper-bound segment
    auto iter = pool.begin(), iter_end = pool.end();
    for(; iter != iter_end; ++iter) if (iter->first > id) break;
    if (iter != pool.begin() && std::prev(iter)->last > id) {
        THROW(DoubleRelease(), "id is already in pool");
    }

    uint64_t prev_last(-1), next_first(0);
    if (iter != pool.begin()) prev_last = std::prev(iter)->last;
    if (iter != iter_end) next_first = iter->first;

    if (prev_last == next_first - 1) {
        // id joins two neighbor segments into a signle one
        std::prev(iter)->last = iter->last;
        pool.erase(iter);
    } else if (prev_last == id) {
        // id adjoins to previos segment
        ++std::prev(iter)->last;
    } else if (id == next_first - 1) {
        // id adjoins to next segment
        --iter->first;
    } else {
        // id is isolated
        pool.emplace(iter, id, id + 1);
    }
}

uint64_t IdGen::unused() const {
    std::lock_guard<std::mutex> lock(mut);
    uint64_t unused = 0;
    for(auto &item : pool)
      unused += (item.last - item.first);
    return unused;
}

json IdGen::to_json() const {
    json ret {
        { "first", first },
        { "capacity", capacity },
        { "pool", json::array() }
    };

    for (const auto& s : pool) {
        ret["pool"].push_back({ s.first, s.last });
    }

    return ret;
}

void IdGen::from_json(const json& obj) {
    auto j_first = obj["first"].get<uint64_t>();
    auto j_capacity = obj["capacity"].get<uint64_t>();
    THROW_IF(first != j_first || capacity != j_capacity,
            invalid_argument(), "incorrect values in json");

    pool.clear();
    for (const json& s : obj["pool"]) {
        pool.emplace_back(s.at(0), s.at(1));
    }
}

} // runos
