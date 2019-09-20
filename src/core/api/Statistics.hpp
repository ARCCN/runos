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

#include <cstdint>
#include <array>

namespace runos {

template<template<class> class Measurement>
struct Statistics {
    template<class T>
    using measurement_type = Measurement<T>;

    Measurement<uint64_t> integral;
    Measurement<double> current_speed;
    Measurement<double> max_speed;
};

template<class T>
struct TrafficMeasurement : std::array<T, 2> {
    using std::array<T, 2>::array;

    T& rx_packets() { return (*this)[0]; }
    const T& tx_packets() const { return (*this)[0]; }
    T& rx_bytes() { return (*this)[1]; }
    const T& tx_bytes() const { return (*this)[1]; }
};

template<class T>
struct PortMeasurement : std::array<T, 8> {
    using std::array<T, 8>::array;

    T& rx_packets() { return (*this)[0]; }
    const T& rx_packets() const { return (*this)[0]; }
    T& tx_packets() { return (*this)[1]; }
    const T& tx_packets() const { return (*this)[1]; }

    T& rx_bytes() { return (*this)[2]; }
    const T& rx_bytes() const { return (*this)[2]; }
    T& tx_bytes() { return (*this)[3]; }
    const T& tx_bytes() const { return (*this)[3]; }

    T& rx_dropped() { return (*this)[4]; }
    const T& rx_dropped() const { return (*this)[4]; }
    T& tx_dropped() { return (*this)[5]; }
    const T& tx_dropped() const { return (*this)[5]; }

    T& rx_errors() { return (*this)[6]; }
    const T& rx_errors() const { return (*this)[6]; }
    T& tx_errors() { return (*this)[7]; }
    const T& tx_errors() const { return (*this)[7]; }

};

template<class T>
struct QueueMeasurement : std::array<T, 3> {
    using std::array<T, 3>::array;

    T& tx_bytes() { return (*this)[0]; }
    const T& tx_bytes() const { return (*this)[0]; }
    T& tx_packets() { return (*this)[1]; }
    const T& tx_packets() const { return (*this)[1]; }
    T& tx_errors() { return (*this)[2]; }
    const T& tx_errors() const { return (*this)[2]; }
};

template<class T>
struct FlowMeasurement : std::array<T, 3> {
    using std::array<T, 3>::array;

    T& bytes() { return (*this)[0]; }
    const T& bytes() const { return (*this)[0]; }
    T& packets() { return (*this)[1]; }
    const T& packets() const { return (*this)[1]; }
    T& flows() { return (*this)[2]; }
    const T& flows() const { return (*this)[2]; }
};

template<class T>
struct TableMeasurement : std::array<T, 3> {
    using std::array<T, 3>::array;

    T& active_entries() { return (*this)[0]; }
    const T& active_entries() const { return (*this)[0]; }
    T& packets() { return (*this)[1]; }
    const T& packets() const { return (*this)[1]; }
    T& matched_packets() { return (*this)[2]; }
    const T& matched_packets() const { return (*this)[2]; }
};

template<class T>
struct GroupMeasurement : std::array<T, 3> {
    using std::array<T, 3>::array;

    T& bytes() { return (*this)[0]; }
    const T& bytes() const { return (*this)[0]; }
    T& packets() { return (*this)[1]; }
    const T& packets() const { return (*this)[1]; }
    T& references() { return (*this)[2]; }
    const T& references() const { return (*this)[2]; }
};

template<class T>
struct MeterMeasurement : std::array<T, 3> {
    using std::array<T, 3>::array;

    T& rx_packets() { return (*this)[0]; }
    const T& rx_packets() const { return (*this)[0]; }
    T& rx_bytes() { return (*this)[1]; }
    const T& rx_bytes() const { return (*this)[1]; }
    T& flows() { return (*this)[2]; }
    const T& flows() const { return (*this)[2]; }
};

} // namespace runos
