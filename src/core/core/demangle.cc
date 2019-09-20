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

#include <runos/core/demangle.hpp>

#include <boost/core/demangle.hpp>
#include <string>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace runos {

std::string demangle( const char* name )
{
    if ( strcmp(name, typeid(std::string).name()) == 0 )
        return "std::string";
    if ( strcmp(name, typeid(std::string_view).name()) == 0 )
        return "string_view";

    if ( strcmp(name, typeid(uint8_t).name()) == 0 )
        return "uint8_t";
    if ( strcmp(name, typeid(uint16_t).name()) == 0 )
        return "uint16_t";
    if ( strcmp(name, typeid(uint32_t).name()) == 0 )
        return "uint32_t";
    if ( strcmp(name, typeid(uint64_t).name()) == 0 )
        return "uint64_t";

    if ( strcmp(name, typeid(int8_t).name()) == 0 )
        return "int8_t";
    if ( strcmp(name, typeid(int16_t).name()) == 0 )
        return "int16_t";
    if ( strcmp(name, typeid(int32_t).name()) == 0 )
        return "int32_t";
    if ( strcmp(name, typeid(int64_t).name()) == 0 )
        return "int64_t";

    return boost::core::demangle(name);
}

} // runos
