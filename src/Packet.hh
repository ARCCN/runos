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

#include <type_traits>
#include <vector>
#include "Common.hh"
#include "OXMTLVUnion.hh"

/**
 * Wraps a packet received from the switch and allows
 * to read and modify OXM fields on it.
 */
class Packet {
public:
    explicit Packet(of13::PacketIn& pi);
    ~Packet();

    /*
     * Extracts OXM field.
     * @param field OXM field to read.
     * @return OXM TLV with corresponding field value. Delete it after use.
     */
    of13::OXMTLV* read(uint8_t field);

    /*
     * Extracts OXM field into OXMTLVUnion.
     */
    void read(OXMTLVUnion& tlv);

    /*
     * Extracts OXM field into OXMTLV.
     */
    void read(of13::OXMTLV& tlv);

    /*
     * Raw packet data clipped to miss_send_len.
     */
    std::vector<uint8_t> serialize() const;

    //@{
    /*
     * Similar to read(uint8_t) but faster and easier to use.
     */
    // TODO: maybe add switch here ?
    // OpenFlow
    uint32_t          readInPort();
    uint32_t          readInPhyPort();
    uint64_t          readMetadata();
    // Ethernet
    EthAddress        readEthSrc();
    EthAddress        readEthDst();
    uint16_t          readEthType();
    // ARP
    uint16_t          readARPOp();
    IPAddress         readARPSPA();
    IPAddress         readARPTPA();
    EthAddress        readARPSHA();
    EthAddress        readARPTHA();
    // IP
    IPAddress         readIPv4Src();
    IPAddress         readIPv4Dst();
    uint8_t           readIPDSCP();
    uint8_t           readIPECN();
    uint8_t           readIPProto();
    // TODO: fill all possible OXM fields implemented by fluid_msg
    //@

private:
    struct PacketImpl* m;
};

