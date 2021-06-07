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

    // Joins all threads that have been created with create_thread() for
    // *enclave*.
    void join_all_threads(const oe_enclave_t* enclave);

    // Cancels all threads that have been created with create_thread() for
    // *enclave*.
    void cancel_all_threads(oe_enclave_t* enclave);

  private:
    EnclaveThreadManager() = default;

    struct Thread
    {
        std::thread thread;
        std::atomic<bool> finished;
    };

    std::map<const oe_enclave_t*, std::list<Thread>> threads_;
    std::mutex mutex_;
    std::atomic<bool> exit_ = false;
};
} // namespace ert::host
