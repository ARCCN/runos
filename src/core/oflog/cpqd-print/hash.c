/*
 * Copyright (c) 2008, 2009, 2010 Nicira Networks.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hash.h"
#include <string.h>

/* Returns the hash of the 'n' 32-bit words at 'p', starting from 'basis'.
 * 'p' must be properly aligned. */
uint32_t
hash_words(const uint32_t *p, size_t n, uint32_t basis)
{
    uint32_t a, b, c;

    a = b = c = 0xdeadbeef + (((uint32_t) n) << 2) + basis;

    while (n > 3) {
        a += p[0];
        b += p[1];
        c += p[2];
        HASH_MIX(a, b, c);
        n -= 3;
        p += 3;
    }

    switch (n) {
    case 3:
        c += p[2];
        /* fall through */
    case 2:
        b += p[1];
        /* fall through */
    case 1:
        a += p[0];
        HASH_FINAL(a, b, c);
        /* fall through */
    case 0:
        break;
    }
    return c;
}

/* Returns the hash of 'a', 'b', and 'c'. */
uint32_t
hash_3words(uint32_t a, uint32_t b, uint32_t c)
{
    a += 0xdeadbeef;
    b += 0xdeadbeef;
    c += 0xdeadbeef;
    HASH_FINAL(a, b, c);
    return c;
}

/* Returns the hash of 'a' and 'b'. */
uint32_t
hash_2words(uint32_t a, uint32_t b)
{
    return hash_3words(a, b, 0);
}

/* Returns the hash of the 'n' bytes at 'p', starting from 'basis'. */
uint32_t
hash_bytes(const void *p_, size_t n, uint32_t basis)
{
    const uint8_t *p = p_;
    uint32_t a, b, c;
    uint32_t tmp[3];

    a = b = c = 0xdeadbeef + n + basis;

    while (n >= sizeof tmp) {
        memcpy(tmp, p, sizeof tmp);
        a += tmp[0];
        b += tmp[1];
        c += tmp[2];
        HASH_MIX(a, b, c);
        n -= sizeof tmp;
        p += sizeof tmp;
    }

    if (n) {
        tmp[0] = tmp[1] = tmp[2] = 0;
        memcpy(tmp, p, n);
        a += tmp[0];
        b += tmp[1];
        c += tmp[2];
        HASH_FINAL(a, b, c);
    }

    return c;
}
