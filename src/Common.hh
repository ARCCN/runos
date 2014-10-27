/** @file Common.h
  * @brief Common headers and global definitions.
  */
#pragma once

#include <memory>
#include <glog/logging.h>
#include <QtCore>
#include <fluid/OFConnection.hh>
#include <fluid/of13msg.hh>
using namespace fluid_base;
using namespace fluid_msg;

// Implement missing macro
#ifndef NDEBUG
#define DLOG_FIRST_N(severity, n) LOG_FIRST_N(severity, n)
#else
#define DLOG_FIRST_N(severity, n)
    true ? (void) 0 : google::LogMessageVoidify() & LOG(severity)
#endif

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::static_pointer_cast;
using std::dynamic_pointer_cast;
using std::const_pointer_cast;
