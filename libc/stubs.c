// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <assert.h>
#include <errno.h>
#include <openenclave/enclave_stubs.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/trace.h>
#include <string.h>
#include <wchar.h>

int __setxid(void)
{
    return 0; // noop, return success
}

void oe_stub_trace(const char* msg)
{
    OE_TRACE_ERROR("not implemented: %s", msg);
}

// clang-format off
#define CHK2(x) void* __##x##_chk(void* a, void* b) { return x(a, b); }
#define CHK3(x) void* __##x##_chk(void* a, void* b, void* c) { return x(a, b, c); }
// clang-format on

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"

CHK3(memcpy)
CHK3(memmove)
CHK3(memset)
CHK2(strcat)
CHK2(strcpy)
CHK3(strncpy)
CHK2(wcscat)
CHK2(wcscpy)
CHK3(wcsncpy)

#pragma GCC diagnostic pop

int pthread_attr_getstacksize(const void* attr, size_t* stacksize)
{
    assert(attr);
    assert(stacksize);
    *stacksize = oe_get_stack_size();
    return 0;
}

int pthread_sigmask()
{
    errno = ENOSYS;
    return -1;
}
