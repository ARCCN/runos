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

#include <memory>

namespace runos {

struct mod_reuse_trait {
    template<class T>
    using pointer_type = T*;

    template<template<class> class T>
    using return_type = pointer_type<T<mod_reuse_trait>>;
};

struct mod_create_trait {
    template<class T>
    using pointer_type = std::unique_ptr<T>;

    template<template<class> class T>
    using return_type = pointer_type<T<mod_reuse_trait>>;
};

} // namespace runos
