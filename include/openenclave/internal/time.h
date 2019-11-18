// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef _OE_INCLUDE_TIME_H
#define _OE_INCLUDE_TIME_H

#include <openenclave/bits/types.h>
#include <openenclave/corelibc/time.h>

OE_EXTERNC_BEGIN

/*
**==============================================================================
**
** oe_get_time()
**
**     Return milliseconds elapsed since the Epoch or (uint64_t)-1 on error.
**
**     The Epoch is defined as: 1970-01-01 00:00:00 +0000 (UTC)
**
**==============================================================================
*/

uint64_t oe_get_time(void);

/*
**==============================================================================
**
** oe_clock_gettime()
**
**     Get the time of the specified clock. Like clock_gettime.
**
**==============================================================================
*/
int oe_clock_gettime(int clk_id, struct oe_timespec* tp);

OE_EXTERNC_END

#endif /* _OE_INCLUDE_TIME_H */
