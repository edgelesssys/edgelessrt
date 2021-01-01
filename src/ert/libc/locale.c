// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <locale.h>

// will be overriden by ertlibc
__attribute__((__weak__)) locale_t __newlocale(
    int mask,
    const char* locale,
    locale_t loc)
{
    return newlocale(mask, locale, loc);
}

void __freelocale(locale_t loc)
{
    freelocale(loc);
}

void __uselocale(locale_t loc)
{
    uselocale(loc);
}
