// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/sgx/enclave_thread_manager.h>
#include <openenclave/internal/trace.h>
#include <cassert>
#include <exception>
#include "ertlibc_u.h"

using namespace std;
using namespace open_enclave;

static void _invoke_create_thread_ecall(oe_enclave_t* enclave) noexcept
{
    assert(enclave);
    if (ert_create_thread_ecall(enclave) != OE_OK)
        abort();
}

bool ert_create_thread_ocall(oe_enclave_t* enclave)
{
    assert(enclave);

    try
    {
        host::EnclaveThreadManager::get_instance().create_thread(
            enclave, _invoke_create_thread_ecall);
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return false;
    }

    return true;
}
