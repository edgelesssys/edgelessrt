// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/sgx/enclave_thread_manager.h>
#include <openenclave/internal/trace.h>
#include <cassert>
#include <thread>

using namespace std;

namespace open_enclave::host
{
EnclaveThreadManager& EnclaveThreadManager::get_instance()
{
    static EnclaveThreadManager instance;
    return instance;
}

void EnclaveThreadManager::create_thread(
    oe_enclave_t* enclave,
    void (*func)(oe_enclave_t*))
{
    assert(enclave);
    assert(func);

    {
        const lock_guard lock(mutex_);
        ++thread_count_[enclave];
    }

    thread([=] {
        try
        {
            func(enclave);
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
        }

        const lock_guard lock(mutex_);
        --thread_count_[enclave];
        cv_.notify_one();
    }).detach();
}

void EnclaveThreadManager::join_all_threads(const oe_enclave_t* enclave)
{
    assert(enclave);
    unique_lock lock(mutex_);
    int& count = thread_count_[enclave];
    cv_.wait(lock, [&count] { return count == 0; });
}
} // namespace open_enclave::host
