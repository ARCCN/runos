/*

Copyright (c) 2017. RunSDN LTD. All rights reserved.

*/

#include "ofpda20match.hh"

namespace runos {

namespace ofdpa {

namespace of13 = fluid_msg::of13;
    

VRFid::VRFid() 
    : OXMTLV(of13::OFPXMC_EXPERIMENTER, OFDPA_OXM_VRF, false, 
        OFDPA_OXM_VRF_LEN) {
    create_oxm_req(0, 0, 0, 0);
}

VRFid::VRFid(uint16_t id)
    : OXMTLV(of13::OFPXMC_EXPERIMENTER, OFDPA_OXM_VRF, false,
        OFDPA_OXM_VRF_LEN),
    id_(id) {
    create_oxm_req(0, 0, 0, 0);
}
    
bool VRFid::equals(const OXMTLV &other) {
    if (const VRFid *field = dynamic_cast<const VRFid *>(&other)) {
        return ((OXMTLV::equals(other)) && (this->id_ == field->id_));
    }
    else {
        return false;
    }
}

of13::OXMTLV& VRFid::operator=(const OXMTLV& field) {    
    const VRFid& vrf = dynamic_cast<const VRFid&>(field);
    OXMTLV::operator=(field);
    id_ = vrf.id_;
    return *this;
}

size_t VRFid::pack(uint8_t *buf) {
    OXMTLV::pack(buf);
    uint16_t id = htons(id_);
    
    memcpy(buf + of13::OFP_OXM_HEADER_LEN, &OFDPA_EXPERIMENTER_ID, sizeof(OFDPA_EXPERIMENTER_ID));
    memcpy(buf + of13::OFP_OXM_HEADER_LEN + sizeof(OFDPA_EXPERIMENTER_ID), &id, sizeof(id));
    return 0;
}

fluid_msg::of_error VRFid::unpack(uint8_t *buf) {
    of13::OXMTLV::unpack(buf);
    id_ = ntohs(*((uint16_t *)(buf + OFDPA_OXM_VRF_LEN)));
    return 0;
}
  

AllowVlanTranslation::AllowVlanTranslation() 
    : OXMTLV(of13::OFPXMC_EXPERIMENTER, OFDPA_OXM_ALLOW_VLAN_TRANSLATION, false, 
        OFDPA_OXM_ALLOW_VLAN_TRANSLATION_LEN),
    allow(false){
    create_oxm_req(0, 0, 0, 0);
}

AllowVlanTranslation::AllowVlanTranslation(uint8_t allow)
    : OXMTLV(of13::OFPXMC_EXPERIMENTER, OFDPA_OXM_ALLOW_VLAN_TRANSLATION, false,
        OFDPA_OXM_ALLOW_VLAN_TRANSLATION_LEN),
    allow(allow) {
    create_oxm_req(0, 0, 0, 0);
}
    
bool AllowVlanTranslation::equals(const OXMTLV &other) {
    if (const AllowVlanTranslation *field = dynamic_cast<const AllowVlanTranslation*>(&other)) {
        return ((OXMTLV::equals(other)) && (this->allow == field->allow));
    }
    else {
        return false;
    }
}

of13::OXMTLV& AllowVlanTranslation::operator=(const OXMTLV& field) {    
    const AllowVlanTranslation& mode = dynamic_cast<const AllowVlanTranslation&>(field);
    OXMTLV::operator=(field);
    allow = mode.allow;
    return *this;
}

size_t AllowVlanTranslation::pack(uint8_t *buf) {
    OXMTLV::pack(buf);
    
    memcpy(buf + of13::OFP_OXM_HEADER_LEN, &OFDPA_EXPERIMENTER_ID, sizeof(OFDPA_EXPERIMENTER_ID ));
    memcpy(buf + of13::OFP_OXM_HEADER_LEN + sizeof(OFDPA_EXPERIMENTER_ID), &allow, sizeof(allow));
    return 0;
}

fluid_msg::of_error AllowVlanTranslation::unpack(uint8_t *buf) {
    of13::OXMTLV::unpack(buf);
    allow = *(buf + OFDPA_OXM_ALLOW_VLAN_TRANSLATION_LEN);
    return 0;
}
  
}//ofdpa

}//runos
