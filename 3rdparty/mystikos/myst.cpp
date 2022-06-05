#include <cstdlib>

extern "C" void myst_eraise()
{
}

extern "C" void __myst_panic()
{
    abort();
}

extern "C" long myst_syscall_clock_gettime(
    clockid_t clk_id,
    struct timespec* tp)
{
    *tp = timespec{};
    return 0;
}

extern "C" gid_t myst_syscall_getegid()
{
    return 0;
}

extern "C" uid_t myst_syscall_geteuid()
{
    return 0;
}
