/* Copyright (c) 2008, 2009 The Board of Trustees of The Leland Stanford
 * Junior University
 * 
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */
#ifndef FLOW_H
#define FLOW_H 1

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "hash.h"
#include "util.h"

struct ofp_match;
struct ofpbuf;

/* Identification data for a flow.
   All fields are in network byte order.
   In decreasing order by size, so that flow structures can be hashed or
   compared bytewise. */
struct flow {
    uint32_t nw_src;            /* IP source address. */
    uint32_t nw_dst;            /* IP destination address. */
    uint32_t in_port;           /* Input switch port. */
    uint16_t dl_vlan;           /* Input VLAN id. */
    uint16_t dl_type;           /* Ethernet frame type. */
    uint16_t tp_src;            /* TCP/UDP source port. */
    uint16_t tp_dst;            /* TCP/UDP destination port. */
    uint8_t dl_src[6];          /* Ethernet source address. */
    uint8_t dl_dst[6];          /* Ethernet destination address. */
    uint8_t dl_vlan_pcp;        /* Input VLAN priority. */
    uint8_t nw_tos;             /* IPv4 DSCP. */
    uint8_t nw_proto;           /* IP protocol. */
    uint8_t pad[5];
};
BUILD_ASSERT_DECL(sizeof(struct flow) == 40);

int flow_extract(struct ofpbuf *, uint32_t in_port, struct flow *);

void flow_print(FILE *, const struct flow *);
static inline int flow_compare(const struct flow *, const struct flow *);
static inline bool flow_equal(const struct flow *, const struct flow *);
static inline size_t flow_hash(const struct flow *, uint32_t basis);

static inline int
flow_compare(const struct flow *a, const struct flow *b)
{
    return memcmp(a, b, sizeof *a);
}

static inline bool
flow_equal(const struct flow *a, const struct flow *b)
{
    return !flow_compare(a, b);
}

static inline size_t
flow_hash(const struct flow *flow, uint32_t basis)
{
    BUILD_ASSERT_DECL(!(sizeof *flow % sizeof(uint32_t)));
    return hash_words((const uint32_t *) flow,
                      sizeof *flow / sizeof(uint32_t), basis);
}

#endif /* flow.h */
