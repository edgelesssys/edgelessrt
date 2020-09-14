// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef _OE_HOST_SGX_OCALLS_H
#define _OE_HOST_SGX_OCALLS_H

#include "../enclave.h"

void HandleThreadWait(oe_enclave_t* enclave, uint64_t arg);
OE_EXTERNC void HandleThreadWake(oe_enclave_t* enclave, uint64_t arg);
OE_EXTERNC char** oe_host_backtrace_symbols(
    const elf64_t* elf,
    uint64_t enclave_addr,
    const void* const* buffer,
    int size);

#endif /* _OE_HOST_SGX_OCALLS_H */
