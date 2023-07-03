// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "ertfutex.h"
#include <assert.h>
#include <errno.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/trace.h>
#include <stdlib.h>
#include "../enclave/core/sgx/td.h"
#include "futex.h"

#define FUTEX_BITSET_MATCH_ANY 0xffffffff

oe_result_t oe_sgx_thread_timedwait_ocall(
    int* retval,
    oe_enclave_t* enclave,
    uint64_t tcs,
    const struct timespec* timeout,
    bool timeout_absolute);

oe_result_t oe_sgx_thread_wake_multiple_ocall(
    oe_enclave_t* enclave,
    const uint64_t* tcs,
    size_t tcs_size);

static oe_result_t _oe_sgx_thread_timedwait_ocall(
    int* retval,
    oe_enclave_t* enclave,
    uint64_t tcs,
    const struct timespec* timeout,
    bool timeout_absolute)
{
    (void)retval;
    (void)enclave;
    (void)tcs;
    (void)timeout;
    return OE_UNSUPPORTED;
}
OE_WEAK_ALIAS(_oe_sgx_thread_timedwait_ocall, oe_sgx_thread_timedwait_ocall);

static oe_result_t _oe_sgx_thread_wake_multiple_ocall(
    oe_enclave_t* enclave,
    const uint64_t* tcs,
    size_t tcs_size)
{
    (void)enclave;
    (void)tcs;
    (void)tcs_size;
    return OE_UNSUPPORTED;
}
OE_WEAK_ALIAS(
    _oe_sgx_thread_wake_multiple_ocall,
    oe_sgx_thread_wake_multiple_ocall);

static oe_spinlock_t _lock;

// Each thread has an info_t object. Objects of currently waiting threads are in
// the linked list.
typedef struct _info
{
    struct _info* next;
    const int* uaddr; // The futex addr the thread is waiting on.
    uint64_t tcs;
} info_t;

static __thread info_t _thread;
static info_t* _front;
static info_t* _back;

static void _list_unlink(const info_t* p, info_t* prev)
{
    if (p == _front)
        _front = p->next;
    if (p == _back)
        _back = prev;
    if (prev)
        prev->next = p->next;
}

static void _list_push_back(info_t* info)
{
    assert(info);

    info->next = NULL;

    if (_back)
        _back->next = info;
    else
        _front = info;

    _back = info;
}

static info_t* _list_pop(const int* uaddr)
{
    assert(uaddr);

    info_t* prev = NULL;

    for (info_t* p = _front; p; p = p->next)
    {
        if (p->uaddr == uaddr)
        {
            _list_unlink(p, prev);
            return p;
        }
        prev = p;
    }

    return NULL;
}

static bool _list_contains(const info_t* info)
{
    assert(info);

    for (const info_t* p = _front; p; p = p->next)
    {
        if (p == info)
            return true;
    }

    return false;
}

static void _list_remove(const info_t* info)
{
    assert(info);

    info_t* prev = NULL;

    for (info_t* p = _front; p; p = p->next)
    {
        if (p == info)
        {
            _list_unlink(p, prev);
            return;
        }
        prev = p;
    }
}

static void _list_remove_tcs(uint64_t tcs)
{
    assert(tcs);

    info_t* prev = NULL;

    for (info_t* p = _front; p; p = p->next)
    {
        if (p->tcs == tcs)
        {
            _list_unlink(p, prev);
            return;
        }
        prev = p;
    }
}

static void _list_replace(const int* uaddr, const int* uaddr2)
{
    assert(uaddr);
    assert(uaddr2);

    for (info_t* p = _front; p; p = p->next)
    {
        if (p->uaddr == uaddr)
            p->uaddr = uaddr2;
    }
}

static int _wait(
    const int* uaddr,
    int val,
    const struct timespec* timeout,
    bool timeout_absolute)
{
    assert(uaddr);
    assert(!_thread.uaddr);

    const uint64_t tcs = (uint64_t)td_to_tcs(oe_sgx_get_td());
    _thread.tcs = tcs;
    int result = -1;

    oe_spin_lock(&_lock);

    // Don't wait if *uaddr has already changed
    if (__atomic_load_n(uaddr, __ATOMIC_SEQ_CST) != val)
    {
        result = -EWOULDBLOCK;
        goto done;
    }

    // Add the self thread to the wait list
    _thread.uaddr = uaddr;
    _list_push_back(&_thread);

    for (;;)
    {
        oe_spin_unlock(&_lock);

        int ret = -1;
        const bool timedout =
            oe_sgx_thread_timedwait_ocall(
                &ret, oe_get_enclave(), tcs, timeout, timeout_absolute) ==
                OE_OK &&
            ret == ETIMEDOUT;

        oe_spin_lock(&_lock);

        if (timedout)
        {
            _list_remove(&_thread);
            result = -ETIMEDOUT;
            break;
        }

        // If self is no longer in the list, then it was woken
        if (!_list_contains(&_thread))
        {
            result = 0;
            break;
        }
    }

done:
    oe_spin_unlock(&_lock);
    _thread.uaddr = NULL;
    return result;
}

static int _wake(const int* uaddr, unsigned int val, const int* uaddr2)
{
    assert(uaddr);

    const unsigned int max = 128;
    uint64_t tcs[max];
    if (val > max)
        val = max;

    unsigned int count = 0;

    oe_spin_lock(&_lock);

    // Get up to val threads waiting on this futex.
    for (; count < val; ++count)
    {
        const info_t* const waiter = _list_pop(uaddr);
        if (!waiter)
            break;
        tcs[count] = waiter->tcs;
    }

    // Handle FUTEX_REQUEUE.
    if (uaddr2)
        _list_replace(uaddr, uaddr2);

    oe_spin_unlock(&_lock);

    if (count > 0 && oe_sgx_thread_wake_multiple_ocall(
                         oe_get_enclave(), tcs, count) != OE_OK)
        abort();

    return count;
}

int ert_futex(
    const int* uaddr,
    int op,
    int val,
    const struct timespec* timeout,
    const int* uaddr2,
    int val3)
{
    if (!uaddr)
        return -EFAULT;

    op &= ~FUTEX_PRIVATE;

    if (op == FUTEX_WAIT)
        return _wait(uaddr, val, timeout, false);
    if (op == FUTEX_WAKE && val)
        return _wake(uaddr, val, NULL);
    if (op == FUTEX_REQUEUE && uaddr2)
        return _wake(uaddr, val, uaddr2);
    if (op == FUTEX_WAIT_BITSET && val3 == FUTEX_BITSET_MATCH_ANY)
        return _wait(uaddr, val, timeout, true);

    OE_TRACE_FATAL("unsupported futex operation %d", op);
    abort();
}

void ert_futex_remove_tcs(const void* tcs)
{
    oe_spin_lock(&_lock);
    _list_remove_tcs(tcs);
    oe_spin_unlock(&_lock);
}

void ert_futex_wake_tcs(const void* tcs)
{
    if (oe_sgx_thread_wake_multiple_ocall(
            oe_get_enclave(), (uint64_t*)&tcs, 1) != OE_OK)
        abort();
}
