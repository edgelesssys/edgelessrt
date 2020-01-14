// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <limits.h>
#include <locale.h>
#include <openenclave/enclave.h>
#include <stdio.h>
#include <stdlib.h>

long long int strtoll_l(const char* nptr, char** endptr, int base, locale_t loc)
{
    OE_UNUSED(loc);
    return strtoll(nptr, endptr, base);
}

unsigned long long strtoull_l(
    const char* nptr,
    char** endptr,
    int base,
    locale_t loc)
{
    OE_UNUSED(loc);
    return strtoull(nptr, endptr, base);
}

// http://refspecs.linux-foundation.org/LSB_4.0.0/LSB-Core-generic/LSB-Core-generic/libc---realpath-chk-1.html
// https://github.com/bminor/glibc/blob/master/debug/realpath_chk.c
char* __realpath_chk(const char* buf, char* resolved, size_t resolvedlen)
{
    if (resolvedlen < PATH_MAX)
    {
        //__chk_fail ();
        abort();
    }
    return realpath(buf, resolved);
}
