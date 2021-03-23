#include <openenclave/ert.h>
#include <openenclave/internal/syscall/hook.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <ttls/ttls.h>
#include <cassert>

using namespace edgeless;

extern "C" long oe_syscall(long number, ...);

namespace
{
class OESocket final : public ttls::Socket
{
  public:
    int Connect(int sockfd, const sockaddr* addr, socklen_t addrlen) override
    {
        return oe_syscall(SYS_connect, sockfd, addr, addrlen);
    }
    ssize_t Send(int sockfd, const void* buf, size_t len, int /*flags*/)
        override
    {
        return oe_syscall(SYS_write, sockfd, buf, len);
    }
    ssize_t Recv(int sockfd, void* buf, size_t len, int /*flags*/) override
    {
        return oe_syscall(SYS_read, sockfd, buf, len);
    }
    int Shutdown(int fd, int how) override
    {
        return oe_syscall(SYS_shutdown, fd, how);
    }
    int Close(int fd) override
    {
        return oe_syscall(SYS_close, fd);
    }
};
} // namespace

static std::unique_ptr<ttls::Dispatcher> dis;

static oe_result_t _syscall_hook(
    long number,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long /*arg5*/,
    long /*arg6*/,
    long* ret)
{
    assert(ret);

    switch (number)
    {
        case SYS_connect:
            *ret = dis->Connect(arg1, reinterpret_cast<sockaddr*>(arg2), arg3);
            return OE_OK;
        case SYS_write:
            *ret = dis->Send(
                arg1, reinterpret_cast<const void*>(arg2), arg3, arg4);
            return OE_OK;
        case SYS_read:
            *ret = dis->Recv(arg1, reinterpret_cast<void*>(arg2), arg3, arg4);
            return OE_OK;
        case SYS_shutdown:
            *ret = dis->Shutdown(arg1, arg2);
            return OE_OK;
        case SYS_close:
            *ret = dis->Close(arg1);
            return OE_OK;
    }

    return OE_UNEXPECTED;
}

/**
 * Initialize ttls dispatcher and hook syscalls
 */
void ert_init_ttls(const char* config)
{
    if (!config || !*config)
        return;

    const auto sock = std::make_shared<OESocket>();
    const auto raw_sock = std::make_shared<ttls::RawSocket>(sock);
    const auto tls_sock = std::make_shared<ttls::MbedtlsSocket>(sock);
    dis = new ttls::Dispatcher(config, raw_sock, tls_sock);

    oe_register_syscall_hook(_syscall_hook);
}
