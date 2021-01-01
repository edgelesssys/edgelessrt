// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

OE_EXTERNC oe_result_t
ert_cancel_threads_created_inside_enclave(oe_enclave_t* enclave);
