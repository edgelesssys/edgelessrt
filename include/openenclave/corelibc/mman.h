// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/defs.h>
#include <openenclave/corelibc/bits/types.h>

#define OE_MAP_FAILED ((void*)-1)
#define OE_MAP_PRIVATE 0x02
#define OE_MAP_FIXED 0x10
#define OE_MAP_ANON 0x20

#define OE_PROT_READ 1
#define OE_PROT_WRITE 2
#define OE_PROT_EXEC 4

OE_EXTERNC_BEGIN

void* oe_mmap(
    void* addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    oe_off_t offset);

int oe_munmap(void* addr, size_t length);

#ifdef OE_NEED_STDC_NAMES

#define MAP_FAILED OE_MAP_FAILED
#define MAP_PRIVATE OE_MAP_PRIVATE
#define MAP_FIXED OE_MAP_FIXED
#define MAP_ANON OE_MAP_ANON

#define PROT_READ OE_PROT_READ
#define PROT_WRITE OE_PROT_WRITE
#define PROT_EXEC OE_PROT_EXEC

void* mmap(
    void* addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    oe_off_t offset);

int munmap(void* addr, size_t length);

#endif // OE_NEED_STDC_NAMES

OE_EXTERNC_END
