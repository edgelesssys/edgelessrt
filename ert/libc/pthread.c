// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/pthread.h>
#include "../ertlibc/ertthread.h"
#include "pthread_impl.h"

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
        self.et.tid = self.pt.tid;
    }

    return &self.pt;
}

int pthread_cancel(pthread_t thread)
{
    return -1;
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
