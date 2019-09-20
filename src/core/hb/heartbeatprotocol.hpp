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

#include "../lib/better_enum.hpp"

#include "runos/core/logging.hpp"
#include <QDataStream>
#include <QtNetwork/QHostAddress>
#include <QTime>

BETTER_ENUM(CommunicationType,
            int,
            UNDEFINED,
            UNICAST,
            MULTICAST,
            BROADCAST)

BETTER_ENUM(ControllerState,
            int,
            NOT_ACTIVE,
            ACTIVE)

BETTER_ENUM(ControllerStatus,
            int,
            UNDEFINED,
            BACKUP,
            PRIMARY,
            RECOVERY)

BETTER_ENUM(HeartbeatCommand,
            int,
            BEGIN,
            HEARTBEAT_ECHO_REQUEST,
            HEARTBEAT_ECHO_REPLY,
            HEARTBEAT_PARAMETERS_UPDATE,
            END)

using Connection = QPair<QHostAddress, quint16>;

struct EchoMessage
{
    qint32 unique_node_id;
    QTime hb_start_time;
    qint64 message_number;

    friend QDataStream& operator<<(QDataStream& s, const EchoMessage& c)
    {
        return s << c.unique_node_id << c.hb_start_time << c.message_number;
    }

    friend QDataStream& operator>>(QDataStream& s, EchoMessage& c)
    {
        return s >> c.unique_node_id >> c.hb_start_time >> c.message_number;
    }
};

struct ParamsMessage
{
    qint32 unique_node_id;
    Connection heartbeat_connection_params;
    Connection openflow_connection_params;
    Connection db_connection_params;

    friend QDataStream& operator<<(QDataStream& s, const ParamsMessage& c)
    {
        return s << c.unique_node_id << c.heartbeat_connection_params
                 << c.openflow_connection_params << c.db_connection_params;
    }

    friend QDataStream& operator>>(QDataStream& s, ParamsMessage& c)
    {
        return s >> c.unique_node_id >> c.heartbeat_connection_params
                 >> c.openflow_connection_params >> c.db_connection_params;
    }
};
