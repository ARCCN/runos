/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

/** @file Common.h
  * @brief Common headers and global definitions.
  */
#pragma once

#include <memory>
#include <glog/logging.h>
#include <QtCore>
#include <fluid/of13msg.hh>
using namespace fluid_msg;

Q_DECLARE_METATYPE(uint32_t)
Q_DECLARE_METATYPE(uint64_t)
Q_DECLARE_METATYPE(of13::FeaturesReply)
Q_DECLARE_METATYPE(of13::FlowRemoved)
Q_DECLARE_METATYPE(of13::PortStatus)
Q_DECLARE_METATYPE(of13::Port)
Q_DECLARE_METATYPE(of13::Match)
Q_DECLARE_METATYPE(std::shared_ptr<of13::Error>)
Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)
