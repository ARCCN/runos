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

#include <runos/build-config.hpp>

#ifndef RUNOS_DISABLE_ASSERTIONS
#define RUNOS_ENABLE_ASSERTIONS
#endif

#ifndef RUNOS_DONT_USE_SHORT_MACRO
#define RUNOS_USE_SHORT_MACRO
#endif

#ifndef RUNOS_DISABLE_CRASH_REPORTING
#define RUNOS_ENABLE_CRASH_REPORTING
#endif

#ifndef RUNOS_DISABLE_EXPRESSION_DECOMPOSER
#define RUNOS_ENABLE_EXPRESSION_DECOMPOSER
#endif
