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

#include <typeinfo>

#ifndef RUNOS_PRECATCH_SHIM_UNAVAILABLE
extern "C" {
    void precatch_shim(void(*f)(void*), void* ctx);
}
#else
    void precatch_shim(void(*f)(void*), void* ctx) { f(ctx); }
#endif

namespace runos {

void precatch_shim_handler(std::type_info* exception_type, void* thrown_object);

template<class Functor/*, class Handler*/>
void precatch_shim(Functor&& f/*, Handler&& precatch*/)
{
    static auto f_wrapped = [](void* ctx) {
        (*reinterpret_cast<Functor*>(ctx))();
    };
    // static auto handler_wrapped = [](void* ctx) {
    //     (*reinterpret_cast<Handler*>(ctx))();
    // };
    ::precatch_shim(f_wrapped, &f/*, handler_wrapped, &precatch*/);
}

} // runos
