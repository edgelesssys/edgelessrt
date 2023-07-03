// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// clang-format off
#define CHK2(x) void* __##x##_chk(void* a, void* b) { return x(a, b); }
#define CHK3(x) void* __##x##_chk(void* a, void* b, void* c) { return x(a, b, c); }
#define CHK4(x) void* __##x##_chk(void* a, void* b, void* c, void* d) { return x(a, b, c, d); }
#define CHK5(x) void* __##x##_chk(void* a, void* b, void* c, void* d, void* e) { return x(a, b, c, d, e); }
// clang-format on

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"

CHK5(mbsnrtowcs)
CHK4(mbsrtowcs)
CHK3(memcpy)
CHK3(memmove)
CHK3(memset)
CHK3(poll)
CHK2(realpath)
CHK2(strcat)
CHK2(strcpy)
CHK3(strncat)
CHK3(strncpy)
CHK2(wcscat)
CHK2(wcscpy)
CHK3(wcsncat)
CHK3(wcsncpy)
CHK3(wmemcpy)
CHK3(wmemmove)
CHK3(wmemset)

#pragma GCC diagnostic pop
