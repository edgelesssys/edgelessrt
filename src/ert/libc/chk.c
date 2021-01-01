// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// clang-format off
#define CHK2(x) void* __##x##_chk(void* a, void* b) { return x(a, b); }
#define CHK3(x) void* __##x##_chk(void* a, void* b, void* c) { return x(a, b, c); }
// clang-format on

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"

CHK3(memcpy)
CHK3(memmove)
CHK3(memset)
CHK2(realpath)
CHK2(strcat)
CHK2(strcpy)
CHK3(strncat)
CHK3(strncpy)
CHK2(wcscat)
CHK2(wcscpy)
CHK3(wcsncat)
CHK3(wcsncpy)

#pragma GCC diagnostic pop
