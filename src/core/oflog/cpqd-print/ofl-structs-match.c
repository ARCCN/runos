/* Copyright (c) 2011, CPqD, Brazil
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Ericsson Research nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include "ofl-structs.h"
#include "hash.h"
#include "oxm-match.h"

void
ofl_structs_match_init(struct ofl_match *match){

    match->header.type = OFPMT_OXM;
    match->header.length = 0;
    match->match_fields = (struct hmap) HMAP_INITIALIZER(&match->match_fields);
}


void
ofl_structs_match_put8(struct ofl_match *match, uint32_t header, uint8_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint8_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}

void
ofl_structs_match_put8m(struct ofl_match *match, uint32_t header, uint8_t value, uint8_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint8_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put16(struct ofl_match *match, uint32_t header, uint16_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint16_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}


void
ofl_structs_match_put16m(struct ofl_match *match, uint32_t header, uint16_t value, uint16_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint16_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put32(struct ofl_match *match, uint32_t header, uint32_t value){
    struct ofl_match_tlv *m = xmalloc(sizeof (struct ofl_match_tlv));

    int len = sizeof(uint32_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put32m(struct ofl_match *match, uint32_t header, uint32_t value, uint32_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint32_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put64(struct ofl_match *match, uint32_t header, uint64_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint64_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put64m(struct ofl_match *match, uint32_t header, uint64_t value, uint64_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint64_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put_pbb_isid(struct ofl_match *match, uint32_t header, uint8_t value[PBB_ISID_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = OXM_LENGTH(header);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}


void
ofl_structs_match_put_pbb_isidm(struct ofl_match *match, uint32_t header, uint8_t value[PBB_ISID_LEN], uint8_t mask[PBB_ISID_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = OXM_LENGTH(header);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put_eth(struct ofl_match *match, uint32_t header, uint8_t value[ETH_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = ETH_ADDR_LEN;

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put_eth_m(struct ofl_match *match, uint32_t header, uint8_t value[ETH_ADDR_LEN], uint8_t mask[ETH_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = ETH_ADDR_LEN;

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put_ipv6(struct ofl_match *match, uint32_t header, uint8_t value[IPv6_ADDR_LEN]){

    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = IPv6_ADDR_LEN;

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put_ipv6m(struct ofl_match *match, uint32_t header, uint8_t value[IPv6_ADDR_LEN], uint8_t mask[IPv6_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = IPv6_ADDR_LEN;

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

