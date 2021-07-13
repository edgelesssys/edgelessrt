// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

// Same as musl's newlocale.c, but with weak __newlocale. We override it in
// ertlibc/syscall.cpp.
#define __newlocale ___newlocale
#include "../3rdparty/musl/musl/src/locale/newlocale.c"
#undef __newlocale
weak_alias(___newlocale, __newlocale);
