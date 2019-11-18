// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/host.h>
#include <condition_variable>
#include <map>
#include <mutex>

namespace open_enclave::host
{
class EnclaveThreadManager
{
  public:
    EnclaveThreadManager(const EnclaveThreadManager&) = delete;
    EnclaveThreadManager& operator=(const EnclaveThreadManager&) = delete;

    static EnclaveThreadManager& get_instance();

    // Creates a thread and associates it with *enclave*. The new thread invokes
    // *func*.
    void create_thread(oe_enclave_t* enclave, void (*func)(oe_enclave_t*));

    // Joins all threads that have been created with create_thread() for
    // *enclave*.
    void join_all_threads(const oe_enclave_t* enclave);

  private:
    EnclaveThreadManager() = default;

    std::map<const oe_enclave_t*, int> thread_count_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
} // namespace open_enclave::host
