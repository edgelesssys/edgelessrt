// Copyright (c) Edgeless Systems GmbH.
// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include "debug.h"
#include <openenclave/internal/safecrt.h>
#include <openenclave/internal/safemath.h>
#include <cstdlib>
#include <cstring>

// modified from _backtrace_symbols in host/sgx/ocalls/debug.c
char** ert_backtrace_symbols(
    const elf64_t* elf,
    uint64_t enclave_addr,
    const void* const* buffer,
    int size)
{
    char** ret = NULL;

    size_t malloc_size = 0;
    const char unknown[] = "<unknown>";
    char* ptr = NULL;

    if (!elf || !enclave_addr || !buffer || !size)
        goto done;

    /* Determine total memory requirements */
    {
        /* Calculate space for the array of string pointers */
        if (oe_safe_mul_sizet((size_t)size, sizeof(char*), &malloc_size) !=
            OE_OK)
            goto done;

        /* Calculate space for each string */
        for (int i = 0; i < size; i++)
        {
            const uint64_t vaddr = (uint64_t)buffer[i] - enclave_addr;
            const char* name = elf64_get_function_name(elf, vaddr);

            if (!name)
                name = unknown;

            if (oe_safe_add_sizet(malloc_size, strlen(name), &malloc_size) !=
                OE_OK)
                goto done;

            if (oe_safe_add_sizet(malloc_size, sizeof(char), &malloc_size) !=
                OE_OK)
                goto done;
        }
    }

    /* Allocate the array of string pointers, followed by the strings */
    if (!(ptr = (char*)malloc(malloc_size)))
        goto done;

    /* Set pointer to array of strings */
    ret = (char**)ptr;

    /* Skip over array of strings */
    ptr += (size_t)size * sizeof(char*);

    /* Copy strings into return buffer */
    for (int i = 0; i < size; i++)
    {
        const uint64_t vaddr = (uint64_t)buffer[i] - enclave_addr;
        const char* name = elf64_get_function_name(elf, vaddr);

        if (!name)
            name = unknown;

        size_t name_size = strlen(name) + sizeof(char);
        oe_memcpy_s(ptr, name_size, name, name_size);
        ret[i] = ptr;
        ptr += name_size;
    }

done:
    return ret;
}
