// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <assert.h>
#include <dlfcn.h>
#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/trace.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

/*
Here we implement some dynlink functions. However, we do not actually implement
dynamic linking. If an app tries to load a shared object, we pretend success and
try to find all symbols in the statically linked enclave.
*/

using namespace std;
using namespace ert;

extern "C" const uint64_t _payload_reloc_rva;
extern "C" const uint64_t _payload_reloc_size;
extern "C" void __dl_seterr(const char*, ...);

void* dlopen(const char* file, int mode)
{
    (void)file;
    (void)mode;
    return (void*)1; // just return any non-NULL value
}

// copied from musl/ldso/dynlink.c
static uint32_t gnu_hash(const char* s0)
{
    const unsigned char* s = reinterpret_cast<const unsigned char*>(s0);
    uint_fast32_t h = 5381;
    for (; *s; s++)
        h += h * 32 + *s;
    return (uint32_t)h;
}

// adapted from musl/ldso/dynlink.c
static const Elf64_Sym* gnu_lookup(
    uint32_t h1,
    const uint32_t* hashtab,
    const char* strings,
    const Elf64_Sym* syms,
    const char* s)
{
    uint32_t nbuckets = hashtab[0];
    const uint32_t* buckets = hashtab + 4 + hashtab[2] * (sizeof(size_t) / 4);
    uint32_t i = buckets[h1 % nbuckets];

    if (!i)
        return 0;

    const uint32_t* hashval = buckets + nbuckets + (i - hashtab[1]);

    for (h1 |= 1;; i++)
    {
        uint32_t h2 = *hashval++;
        if ((h1 == (h2 | 1)) && !strcmp(s, strings + syms[i].st_name))
            return syms + i;
        if (h2 & 1)
            break;
    }

    return 0;
}

extern "C" const void* __dlsym(void* handle, const char* symbol)
{
    (void)handle;

    if (!symbol)
        abort();

    const auto base =
        static_cast<const uint8_t*>(__oe_get_enclave_start_address());
    assert(base);
    const auto ehdr =
        static_cast<const Elf64_Ehdr*>(__oe_get_enclave_elf_header());
    assert(ehdr);
    assert(ehdr->e_phoff);
    const Elf64_Phdr* const phdr = (Elf64_Phdr*)(base + ehdr->e_phoff);

    // find address of dynamic section in program header
    const Elf64_Dyn* dyn = NULL;
    for (int i = 0; i < ehdr->e_phnum; ++i)
        if (phdr[i].p_type == PT_DYNAMIC)
        {
            assert(phdr[i].p_vaddr);
            dyn = (Elf64_Dyn*)(base + phdr[i].p_vaddr);
            break;
        }
    assert(dyn);

    const char* strtab = NULL;
    const Elf64_Sym* symtab = NULL;
    const uint32_t* hashtab = NULL;

    // find addresses of string, symbol, and symbol hash table in dynamic
    // section
    for (; dyn->d_tag != DT_NULL; ++dyn)
        switch (dyn->d_tag)
        {
            case DT_STRTAB:
                strtab = (char*)(base + dyn->d_un.d_ptr);
                break;
            case DT_SYMTAB:
                symtab = (Elf64_Sym*)(base + dyn->d_un.d_ptr);
                break;
            case DT_GNU_HASH:
                hashtab = (uint32_t*)(base + dyn->d_un.d_ptr);
                break;
        }

    assert(strtab);
    assert(symtab);
    assert(hashtab);

    // look up symbol
    const Elf64_Sym* const sym =
        gnu_lookup(gnu_hash(symbol), hashtab, strtab, symtab, symbol);

    if (sym && sym->st_value)
        return base + sym->st_value;

    OE_TRACE_WARNING("Symbol not found: %s", symbol);
    __dl_seterr("Symbol not found: %s", symbol);

    return NULL;
}
