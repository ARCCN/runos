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

#include <chrono>
#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip_with.hpp>
#include <range/v3/view/bounded.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/algorithm/copy.hpp>

#include "api/Statistics.hpp"

namespace runos {

template<template<class> class Measurement>
class StatisticsStore {
public:
    template<class T>
    using measurement_type = Measurement<T>;
    //using statistics_type = Statistics<measurement_type>; // CLANG BUG
    using statistics_type = Statistics<Measurement>;

    StatisticsStore()
    {
        reset();
    }

    statistics_type get() const
    {
        return {curr_, current_speed(), max_};
    }

    void reset_without_max()
    {
        prev_time_ = std::chrono::seconds(0);
        curr_time_ = std::chrono::seconds(0);
        ranges::fill(prev_, 0);
        ranges::fill(curr_, 0);
    }

    void reset()
    {
        reset_without_max();
        ranges::fill(max_, 0);
    }

    template<class Duration>
    void append(Duration next_time, measurement_type<uint64_t> next)
    {
        if (curr_time_ > next_time) {
            reset_without_max();
        }

        prev_ = curr_;
        prev_time_ = curr_time_;
        
        curr_time_ = next_time;
        curr_ = next;

        // for example, when we rewrite bucket's flows
        if (curr_ < prev_)
            ranges::fill(prev_, 0);

        auto max_fn = [](double a, double b) { return std::max(a, b); };
        auto speed = current_speed();

        ranges::copy(
            ranges::view::zip_with(max_fn, max_, speed)
          , max_.begin()
        );
    }

private:
    using fpseconds
        = std::chrono::duration<double>;

    fpseconds prev_time_;
    fpseconds curr_time_;

    measurement_type<uint64_t> prev_;
    measurement_type<uint64_t> curr_;
    measurement_type<double> max_;

    measurement_type<double> current_speed() const
    {
        using namespace ranges;
        using std::chrono::seconds;
        using std::chrono::duration_cast;

        auto delta = (curr_time_ - prev_time_).count();

        measurement_type<double> ret;
        ranges::copy( view::zip_with(std::minus<uint64_t>{}, curr_, prev_)
                    | view::transform([=](uint64_t s) { return s / delta; })
                    , ret.begin());
        return std::move(ret);
    }
};

} // namespace runos

