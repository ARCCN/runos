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

#include <runos/build-config.hpp>
#include <runos/core/crash_reporter.hpp>
#include <runos/core/logging.hpp>
#include <runos/core/assertion_setup.hpp>

#ifndef RUNOS_DISABLE_CRASH_REPORTING

#include <breakpad/exception_handler.h>

#include <memory>
#include <new>

namespace runos {
namespace crash_reporter {

using google_breakpad::ExceptionHandler;

static std::unique_ptr<ExceptionHandler> handler;

bool filterCallback(void * /*context == NULL*/)
{
    return true;
}

bool minidumpCallback(const char *dump_dir,
                      const char *minidump_id,
                      void *context, bool succeeded)
{
    if (succeeded) {
        LOG(INFO) << "Minidump saved: " << minidump_id;
    } else {
        LOG(ERROR) << "Can't create minidump";
    }
    return true;
}

void setup(const char* dump_path)
{
    const char* serverPort = NULL;

    handler.reset(new (std::nothrow)
        ExceptionHandler(dump_path,
                         &filterCallback,
                         &minidumpCallback, NULL,
                         true, serverPort));

    on_assertion_failed([](std::string const& /*msg*/,
                           const char* /*expr*/,
                           std::string const& /*expr_decomposed*/,
                           source_location const& /*where*/)
    {
        snapshot();
    });
}

void snapshot(/*TODO: reason, appdata */)
{
    if (handler) handler->WriteMinidump();
}

} // crash_reporter

void precatch_shim_handler(std::type_info* exception_type, void* thrown_object)
{
    if (*exception_type == typeid(assertion_error))
        return;
    crash_reporter::snapshot();
}

} // runos

#else

namespace runos {
void precatch_shim_handler(std::type_info* exception_type, void* thrown_object)
{
}
}

#endif
