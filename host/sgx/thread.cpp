// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "thread.h"
#include <openenclave/host.h>
#include <openenclave/internal/sgx/enclave_thread_manager.h>
#include <openenclave/internal/trace.h>
#include <cassert>
#include <exception>
#include "platform_u.h"

using namespace std;
using namespace open_enclave;

static void _invoke_create_thread_ecall(oe_enclave_t* enclave) noexcept
{
    assert(enclave);
    oe_result_t ret = OE_FAILURE;
    if (oe_sgx_create_thread_ecall(enclave, &ret) != OE_OK || ret != OE_OK)
        abort();
}

extern "C" oe_result_t oe_sgx_create_thread_ocall(oe_enclave_t* enclave)
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
        return OE_FAILURE;
    }

    return OE_OK;
}

extern "C" oe_result_t oe_join_threads_created_inside_enclave(
    oe_enclave_t* enclave)
{
    assert(enclave);

    try
    {
        auto& thread_manager = host::EnclaveThreadManager::get_instance();

        OE_TRACE_INFO("joining threads");
        thread_manager.join_all_threads(enclave);
        OE_TRACE_INFO("finished joining threads");
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return OE_FAILURE;
    }

    return OE_OK;
}

extern "C" oe_result_t oe_cancel_threads_created_inside_enclave(
    oe_enclave_t* enclave)
{
    assert(enclave);

    try
    {
        auto& thread_manager = host::EnclaveThreadManager::get_instance();

        OE_TRACE_INFO("canceling threads");
        thread_manager.cancel_all_threads(enclave);
        OE_TRACE_INFO("finished canceling threads");
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return OE_FAILURE;
    }

    return OE_OK;
}
