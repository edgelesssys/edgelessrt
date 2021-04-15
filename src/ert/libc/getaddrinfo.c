// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/syscall/netdb.h>

int (*ert_getaddrinfo_func)(
    const char*,
    const char*,
    const struct oe_addrinfo*,
    struct oe_addrinfo**) = oe_getaddrinfo;

int getaddrinfo(
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo** res)
{
    return ert_getaddrinfo_func(node, service, hints, res);
}
