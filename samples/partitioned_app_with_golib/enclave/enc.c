// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <stdlib.h>
#include "../golib/hello.h"
#include "helloworld_t.h"

void enclave_helloworld()
{
    // Go runtime needs epoll
    if (oe_load_module_host_epoll() != OE_OK)
        abort();

    // call the exported Go function
    myHello();
}
