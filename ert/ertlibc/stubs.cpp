// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/enclave_stubs.h>
#include <openenclave/internal/trace.h>

extern "C" void oe_stub_trace(const char* msg)
{
    OE_TRACE_ERROR("not implemented: %s", msg);
}

extern "C" int __res_init()
{
    return 0; // success
}
