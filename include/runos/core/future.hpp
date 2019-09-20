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

#include <runos/core/future-decl.hpp>
#include <runos/core/detail/when_all_fix.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/futures/wait_for_all.hpp>
#include <boost/thread/futures/wait_for_any.hpp>

namespace runos {

using boost::future_error;
using boost::when_all;
using boost::when_any;
using boost::make_ready_future;

} // namespace
