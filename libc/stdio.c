// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>
#include "../3rdparty/musl/musl/src/internal/pthread_impl.h"
#include "../3rdparty/musl/musl/src/internal/stdio_impl.h"

// We must adapt the musl getc/putc family of functions because they originally
// depend on futex.

// adapted from musl/stdio/getc.h
static int locking_getc(FILE* f)
{
    if (a_cas(&f->lock, 0, MAYBE_WAITERS - 1))
        __lockfile(f);
    int c = getc_unlocked(f);
    a_swap(&f->lock, 0);
    return c;
}

// copied from musl/stdio/getc.h
static inline int do_getc(FILE* f)
{
    int l = f->lock;
    if (l < 0 || l && (l & ~MAYBE_WAITERS) == __pthread_self()->tid)
        return getc_unlocked(f);
    return locking_getc(f);
}

// copied from musl/stdio/getchar.c
int getchar(void)
{
    return do_getc(stdin);
}

// copied from musl/stdio/getc.c
int getc(FILE* f)
{
    return do_getc(f);
}
weak_alias(getc, _IO_getc);

// copied from musl/stdio/fgetc.c
int fgetc(FILE* f)
{
    return do_getc(f);
}

// adapted from musl/stdio/putc.h
static int locking_putc(int c, FILE* f)
{
    if (a_cas(&f->lock, 0, MAYBE_WAITERS - 1))
        __lockfile(f);
    c = putc_unlocked(c, f);
    a_swap(&f->lock, 0);
    return c;
}

// copied from musl/stdio/putc.h
static inline int do_putc(int c, FILE* f)
{
    int l = f->lock;
    if (l < 0 || l && (l & ~MAYBE_WAITERS) == __pthread_self()->tid)
        return putc_unlocked(c, f);
    return locking_putc(c, f);
}

// copied from musl/stdio/putchar.c
int putchar(int c)
{
    return do_putc(c, stdout);
}

// copied from musl/stdio/putc.c
int putc(int c, FILE* f)
{
    return do_putc(c, f);
}
weak_alias(putc, _IO_putc);

// copied from musl/stdio/fputc.c
int fputc(int c, FILE* f)
{
    return do_putc(c, f);
}

size_t __fread_chk(
    void* buffer,
    size_t buffersize,
    size_t size,
    size_t count,
    FILE* stream)
{
    if (count * size > buffersize)
        abort();
    return fread(buffer, size, count, stream);
}
