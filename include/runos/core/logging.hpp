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

#ifndef _LOGGING_H_
#include <glog/logging.h>
#undef CHECK
#define CHECK RUNOS_CHECK
#endif // _LOGGING_H_

#define USE_SYSLOG true

#if USE_SYSLOG
#define VSYSLOG(verboselevel) SYSLOG_IF(INFO, VLOG_IS_ON(verboselevel))
#ifndef NDEBUG
#define DSYSLOG(severity) SYSLOG(severity)
#define DVSYSLOG(verboselevel) VSYSLOG(verboselevel)
#else // NDEBUG
#define DSYSLOG(severity) \
    true ? (void) 0 : @ac_google_namespace@::LogMessageVoidify() & SYSLOG(severity)
#define DVSYSLOG(verboselevel) \
    (true || !VLOG_IS_ON(verboselevel)) ?\
        (void) 0 : @ac_google_namespace@::LogMessageVoidify() & SYSLOG(INFO)
#endif // NDEBUG

#undef LOG
#define LOG SYSLOG
#undef VLOG
#define VLOG VSYSLOG
#undef DLOG
#define DLOG DSYSLOG
#undef DVLOG
#define DVLOG DVSYSLOG
#endif // USE_SYSLOG
