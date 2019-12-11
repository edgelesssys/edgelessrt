// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/host.h>

OE_EXTERNC void oe_trace_ocall(oe_enclave_t* enclave, const void* func);
