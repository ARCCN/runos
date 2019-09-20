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

#include <runos/core/detail/precatch_shim.hpp>

#include <unwind.h>
#include <cstddef>
#include <cstdint>
#include <typeinfo>
#include <exception>

// Clang Exception classes:
//  CLNGC++\0
//  CLNGC++\1 - depended (rethrown)
// GCC Exception classes:
//  GNUCC++\0
//  GNUCC++\1 - depended
//
//  In general, exception class is written in form AAAABBBC where:
//      AAAA - vendor
//      BBB - language
//      C - depended or not (C++ only?)

static const uint64_t cpp_exception = 0x00000000432B2B00; // CLNGC++\0 
static const uint64_t get_language  = 0x00000000FFFFFF00; // mask for ????C++?
static const uint64_t is_dependent  = 0x00000000000000FF; // mask for ????C++?

// Conforms Intel Itanium C++ ABI level II
struct __cxa_exception { 
    size_t referenceCount;

    std::type_info *	exceptionType;
    void (*exceptionDestructor) (void *); 
    std::unexpected_handler	unexpectedHandler;
    std::terminate_handler	terminateHandler;
    __cxa_exception *	nextException;

    int             handlerCount;
    int             handlerSwitchValue;
    const char *    actionRecord;
    const char *    languageSpecificData;
    void *          catchTemp;
    void *          adjustedPtr;

    _Unwind_Exception	unwindHeader;
};

struct __cxa_dependent_exception { 
    void* primaryException;

    std::type_info *	exceptionType;
    void (*exceptionDestructor) (void *); 
    std::unexpected_handler	unexpectedHandler;
    std::terminate_handler	terminateHandler;
    __cxa_exception *	nextException;

    int             handlerCount;
    int             handlerSwitchValue;
    const char *    actionRecord;
    const char *    languageSpecificData;
    void *          catchTemp;
    void *          adjustedPtr;

    _Unwind_Exception	unwindHeader;
};

static
void*
get_thrown_object_ptr(_Unwind_Exception* unwind_exception)
{
    void* adjustedPtr = unwind_exception + 1;
    if (unwind_exception->exception_class & is_dependent)
        adjustedPtr = ((__cxa_dependent_exception*)adjustedPtr - 1)->primaryException;
    return adjustedPtr;
}

extern "C"
_Unwind_Reason_Code
precatch_shim_personality(int                version,
                          _Unwind_Action     actions,
                          uint64_t           exceptionClass,
                          _Unwind_Exception* unwind_exception,
                          _Unwind_Context*   /*context*/)
{
    if (version == 1 && (actions & _UA_SEARCH_PHASE)) {
        if ((exceptionClass & get_language) == cpp_exception) {
            __cxa_exception* exception_header = (__cxa_exception*)(unwind_exception+1) - 1;
            runos::precatch_shim_handler(exception_header->exceptionType,
                                         get_thrown_object_ptr(unwind_exception));
        } else {
            runos::precatch_shim_handler(nullptr, nullptr);
        }
    }
    return _URC_CONTINUE_UNWIND;
}
