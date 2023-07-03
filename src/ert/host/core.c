// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <assert.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "../host/sgx/enclave.h"
#include "platform_u.h"

int oe_sgx_thread_timedwait_ocall(
    oe_enclave_t* enclave,
    uint64_t tcs,
    const struct oe_sgx_thread_timedwait_ocall_timespec* timeout,
    bool timeout_absolute)
{
    EnclaveEvent* event = GetEnclaveEvent(enclave, tcs);
    assert(event);

    if (__sync_fetch_and_add(&event->value, (uint32_t)-1) == 0)
    {
        do
        {
            const long res = syscall(
                __NR_futex,
                &event->value,
                timeout_absolute ? FUTEX_WAIT_BITSET_PRIVATE
                                 : FUTEX_WAIT_PRIVATE,
                -1,
                timeout,
                NULL,
                FUTEX_BITSET_MATCH_ANY);

            if (res == -1 && errno == ETIMEDOUT)
            {
                __sync_fetch_and_add(&event->value, 1);
                return ETIMEDOUT;
            }

            // If event->value is still -1, then this is a spurious-wake.
            // Spurious-wakes are ignored by going back to FUTEX_WAIT.
            // Since FUTEX_WAIT uses atomic instructions to load event->value,
            // it is safe to use a non-atomic operation here.
        } while (event->value == (uint32_t)-1);
    }

    return 0;
}

void oe_sgx_thread_wake_multiple_ocall(
    oe_enclave_t* enclave,
    const uint64_t* tcs,
    size_t tcs_size)
{
    for (size_t i = 0; i < tcs_size; ++i)
    {
        EnclaveEvent* event = GetEnclaveEvent(enclave, tcs[i]);
        assert(event);

        if (__sync_fetch_and_add(&event->value, 1) != 0)
            syscall(
                __NR_futex,
                &event->value,
                FUTEX_WAKE_PRIVATE,
                1,
                NULL,
                NULL,
                0);
    }
}
