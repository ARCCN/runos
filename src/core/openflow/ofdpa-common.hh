/*

Copyright (c) 2017. RunSDN LTD. All rights reserved.

*/

#pragma once

#include <fluid/of13/openflow-13.h>
#include <cstdint>
namespace runos {

namespace ofdpa {

const uint32_t OFDPA_EXPERIMENTER_ID = (0x10 << 16) + 0x18; 

const uint8_t OFDPA_OXM_VRF_LEN = 6;
const uint8_t OFDPA_OXM_ALLOW_VLAN_TRANSLATION_LEN = 5;

/* OF-DPA Experimenter Match Field types */
enum ofdpa_match_exp_fields {
    OFDPA_OXM_VRF = 1,
    OFDPA_OXM_TRAFFIC_CLASS = 2,
    OFDPA_OXM_COLOR = 3,
    OFDPA_OXM_DEI = 4,
    OFDPA_OXM_QOS_INDEX = 5,
    OFDPA_OXM_LMEP_ID = 6,
    OFDPA_OXM_MPLS_TTL = 7,
    OFDPA_OXM_MPLS_L2_Port = 8,
    OFDPA_OXM_L3_IN_PORT = 9,
    OFDPA_OXM_OVID = 10,
    OFDPA_OXM_MPLS_DATA_FIRST_NIBBLE = 11,
    OFDPA_OXM_MPLS_ACH_CHANNEL = 12,
    OFDPA_OXM_MPLS_NEXT_LABEL_IS_GAL = 13,
    OFDPA_OXM_OAM_Y1731_MDL = 14,
    OFDPA_OXM_OAM_Y1731_OPCODE = 15,
    OFDPA_OXM_COLOR_ACTIONS_INDEX = 16,
    OFDPA_OXM_PROTECTION_INDEX = 21,
    OFDPA_OXM_ETH_SUB_TYPE = 22,
    OFDPA_OXM_MPLS_TYPE = 23,
    OFDPA_OXM_ALLOW_VLAN_TRANSLATION = 24,
    OFDPA_OXM_NUM = 25,

    OFDPA_OXM_HEADER_LEN    = fluid_msg::of13::OFP_OXM_HEADER_LEN + 4,
    OFDPA_OXM_EXPERIMENTER_ID = 0x001018
};


struct ofdpa_oxm_match_field {
    uint32_t oxm_header; /* oxm_class = OFPXMC_EXPERIMENTER */
    uint32_t experimenter; /* Experimenter ID: 00-00-10-18 */
};
}

}//runos


