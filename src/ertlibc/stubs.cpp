// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert_stubs.h>
#include <openenclave/internal/trace.h>

void ert_stub_trace(const char* msg)
{
    OE_TRACE_ERROR("not implemented: %s", msg);
}
