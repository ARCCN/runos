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

#include "OFTransaction.hh"

#include "SwitchConnection.hh"

OFTransaction::OFTransaction(uint32_t xid, QObject *parent)
    : QObject(parent), m_xid(xid)
{ }

void OFTransaction::request(SwitchConnectionPtr conn, OFMsg& msg)
{
    msg.xid(m_xid);
    conn->send(msg);
}
