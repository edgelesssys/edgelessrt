#include <fcntl.h>
#include <openenclave/ert.h>
#include <openenclave/internal/syscall/hook.h>
#include <openenclave/internal/syscall/netdb.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <ttls/ttls.h>
#include <cassert>

using namespace edgeless;

extern "C" long oe_syscall(long number, ...);
extern "C" int (*ert_getaddrinfo_func)(
    const char*,
    const char*,
    const oe_addrinfo*,
    oe_addrinfo**);

namespace
{
class OESocket final : public ttls::RawSocket
{
  public:
    int Connect(int sockfd, const sockaddr* addr, socklen_t addrlen) override
    {
        return oe_syscall(SYS_connect, sockfd, addr, addrlen);
    }
    int Bind(int sockfd, const sockaddr* addr, socklen_t addrlen) override
    {
        return oe_syscall(SYS_bind, sockfd, addr, addrlen);
    }
    int Accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags)
        override
    {
        // adapted from musl/src/network/accept4.c
        const int ret = oe_syscall(SYS_accept, sockfd, addr, addrlen);
        if (ret < 0)
            return ret;
        if (flags & SOCK_CLOEXEC)
            oe_syscall(SYS_fcntl, ret, F_SETFD, FD_CLOEXEC);
        if (flags & SOCK_NONBLOCK)
            oe_syscall(SYS_fcntl, ret, F_SETFL, O_NONBLOCK);
        return ret;
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
    int Getaddrinfo(
        const char* node,
        const char* service,
        const addrinfo* hints,
        addrinfo** res) override
    {
        return oe_getaddrinfo(
            node,
            service,
            reinterpret_cast<const oe_addrinfo*>(hints),
            reinterpret_cast<oe_addrinfo**>(res));
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
        case SYS_bind:
            *ret =
                dis->Bind(arg1, reinterpret_cast<const sockaddr*>(arg2), arg3);
            return OE_OK;
        case SYS_write:
            *ret = dis->Send(
                arg1, reinterpret_cast<const void*>(arg2), arg3, arg4);
            return OE_OK;
        case SYS_accept:
            *ret = dis->Accept4(
                arg1,
                reinterpret_cast<sockaddr*>(arg2),
                reinterpret_cast<socklen_t*>(arg3),
                0);
            return OE_OK;
        case SYS_accept4:
            *ret = dis->Accept4(
                arg1,
                reinterpret_cast<sockaddr*>(arg2),
                reinterpret_cast<socklen_t*>(arg3),
                arg4);
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

static int _getaddrinfo(
    const char* node,
    const char* service,
    const oe_addrinfo* hints,
    oe_addrinfo** res)
{
    return dis->Getaddrinfo(
        node,
        service,
        reinterpret_cast<const addrinfo*>(hints),
        reinterpret_cast<addrinfo**>(res));
}

/**
 * Initialize ttls dispatcher and hook syscalls
 */
void ert_init_ttls(const char* config)
{
    if (!config || !*config)
        return;

    const auto raw_sock = std::make_shared<OESocket>();
    const auto tls_sock = std::make_shared<ttls::TlsSocket>(raw_sock, true);
    dis = std::make_unique<ttls::Dispatcher>(config, raw_sock, tls_sock);

    oe_register_syscall_hook(_syscall_hook);
    ert_getaddrinfo_func = _getaddrinfo;
}
