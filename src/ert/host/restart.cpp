#include <unistd.h>
#include <cstdlib>
#include "ertlibc_u.h"

static char** _argv;
static char** _envp;

__attribute__((constructor)) static void _init(int, char** argv, char** envp)
{
    _argv = argv;
    _envp = envp;
}

void ert_restart_host_process_ocall()
{
    execve("/proc/self/exe", _argv, _envp);
    abort();
}
