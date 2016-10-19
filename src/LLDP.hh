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

#pragma once

#define LLDP_ETH_TYPE 0x88cc

/*
 * LLDP TLV type codes
 */

#define LLDP_END_TLV                    0
#define LLDP_CHASSIS_ID_TLV             1
#define LLDP_PORT_ID_TLV                2
#define LLDP_TTL_TLV                    3
#define LLDP_SYSTEM_DESCR_TLV           6
/*
 * LLDP TLV sub-type codes
 */

#define LLDP_CHASSIS_ID_SUB_MAC         4

#define LLDP_PORT_ID_SUB_ALIAS          1
#define LLDP_PORT_ID_SUB_COMPONENT      2
#define LLDP_PORT_ID_SUB_MAC            3
#define LLDP_PORT_ID_SUB_ADDRESS        4
#define LLDP_PORT_ID_SUB_NAME           5
#define LLDP_PORT_ID_SUB_CIRCUIT        6
#define LLDP_PORT_ID_SUB_LOCAL          7
