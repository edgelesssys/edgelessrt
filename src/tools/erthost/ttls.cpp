#include <openenclave/ert.h>
#include <openenclave/internal/syscall/hook.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <cassert>

// TODO remove this dummy
namespace edgeless::ttls
{
class Socket
{
  public:
    virtual int Connect(
        int sockfd,
        const sockaddr* addr,
        socklen_t addrlen) = 0;
};
} // namespace edgeless::ttls

using namespace edgeless;

extern "C" long oe_syscall(long number, ...);

namespace
{
class RawSocket final : public ttls::Socket
{
  public:
    int Connect(int sockfd, const sockaddr* addr, socklen_t addrlen) override
    {
        return oe_syscall(SYS_connect, sockfd, addr, addrlen);
    }
};
} // namespace

static oe_result_t _syscall_hook(
    long number,
    long /*arg1*/,
    long /*arg2*/,
    long /*arg3*/,
    long /*arg4*/,
    long /*arg5*/,
    long /*arg6*/,
    long* ret)
{
    assert(ret);

    switch (number)
    {
        case SYS_connect:
            // TODO call dispatcher instead
            *ret = 2;
            return OE_OK;
    }

    return OE_UNEXPECTED;
}

void ert_init_ttls(const char* config)
{
    if (!config || !*config)
        return;

    // TODO create dispatcher etc

    oe_register_syscall_hook(_syscall_hook);
}
