#pragma once

#include <cstdint>
#include <arpa/inet.h>

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

// FIXME: is it correct code for big-endian platforms?
inline uint16_t lldp_tlv_header(int type, int length)
{
    return htons((type << 9) | length);
}
