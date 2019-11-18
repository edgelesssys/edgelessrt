// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/bits/types.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/time.h>
#include "core_t.h"

uint64_t oe_get_time(void)
{
    uint64_t ret = (uint64_t)-1;

    if (oe_ocall(OE_OCALL_GET_TIME, 0, &ret) != OE_OK)
    {
        ret = (uint32_t)-1;
        goto done;
    }

done:

    return ret;
}

int oe_clock_gettime(int clk_id, struct oe_timespec* tp)
{
    int ret = -1;
    if (oe_clock_gettime_ocall(&ret, clk_id, tp) != OE_OK)
        return -1;
    return ret;
}

OE_WEAK oe_result_t
oe_clock_gettime_ocall(int* retval, int clk_id, struct oe_timespec* tp)
{
    (void)retval;
    (void)clk_id;
    (void)tp;
    return OE_UNSUPPORTED;
}
