// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/advanced/allocator.h>
#include <openenclave/bits/result.h>
#include <openenclave/corelibc/assert.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/ert_args.h>

#define AT_PAGESZ 6

static int _argc;
static char** _argv;

oe_result_t ert_init_args(void)
{
    oe_assert(!_argv);

    const ert_args_t args = ert_get_args();

    // check args
    if (args.argc < 0 || (args.argc > 0 && !args.argv))
        return OE_INVALID_PARAMETER;
    if (args.envc < 0 || (args.envc > 0 && !args.envp))
        return OE_INVALID_PARAMETER;
    if (args.auxc < 0 || (args.auxc > 0 && !args.auxv))
        return OE_INVALID_PARAMETER;

    // Check if aux contains PAGESZ. Otherwise, we need to inject it.
    int inject_pagesz = 1;
    for (int i = 0; i < args.auxc; ++i)
        if (args.auxv[2 * i] == AT_PAGESZ)
            inject_pagesz = 0;

    //
    // Allocate and arrange argv, envp, and auxv like the ELF loader would do:
    // argv... NULL envp... NULL auxv...
    //

    OE_STATIC_ASSERT(
        sizeof *args.argv == sizeof(char*) &&
        sizeof *args.envp == sizeof(char*) &&
        sizeof *args.auxv == sizeof(char*));

    // will never be freed, so use allocator_calloc to exclude it from leak
    // detection
    char** p = oe_allocator_calloc(
        (size_t)(
            args.argc + 1 + 1 + args.envc + 1 +
            2 * (args.auxc + inject_pagesz + 1)),
        sizeof *p);

    if (!p)
        return OE_OUT_OF_MEMORY;

    _argv = p;

    memcpy(p, args.argv, (size_t)args.argc * sizeof *args.argv);
    p += args.argc + 1;

    // Inject an environment variable that allows programs to detect that they
    // are running in an enclave.
    static char env_is_enclave[] = "OE_IS_ENCLAVE=1";
    *p = env_is_enclave;
    ++p;

    memcpy(p, args.envp, (size_t)args.envc * sizeof *args.envp);
    p += args.envc + 1;

    memcpy(p, args.auxv, 2 * (size_t)args.auxc * sizeof *args.auxv);
    p += 2 * args.auxc;

    if (inject_pagesz)
    {
        long* const pa = (long*)p;
        pa[0] = AT_PAGESZ;
        pa[1] = OE_PAGE_SIZE;
    }

    _argc = args.argc;

    // set __environ to make getenv() work
    extern char** __environ;
    __environ = ert_get_envp();

    return OE_OK;
}

int ert_get_argc(void)
{
    oe_assert(_argv && "ert_get_argc called before oe_init_args");
    return _argc;
}

char** ert_get_argv(void)
{
    oe_assert(_argv && "ert_get_argv called before oe_init_args");
    return _argv;
}

char** ert_get_envp(void)
{
    oe_assert(_argv && "ert_get_envp called before oe_init_args");
    return _argv + _argc + 1;
}

ert_args_t _ert_get_args_default(void)
{
    return (ert_args_t){};
}
OE_WEAK_ALIAS(_ert_get_args_default, ert_get_args);
