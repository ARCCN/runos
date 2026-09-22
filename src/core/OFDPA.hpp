/*

Copyright (c) 2017. RunSDN LTD. All rights reserved.

*/

#pragma once

#include <fluid/of13msg.hh>

namespace runos {

namespace ofdpa {
    
enum TableId {
    Ingress_Port = 0,
    Port_DSCP_Trust = 5,
    Port_PCPT_Trust = 6,
    Tunnel_DSCP_Trust = 7,
    Tunnel_PCP_Trust = 8,
    Injected_OAM = 9,
    VLAN = 10,
    VLAN1 = 11,
    Ingress_Maintenance_Point = 12,
    MPLS_L2_Port = 13,
    MPLS_L2_Port_DSCP_Trust = 15,
    MPLS_L2_Port_PCP_Trust = 16,
    L2_Policer = 18,
    L2_Policer_Actions = 19,
    Termination_MAC = 20,
    L3_Type = 21,
    MPLS0 = 23,
    MPLS1 = 24,
    MPLS2 = 25,
    MPLS_TP_Maintenance_Point = 26,
    MPLS_L3_Type = 27,
    MPLS_Label_Trust = 28,
    MPLS_Type = 29,
    Unicast_Routing = 30,
    Multicast_Routing = 40,
    Bridging = 50,
    Policy_ACL = 60,
    Color_Based_Actions = 65,
    Egress_VLAN = 210,
    Egress_VLAN1 = 211,
    Egress_Maintenance_Point = 226,
    Egress_DSCP_PCP_Remark = 230,
    Egress_TPID = 235
};

}
}

