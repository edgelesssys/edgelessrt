// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/pthread.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/sgx/td.h>
#include "../../ertlibc/ertthread.h"
#include "pthread_impl.h"

#define PTHREAD_MUTEX_ADAPTIVE_NP 3

pthread_t __pthread_self()
{
    static __thread struct
    {
        ert_thread_t et;
        struct __pthread pt;
    } self;

    if (!self.pt.self)
    {
        __init_tp(&self.pt);

        const unsigned int guard_size = OE_PAGE_SIZE;

        // Stack is located immediately before TCS. See _add_data_pages() in
        // host/sgx/create.c.
        // pt.stack must point to the end of the stack, i.e., the guard page.
        uint8_t* td_to_tcs(const oe_sgx_td_t* td);
        self.pt.stack = td_to_tcs(oe_sgx_get_td()) - guard_size;

        self.pt.stack_size = oe_get_stack_size();
        self.pt.guard_size = guard_size;
        self.et.tid = self.pt.tid;
    }

    return &self.pt;
}

__attribute__((__weak__)) int pthread_cancel(pthread_t thread)
{
    return -1;
}

__attribute__((__weak__)) void pthread_testcancel()
{
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void* value))
{
    return oe_pthread_key_create((oe_pthread_key_t*)key, destructor);
}

OE_WEAK_ALIAS(pthread_key_create, __pthread_key_create);

int pthread_key_delete(pthread_key_t key)
{
    return oe_pthread_key_delete((oe_pthread_key_t)key);
}

OE_WEAK_ALIAS(pthread_key_delete, __pthread_key_delete);

int pthread_setspecific(pthread_key_t key, const void* value)
{
    return oe_pthread_setspecific((oe_pthread_key_t)key, value);
}

void* pthread_getspecific(pthread_key_t key)
{
    return oe_pthread_getspecific((oe_pthread_key_t)key);
}

static void _mutex_fix_recursive(pthread_mutex_t* m)
{
    // stdc++ intializes recursive mutexes with an initializer suited for glibc
    // but not for musl. We must fix this.
    if (__atomic_load_n(&m->__u.__i[4], __ATOMIC_RELAXED) != 1)
        return;
    __atomic_store_n(&m->__u.__i[4], 0, __ATOMIC_SEQ_CST);
    __atomic_or_fetch(&m->_m_type, PTHREAD_MUTEX_RECURSIVE, __ATOMIC_SEQ_CST);
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
    _mutex_fix_recursive(m);
    return __pthread_mutex_lock(m);
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
    _mutex_fix_recursive(m);
    return __pthread_mutex_trylock(m);
}

int pthread_mutex_timedlock(pthread_mutex_t* m, const struct timespec* t)
{
    _mutex_fix_recursive(m);
    return __pthread_mutex_timedlock(m, t);
}

int pthread_mutexattr_settype(pthread_mutexattr_t* a, int type)
{
    // Handle glibc extension. It's just a performance optimization, so we can
    // use normal mutex instead.
    if (type == PTHREAD_MUTEX_ADAPTIVE_NP)
        type = PTHREAD_MUTEX_NORMAL;

    // copied from musl/thread/pthread_mutexattr_settype.c
    if ((unsigned)type > 2)
        return EINVAL;
    a->__attr = (a->__attr & ~3) | type;
    return 0;
}
