// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "../3rdparty/musl/musl/src/internal/libc.h"
#include "../3rdparty/musl/musl/src/internal/stdio_impl.h"
#include "../ertlibc/ertthread.h"

// copied from musl/thread/pthread_create.c
static void init_file_lock(FILE* f)
{
    if (f && f->lock < 0)
        f->lock = 0;
}

// adapted from __pthread_create() in musl/thread/pthread_create.c
void ert_musl_init_threaded(void)
{
    if (libc.threaded)
        return;
    for (FILE* f = *__ofl_lock(); f; f = f->next)
        init_file_lock(f);
    __ofl_unlock();
    init_file_lock(__stdin_used);
    init_file_lock(__stdout_used);
    init_file_lock(__stderr_used);
    libc.threaded = 1;
}
