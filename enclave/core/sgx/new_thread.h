// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/internal/jump.h>
#include <openenclave/internal/thread.h>

typedef enum _oe_new_thread_state
{
    OE_NEWTHREADSTATE_QUEUED,
    OE_NEWTHREADSTATE_RUNNING,
    OE_NEWTHREADSTATE_DONE,
    OE_NEWTHREADSTATE_JOINED
} oe_new_thread_state_t;

typedef struct _oe_new_thread
{
    // set by oe_new_thread_init()
    void* (*func)(void*);
    void* arg;

    void* return_value;
    oe_thread_t self;
    oe_jmpbuf_t jmp_exit; // for oe_thread_exit

    struct _oe_new_thread* _next; // for the queue

    oe_new_thread_state_t _state;
    oe_mutex_t _mutex;
    oe_cond_t _cond;

    bool _detached;
} oe_new_thread_t;

// initializes a newly allocated oe_new_thread_t object
void oe_new_thread_init(
    oe_new_thread_t* new_thread,
    void* (*func)(void*),
    void* arg);

// detaches the thread
void oe_new_thread_detach(oe_new_thread_t* new_thread);

// updates the thread's state and notifies waiting threads
void oe_new_thread_state_update(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t new_state);

// waits until the thread's state is updated to desired_state
void oe_new_thread_state_wait_enter(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t desired_state);

// waits until the thread's state is updated to desired_state or thread is
// detached
void oe_new_thread_state_wait_enter_or_detached(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t desired_state);

// waits until the thread's state is updated to a state other than
// undesired_state
void oe_new_thread_state_wait_exit(
    oe_new_thread_t* new_thread,
    oe_new_thread_state_t undesired_state);

// queue
void oe_new_thread_queue_push_back(oe_new_thread_t* new_thread);
oe_new_thread_t* oe_new_thread_queue_pop_front();
void oe_new_thread_queue_remove(const oe_new_thread_t* new_thread);
