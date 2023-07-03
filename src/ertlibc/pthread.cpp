// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/jump.h>
#include <openenclave/internal/trace.h>
#include <pthread.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <system_error>
#include "../libc/ertfutex.h"
#include "ertlibc_t.h"
#include "ertthread.h"
#include "new_thread.h"

using namespace std;

// defined in enclave/core/sgx/calls.c
extern "C" bool ert_exiting;

// stdc++ of Ubuntu 22.04 needs this symbol
char __libc_single_threaded;

static ert_thread_t* _to_ert_thread(pthread_t thread) noexcept
{
    assert(thread);
    // ert_thread_t struct is just above pthread struct, see ert/libc/pthread.c
    return reinterpret_cast<ert_thread_t*>(thread) - 1;
}

const ert_thread_t* ert_thread_self()
{
    return _to_ert_thread(pthread_self());
}

static ert_thread_t* _thread_create(void* (*start_routine)(void*), void* arg)
{
    if (!start_routine)
        throw system_error(EFAULT, system_category(), "start_routine");

    const auto new_thread = new oe_new_thread_t;
    oe_new_thread_init(new_thread, start_routine, arg);
    oe_new_thread_queue_push_back(new_thread);

    // ert_create_thread_ocall() will create a new thread that calls
    // ert_create_thread_ecall()
    bool created = false;
    if (ert_create_thread_ocall(&created, oe_get_enclave()) != OE_OK ||
        !created)
    {
        oe_new_thread_queue_remove(new_thread);
        delete new_thread;
        throw runtime_error("ert_create_thread_ocall() failed");
    }

    // wait until a thread enters and executes the queued new_thread
    oe_new_thread_state_wait_exit(new_thread, OE_NEWTHREADSTATE_QUEUED);

    assert(new_thread->self);
    return new_thread->self;
}

int pthread_create(
    pthread_t* thread,
    const pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg)
{
    assert(thread);
    assert(start_routine);

    // get detach state from attr
    int detachstate = PTHREAD_CREATE_JOINABLE;
    if (attr)
    {
        const int res = pthread_attr_getdetachstate(attr, &detachstate);
        if (res)
            return res;
    }

    // create thread
    ert_thread_t* t;
    try
    {
        t = _thread_create(start_routine, arg);
    }
    catch (const system_error& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return e.code().value();
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return -1;
    }

    if (detachstate == PTHREAD_CREATE_DETACHED)
        oe_new_thread_detach(t->new_thread);

    *thread = reinterpret_cast<pthread_t>(t + 1);

    return 0;
}

int pthread_join(pthread_t thread, void** retval)
{
    if (!thread)
        return ESRCH;

    oe_new_thread_t* const new_thread = _to_ert_thread(thread)->new_thread;
    if (!new_thread)
        return EINVAL;

    // Wait until the thread has finished executing.
    // If the enclave is exiting, threads have already been canceled.
    if (!ert_exiting)
        oe_new_thread_state_wait_enter(new_thread, OE_NEWTHREADSTATE_DONE);

    if (retval)
        *retval = new_thread->return_value;

    oe_new_thread_state_update(new_thread, OE_NEWTHREADSTATE_JOINED);

    // Open issue: TLS is not unwound yet

    return 0;
}

int pthread_detach(pthread_t thread)
{
    if (!thread)
        return ESRCH;
    oe_new_thread_t* const new_thread = _to_ert_thread(thread)->new_thread;
    if (!new_thread)
        return EINVAL;
    oe_new_thread_detach(new_thread);
    return 0;
}

void pthread_exit(void* retval)
{
    oe_new_thread_t* const new_thread =
        _to_ert_thread(pthread_self())->new_thread;
    if (new_thread)
    {
        new_thread->return_value = retval;
        oe_longjmp(&new_thread->jmp_exit, 1);
    }
    abort();
}

int pthread_cancel(pthread_t thread)
{
    if (!thread)
        return ESRCH;

    ert_thread_t* const t = _to_ert_thread(thread);
    __atomic_store_n(&t->cancel, true, __ATOMIC_SEQ_CST);

    // thread may sleep, so wake it
    ert_futex_wake_tcs(t->tcs);

    return 0;
}

void pthread_testcancel()
{
    ert_thread_t* const self = _to_ert_thread(pthread_self());
    if (__atomic_load_n(&self->cancel, __ATOMIC_SEQ_CST) && self->cancelable)
    {
        // thread may be woken, so clean up futex
        ert_futex_remove_tcs(self->tcs);

        pthread_exit(PTHREAD_CANCELED);
    }
}

void ert_create_thread_ecall()
{
    oe_new_thread_t* const new_thread = oe_new_thread_queue_pop_front();
    if (!new_thread)
        // ert_create_thread_ecall() called without prior _thread_create()
        abort();

    ert_thread_t* const self = _to_ert_thread(pthread_self());
    self->new_thread = new_thread;
    new_thread->self = self;

    // run the thread function
    oe_new_thread_state_update(new_thread, OE_NEWTHREADSTATE_RUNNING);
    new_thread->self->cancelable = true;
    if (oe_setjmp(&new_thread->jmp_exit) == 0)
        new_thread->return_value = new_thread->func(new_thread->arg);
    new_thread->self->cancelable = false;
    oe_new_thread_state_update(new_thread, OE_NEWTHREADSTATE_DONE);

    // wait for join before releasing the thread
    oe_new_thread_state_wait_enter_or_detached(
        new_thread, OE_NEWTHREADSTATE_JOINED);

    delete new_thread;

    // Open issue: TLS is not unwound yet
}

extern "C"
{
    OE_WEAK_ALIAS(pthread_create, __pthread_create);
    OE_WEAK_ALIAS(pthread_join, __pthread_join);
    OE_WEAK_ALIAS(pthread_exit, __pthread_exit);
}

// These functions are called using weak refs in stdc++ (see
// libgcc/gthr-posix.h). They would not be linked if not refererenced elsewhere.
// As enclaves are linked with -gc-sections, the variables defined here won't be
// in the final binary. The same goes for those functions that are really not
// used.
#define ert_ref(x) extern const auto ert_##x = x
ert_ref(pthread_once);
ert_ref(pthread_getspecific);
ert_ref(pthread_setspecific);
ert_ref(pthread_create);
ert_ref(pthread_join);
ert_ref(pthread_equal);
ert_ref(pthread_self);
ert_ref(pthread_detach);
ert_ref(pthread_cancel);
ert_ref(sched_yield);
ert_ref(pthread_mutex_lock);
ert_ref(pthread_mutex_trylock);
ert_ref(pthread_mutex_timedlock);
ert_ref(pthread_mutex_unlock);
ert_ref(pthread_mutex_init);
ert_ref(pthread_mutex_destroy);
ert_ref(pthread_cond_init);
ert_ref(pthread_cond_broadcast);
ert_ref(pthread_cond_signal);
ert_ref(pthread_cond_wait);
ert_ref(pthread_cond_timedwait);
ert_ref(pthread_cond_destroy);
ert_ref(pthread_key_create);
ert_ref(pthread_key_delete);
ert_ref(pthread_mutexattr_init);
ert_ref(pthread_mutexattr_settype);
ert_ref(pthread_mutexattr_destroy);
#undef ert_ref
