#include <sys/eventfd.h>
#include "syscall_u.h"

oe_host_fd_t oe_syscall_eventfd_ocall(unsigned int initval, int flags)
{
    return eventfd(initval, flags);
}
