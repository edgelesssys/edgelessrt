// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/host.h>
#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <thread>

namespace ert::host
{
class EnclaveThreadManager final
{
  public:
    EnclaveThreadManager(const EnclaveThreadManager&) = delete;
    EnclaveThreadManager& operator=(const EnclaveThreadManager&) = delete;
    ~EnclaveThreadManager();

    static EnclaveThreadManager& get_instance();

    // Creates a thread and associates it with *enclave*. The new thread invokes
    // *func*.
    void create_thread(oe_enclave_t* enclave, void (*func)(oe_enclave_t*));

    // Sets whether it's allowed to call pthread_cancel on the current thread.
    // This is also a cancelation point (irrespective of the value of
    // *cancelable*).
    void set_cancelable(bool cancelable);

    // Joins all threads that have been created with create_thread() for
    // *enclave*.
    void join_all_threads(const oe_enclave_t* enclave);

    // Cancels all threads that have been created with create_thread() for
    // *enclave*.
    void cancel_all_threads(oe_enclave_t* enclave);

  private:
    EnclaveThreadManager() = default;

    enum : unsigned int
    {
        Cancelable = 1 << 0,
        Canceling = 1 << 1,
    };

    struct Thread
    {
        std::thread thread;
        std::atomic<bool> finished;
        std::atomic<unsigned int> cancel_state;
    };

    static thread_local Thread* self;

    std::map<const oe_enclave_t*, std::list<Thread>> threads_;
    std::mutex mutex_;
};
} // namespace ert::host
