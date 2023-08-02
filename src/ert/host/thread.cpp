// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "thread.h"
#include <openenclave/host.h>
#include <openenclave/internal/trace.h>
#include <cassert>
#include <exception>
#include "enclave_thread_manager.h"
#include "platform_u.h"

using namespace std;
using namespace ert;

extern "C" void ert_set_cancelable(bool cancelable)
{
    try
    {
        host::EnclaveThreadManager::get_instance().set_cancelable(cancelable);
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
    }
}

extern "C" oe_result_t ert_join_threads_created_inside_enclave(
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

extern "C" oe_result_t ert_cancel_threads_created_inside_enclave(
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
