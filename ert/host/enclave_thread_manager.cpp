// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "enclave_thread_manager.h"
#include <openenclave/internal/trace.h>
#include <pthread.h>
#include <cassert>
#include "../host/hostthread.h"
#include "../host/sgx/enclave.h"
#include "../host/sgx/ocalls/ocalls.h"

using namespace std;

namespace ert::host
{
EnclaveThreadManager::~EnclaveThreadManager()
{
    // Process is exiting. If any enclave has not been terminated, detach its
    // lingering threads (if any) so that the process can exit without an error.
    for (auto& enclaveAndThreads : threads_)
        for (auto& t : enclaveAndThreads.second)
            t.thread.detach();
}

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

    Thread* new_thread;

    {
        const lock_guard lock(mutex_);
        auto& threads = threads_[enclave];

        // remove all threads that have already finished
        threads.remove_if([](Thread& t) {
            if (!t.finished)
                return false;
            t.thread.join();
            return true;
        });

        new_thread = &threads.emplace_back();
    }

    new_thread->thread = thread([=, &finished = new_thread->finished] {
        try
        {
            func(enclave);
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
        }
        finished = true;
    });
}

void EnclaveThreadManager::join_all_threads(const oe_enclave_t* enclave)
{
    assert(enclave);
    const lock_guard lock(mutex_);
    auto& threads = threads_[enclave];
    for (auto& t : threads)
        t.thread.join();
    threads.clear();
}

// wake a thread that is waiting in HandleThreadWait
static void _wake_thread(oe_enclave_t& enclave, oe_thread_t thread) noexcept
{
    uint64_t tcs = 0;

    // get tcs value for *thread*
    oe_mutex_lock(&enclave.lock);
    for (size_t i = 0; i < enclave.num_bindings; ++i)
    {
        const oe_thread_binding_t& binding = enclave.bindings[i];
        if (binding.thread == thread)
        {
            tcs = binding.tcs;
            break;
        }
    }
    oe_mutex_unlock(&enclave.lock);

    if (tcs)
        HandleThreadWake(&enclave, tcs);
}

// Try to cancel all threads of *enclave*. If any thread is spinning inside the
// enclave or doing work that does not require occasional ocalls, this method
// will block. This is not a typical behavior of "lingering" threads (like those
// created by the Go runtime), so we will consider terminating an enclave that
// has actively working threads a bug in the application.
void EnclaveThreadManager::cancel_all_threads(oe_enclave_t* enclave)
{
    assert(enclave);
    const lock_guard lock(mutex_);
    auto& threads = threads_[enclave];

    for (auto& t : threads)
    {
        if (!t.finished)
        {
            const auto handle = t.thread.native_handle();
            if (pthread_cancel(handle) == 0)
                // Thread may be waiting in HandleThreadWait which is not a
                // cancellation point, so wake the thread.
                _wake_thread(*enclave, handle);
        }
        t.thread.join();
    }

    threads.clear();
}
} // namespace ert::host
