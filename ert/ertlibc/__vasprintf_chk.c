// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifdef __GNUC__

#define _GNU_SOURCE

#include <openenclave/enclave.h>
#include <stdio.h>

// GCC sometimes replaces vasprintf() calls with __vasprintf_chk() calls. In
// glibc this function sets the output stream's _IO_FLAGS2_FORTIFY flag, which
// causes glibc to perform various checks on the output stream. Since MUSL has
// no equivalent flag, this implementation simply calls vasprintf().
int __vasprintf_chk(char** s, int flag, const char* format, va_list ap)
{
    OE_UNUSED(flag);
    return vasprintf(s, format, ap);
}

#endif // __GNUC__
