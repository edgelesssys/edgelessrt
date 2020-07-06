// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/pthread.h>
#include "pthread_impl.h"

pthread_t __pthread_self()
{
    static __thread struct __pthread self;
    if (!self.self)
        __init_tp(&self);
    return &self;
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
