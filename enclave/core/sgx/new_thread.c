// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "new_thread.h"
#include <openenclave/corelibc/assert.h>

static struct
{
    oe_new_thread_t* front;
    oe_new_thread_t* back;
    oe_spinlock_t lock;
} _queue = {.lock = OE_SPINLOCK_INITIALIZER};

static void _check(oe_result_t result)
{
    if (result != OE_OK)
        oe_abort();
}

void oe_new_thread_init(
    oe_new_thread_t* new_thread,
    void* (*func)(void*),
    void* arg)
{
    oe_assert(new_thread);
    oe_assert(func);
    new_thread->func = func;
    new_thread->arg = arg;
    new_thread->_next = NULL;
    new_thread->_state = OE_NEWTHREADSTATE_QUEUED;
    _check(oe_mutex_init(&new_thread->_mutex));
    _check(oe_cond_init(&new_thread->_cond));
}

void oe_new_thread_state_update(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t new_state)
{
    oe_assert(new_thread);
    _check(oe_mutex_lock(&new_thread->_mutex));
    new_thread->_state = new_state;
    _check(oe_cond_broadcast(&new_thread->_cond));
    _check(oe_mutex_unlock(&new_thread->_mutex));
}

void oe_new_thread_state_wait_enter(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t desired_state)
{
    oe_assert(new_thread);
    _check(oe_mutex_lock(&new_thread->_mutex));
    while (new_thread->_state != desired_state)
        _check(oe_cond_wait(&new_thread->_cond, &new_thread->_mutex));
    _check(oe_mutex_unlock(&new_thread->_mutex));
}

void oe_new_thread_state_wait_exit(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t undesired_state)
{
    oe_assert(new_thread);
    _check(oe_mutex_lock(&new_thread->_mutex));
    while (new_thread->_state == undesired_state)
        _check(oe_cond_wait(&new_thread->_cond, &new_thread->_mutex));
    _check(oe_mutex_unlock(&new_thread->_mutex));
}

void oe_new_thread_queue_push_back(oe_new_thread_t* new_thread)
{
    oe_assert(new_thread);
    oe_assert(!new_thread->_next);
    oe_assert(new_thread->_state == OE_NEWTHREADSTATE_QUEUED);

    oe_spin_lock(&_queue.lock);

    if (_queue.back)
        _queue.back->_next = new_thread;
    else
        _queue.front = new_thread;

    _queue.back = new_thread;

    oe_spin_unlock(&_queue.lock);
}

oe_new_thread_t* oe_new_thread_queue_pop_front()
{
    oe_spin_lock(&_queue.lock);

    oe_new_thread_t* const new_thread = _queue.front;

    if (new_thread)
    {
        _queue.front = _queue.front->_next;

        if (!_queue.front)
            _queue.back = NULL;
    }

    oe_spin_unlock(&_queue.lock);

    return new_thread;
}

void oe_new_thread_queue_remove(const oe_new_thread_t* new_thread)
{
    oe_assert(new_thread);

    oe_new_thread_t* prev = NULL;

    oe_spin_lock(&_queue.lock);
    for (oe_new_thread_t* p = _queue.front; p; p = p->_next)
    {
        if (p == new_thread)
        {
            if (p == _queue.front)
                _queue.front = p->_next;
            if (p == _queue.back)
                _queue.back = prev;
            if (prev)
                prev->_next = p->_next;
            p->_next = NULL;
            break;
        }
        prev = p;
    }
    oe_spin_unlock(&_queue.lock);
}
