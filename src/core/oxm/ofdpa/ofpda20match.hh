/*

Copyright (c) 2017. RunSDN LTD. All rights reserved.

*/

#pragma once

#include "../../openflow/ofdpa-common.hh"

#include <fluid/ofcommon/openflow-common.hh>
#include <fluid/of13/of13match.hh>

#include <cstdint>

namespace runos {

namespace ofdpa {

namespace of13 = fluid_msg::of13;
    
class VRFid: public of13::OXMTLV {
private:
    uint16_t id_;
public:
    VRFid();
    VRFid(uint16_t value);
    ~VRFid() {
    }
    virtual bool equals(const OXMTLV & other);
    OXMTLV& operator=(const OXMTLV& field);
    virtual VRFid* clone() const {
        return new VRFid(*this);
    }
    size_t pack(uint8_t *buffer);
    fluid_msg::of_error unpack(uint8_t *buffer);
    uint32_t value() const {
        return this->id_;
    }
    void value(uint16_t id) {
        this->id_ = id;
    }
};

class AllowVlanTranslation: public of13::OXMTLV {
private:
    uint8_t allow;
public:
    AllowVlanTranslation();
    AllowVlanTranslation(uint8_t allow);
    ~AllowVlanTranslation() {
    }
    virtual bool equals(const OXMTLV & other);
    OXMTLV& operator=(const OXMTLV& field);
    virtual AllowVlanTranslation* clone() const {
        return new AllowVlanTranslation(*this);
    }
    size_t pack(uint8_t *buffer);
    fluid_msg::of_error unpack(uint8_t *buffer);
    uint8_t value() const {
        return this->allow;
    }
    void value(uint8_t _allow) {
        this->allow = _allow;
    }
};

class OFDPATLV : public fluid_msg::of13::OXMTLV {
public:
    OFDPATLV(uint8_t field, bool mask, uint16_t length):
        fluid_msg::of13::OXMTLV(
                fluid_msg::of13::OFPXMC_EXPERIMENTER,
                field,
                mask,
                length + sizeof(uint32_t))
    { }

    size_t pack(uint8_t* buf) {
        uint32_t tmp = OFDPA_OXM_EXPERIMENTER_ID;
        OXMTLV::pack(buf);
        buf[4] = (tmp >> 24) & 0xff;
        buf[5] = (tmp >> 16) & 0xff;
        buf[6] = (tmp >> 8)  & 0xff;
        buf[7] = (tmp >> 0)  & 0xff;
        return 8;
    }
};

class TrafficClass : public OFDPATLV {
public:
    TrafficClass(uint8_t value = 0):
        OFDPATLV(OFDPA_OXM_TRAFFIC_CLASS, 0, sizeof(uint8_t)), value_(value)
    {
        create_oxm_req(0, 0, 0, 0);
    }

    size_t pack(uint8_t *buf) {
        buf += OFDPATLV::pack(buf);
        buf[0] = value_;
        return 0;
    }

    OXMTLV* clone(void) const {
        return new TrafficClass(value_);
    }

private:
    uint8_t value_;
};

class LmepId : public OFDPATLV {
public:
    LmepId(uint32_t id = 0):
        OFDPATLV(OFDPA_OXM_LMEP_ID, 0, sizeof(id_)),
        id_(id)
    { create_oxm_req(0, 0, 0, 0); }

    size_t pack(uint8_t *buf) {
        buf += OFDPATLV::pack(buf);
        buf[0] = (id_ >> 24) & 0xff;
        buf[1] = (id_ >> 16) & 0xff;
        buf[2] = (id_ >>  8) & 0xff;
        buf[3] = (id_ >>  0) & 0xff;
        return 0;
    }

    virtual OXMTLV* clone(void) const {
        return new LmepId(id_);
    }

private:
    uint32_t id_;
};

class QosIndex : public OFDPATLV {
public:
    QosIndex(uint8_t index = 0):
        OFDPATLV(OFDPA_OXM_QOS_INDEX, 0, sizeof(uint8_t)),
        value_(index)
    { create_oxm_req(0, 0, 0, 0); }

    virtual size_t pack(uint8_t *buf) {
        buf += OFDPATLV::pack(buf);
        buf[0] = value_;
        return 0;
    }

    virtual OXMTLV* clone(void) const {
        return new QosIndex(value_);
    }

private:
    uint8_t value_;
};

class VLANDei : public OFDPATLV {
public:
    VLANDei(bool val = false):
        OFDPATLV(OFDPA_OXM_DEI, 0, sizeof(uint8_t)),
        value_(val)
    { create_oxm_req(0, 0, 0, 0); }

    virtual size_t pack(uint8_t *buf) {
        buf += OFDPATLV::pack(buf);
        buf[0] = (value_) ? 1 : 0;
        return 0;
    }

    virtual OXMTLV* clone(void) const {
        return new VLANDei(value_);
    }
private:
    bool value_;
};


}//ofdpa

}//runos

