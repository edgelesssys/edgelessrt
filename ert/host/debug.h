// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/elf.h>

OE_EXTERNC char** ert_backtrace_symbols(
    const elf64_t* elf,
    uint64_t enclave_addr,
    const void* const* buffer,
    int size);
