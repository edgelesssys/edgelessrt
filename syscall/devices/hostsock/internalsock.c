// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

/*
The internal sockets transfer data between each other without leaving the
enclave. A socket becomes internal by being bound or connected to any port on
255.0.0.1. These sockets than behave as usal.
To support waiting on nonblocking sockets, the internal socket will create an
eventfd if any poller requests its host fd.
*/

#include <openenclave/corelibc/assert.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/internal/syscall/arpa/inet.h>
#include <openenclave/internal/syscall/eventfd.h>
#include <openenclave/internal/syscall/fcntl.h>
#include <openenclave/internal/syscall/raise.h>
#include <openenclave/internal/syscall/unistd.h>
#include "sock.h"
#include "syscall_t.h" // TODO remove

#define STUB_(x, t, r)             \
    static t _stub_##x()           \
    {                              \
        OE_RAISE_ERRNO(OE_ENOSYS); \
    done:                          \
        return r;                  \
    }

#define STUB(x) STUB_(x, int, -1)
#define STUBS(x) STUB_(x, ssize_t, -1)

static int _sock_dup(oe_fd_t* sock_, oe_fd_t** new_sock_out);
STUB(ioctl)
static int _sock_fcntl(oe_fd_t* sock_, int cmd, uint64_t arg);
static ssize_t _sock_read(oe_fd_t* sock_, void* buf, size_t count);
static ssize_t _sock_write(oe_fd_t* sock_, const void* buf, size_t count);
STUBS(readv)
STUBS(writev)
static oe_host_fd_t _sock_get_host_fd(oe_fd_t* sock_);
static int _sock_close(oe_fd_t* sock_);
static oe_fd_t* _sock_accept(
    oe_fd_t* sock_,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen);
STUB(bind)
static int _sock_listen(oe_fd_t* sock_, int backlog);
static int _sock_shutdown(oe_fd_t* sock_, int how);
STUB(getsockopt)
static int _sock_setsockopt(
    oe_fd_t* sock_,
    int level,
    int optname,
    const void* optval,
    oe_socklen_t optlen);
static int _sock_getsockname(
    oe_fd_t* sock_,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen);
static ssize_t _sock_recv(oe_fd_t* sock_, void* buf, size_t count, int flags);
static ssize_t _sock_send(
    oe_fd_t* sock_,
    const void* buf,
    size_t count,
    int flags);
static ssize_t _sock_recvfrom(
    oe_fd_t* sock_,
    void* buf,
    size_t count,
    int flags,
    struct oe_sockaddr* src_addr,
    oe_socklen_t* addrlen);
static ssize_t _sock_sendto(
    oe_fd_t* sock_,
    const void* buf,
    size_t count,
    int flags,
    const struct oe_sockaddr* dest_addr,
    oe_socklen_t addrlen);
STUBS(recvmsg)
STUBS(sendmsg)
STUB(connect)

static oe_socket_ops_t _sock_ops = {
    .fd.dup = _sock_dup,
    .fd.ioctl = _stub_ioctl,
    .fd.fcntl = _sock_fcntl,
    .fd.read = _sock_read,
    .fd.write = _sock_write,
    .fd.readv = _stub_readv,
    .fd.writev = _stub_writev,
    .fd.get_host_fd = _sock_get_host_fd,
    .fd.close = _sock_close,
    .accept = _sock_accept,
    .bind = _stub_bind,
    .listen = _sock_listen,
    .shutdown = _sock_shutdown,
    .getsockopt = _stub_getsockopt,
    .setsockopt = _sock_setsockopt,
    .getpeername = _sock_getsockname,
    .getsockname = _sock_getsockname,
    .recv = _sock_recv,
    .send = _sock_send,
    .recvfrom = _sock_recvfrom,
    .sendto = _sock_sendto,
    .recvmsg = _stub_recvmsg,
    .sendmsg = _stub_sendmsg,
    .connect = _stub_connect,
};

static const uint32_t _ipaddr = 0xFF000001;      // 255.0.0.1
static const uint16_t _client_port = 1024;       // >= 1024 to satisfy test
static internalsock_boundsock_t* _bound_sockets; // linked list
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;

typedef struct
{
    internalsock_buffer_t* self;
    internalsock_buffer_t* other;
} con_t;

static con_t _get_con(sock_t* sock)
{
    oe_assert(sock && sock->internal.connection);

    const internalsock_connection_side_t other_side =
        sock->internal.side == CONNECTION_CLIENT ? CONNECTION_SERVER
                                                 : CONNECTION_CLIENT;

    return (con_t){
        .self = &sock->internal.connection->buf[sock->internal.side],
        .other = &sock->internal.connection->buf[other_side],
    };
}

static bool _is_internal(const struct oe_sockaddr* addr)
{
    oe_assert(addr);
    return addr->sa_family == OE_AF_INET &&
           oe_ntohl(((struct oe_sockaddr_in*)addr)->sin_addr.s_addr) == _ipaddr;
}

// caller must hold _lock
static internalsock_boundsock_t* _find_bound_socket(uint16_t port)
{
    internalsock_boundsock_t* p = _bound_sockets;
    while (p && p->port != port)
        p = p->next;
    return p;
}

// caller must hold buffer->mutex
// Updates the eventfds of the sockets associated with this buffer so that
// threads waiting on these sockets wake up if data is available to read.
static void _update_events(internalsock_buffer_t* buffer, bool force_notify)
{
    oe_assert(buffer);
    const bool needs_notification =
        force_notify || !oe_ringbuffer_empty(buffer->buf);

    for (size_t i = 0; i < OE_COUNTOF(buffer->socks); ++i)
    {
        sock_t* const sock = buffer->socks[i];
        if (!sock || sock->host_fd < 0)
            continue;

        bool* const notified = &sock->internal.event_notified;

        if (needs_notification)
        {
            // Even if the eventfd is already notified, it must be notified
            // again because the poller might be edge-triggered (see
            // http://man7.org/linux/man-pages/man7/epoll.7.html).
            const int res = oe_host_eventfd_write(sock->host_fd, 1);
            oe_assert(res == 0);
            (void)res;
            *notified = true;
        }
        else if (*notified)
        {
            oe_eventfd_t value = 0;
            const int res = oe_host_eventfd_read(sock->host_fd, &value);
            oe_assert(res == 0 && value > 0);
            (void)res;
            *notified = false;
        }
    }
}

static internalsock_connection_t* _connection_alloc(sock_t* sock)
{
    oe_assert(sock && sock->internal.side == CONNECTION_CLIENT);

    // This is the default write buffer size on Linux.
    // cat /proc/sys/net/ipv4/tcp_wmem
    // Not sure if this is the best value to use here.
    const size_t buffer_size = 16384;

    internalsock_connection_t* const res = oe_calloc(1, sizeof *res);
    if (!res)
        return NULL;

    if (!(res->buf[0].buf = oe_ringbuffer_alloc(buffer_size)))
    {
        oe_free(res);
        return NULL;
    }

    if (!(res->buf[1].buf = oe_ringbuffer_alloc(buffer_size)))
    {
        oe_ringbuffer_free(res->buf[0].buf);
        oe_free(res);
        return NULL;
    }

    res->buf[CONNECTION_CLIENT].refcount = 1;
    res->buf[CONNECTION_CLIENT].socks[0] = sock;
    return res;
}

static void _connection_free(internalsock_connection_t* connection)
{
    if (!connection)
        return;
    oe_ringbuffer_free(connection->buf[0].buf);
    oe_ringbuffer_free(connection->buf[1].buf);
    oe_free(connection);
}

// Adds the socket to the connection's sockets array so that its eventfd can be
// notified. sock->internal.connection must already be set.
static void _connection_add(sock_t* sock, bool increment_refcount)
{
    oe_assert(sock);

    internalsock_connection_t* const connection = sock->internal.connection;
    oe_assert(connection);

    internalsock_buffer_t* const con = &connection->buf[sock->internal.side];

    size_t i = 0;

    oe_spin_lock(&connection->lock);

    oe_mutex_lock(&con->mutex);

    for (; i < OE_COUNTOF(con->socks); ++i)
        if (!con->socks[i])
        {
            con->socks[i] = sock;
            break;
        }

    if (i == OE_COUNTOF(con->socks))
    {
        oe_assert("socks full" == NULL);
        oe_abort();
    }

    oe_mutex_unlock(&con->mutex);

    if (increment_refcount)
        ++con->refcount;

    oe_spin_unlock(&connection->lock);
}

static void _connection_remove(sock_t* sock)
{
    oe_assert(sock);

    internalsock_connection_t* const connection = sock->internal.connection;
    if (!connection)
        return;

    const con_t con = _get_con(sock);

    oe_spin_lock(&connection->lock);
    oe_assert(con.self->refcount > 0);
    --con.self->refcount;
    const bool is_zero = con.self->refcount == 0;
    const bool is_other_zero = con.other->refcount == 0;
    oe_spin_unlock(&connection->lock);

    size_t i = 0;

    oe_mutex_lock(&con.self->mutex);

    for (; i < OE_COUNTOF(con.self->socks); ++i)
        if (con.self->socks[i] == sock)
        {
            con.self->socks[i] = NULL;
            break;
        }

    oe_mutex_unlock(&con.self->mutex);

    oe_assert(i < OE_COUNTOF(con.self->socks));

    if (!is_zero)
        return;

    if (is_other_zero)
    {
        _connection_free(connection);
        return;
    }

    // notify other side of the connection
    oe_mutex_lock(&con.other->mutex);
    oe_cond_broadcast(&con.other->cond);
    _update_events(con.other, true);
    oe_mutex_unlock(&con.other->mutex);
}

static unsigned int _get_refcount(
    internalsock_connection_t* connection,
    internalsock_buffer_t* c)
{
    oe_spin_lock(&connection->lock);
    const unsigned int result = c->refcount;
    oe_spin_unlock(&connection->lock);
    return result;
}

oe_result_t oe_internalsock_bind(sock_t* sock, const struct oe_sockaddr* addr)
{
    oe_assert(sock);
    oe_assert(addr);

    if (!_is_internal(addr))
        return OE_NOT_FOUND;

    // TODO internal sockets should not have host sockets in the first place
    int ret;
    oe_syscall_close_socket_ocall(&ret, sock->host_fd);
    sock->host_fd = -1;

    uint16_t port = oe_ntohs(((struct oe_sockaddr_in*)addr)->sin_port);

    oe_result_t result = OE_FAILURE;
    oe_spin_lock(&_lock);

    if (!port)
    {
        // get unused port
        for (port = 1024; port < OE_UINT16_MAX; ++port)
            if (!_find_bound_socket(port))
                break;

        if (port == OE_UINT16_MAX)
            OE_RAISE_ERRNO(OE_EADDRINUSE);
    }
    else if (_find_bound_socket(port))
        OE_RAISE_ERRNO(OE_EADDRINUSE);

    internalsock_boundsock_t* const bound = oe_calloc(1, sizeof *bound);
    if (!bound)
        OE_RAISE_ERRNO(OE_ENOMEM);

    bound->port = port;

    // add to bound sockets list
    bound->next = _bound_sockets;
    _bound_sockets = bound;

    sock->internal.boundsock = bound;
    sock->base.ops.socket = _sock_ops; // override hostsock ops

    result = OE_OK;

done:
    oe_spin_unlock(&_lock);
    return result;
}

oe_result_t oe_internalsock_connect(
    sock_t* sock,
    const struct oe_sockaddr* addr)
{
    oe_assert(sock);
    oe_assert(addr);

    if (!_is_internal(addr))
        return OE_NOT_FOUND;

    // TODO internal sockets should not have host sockets in the first place
    int ret;
    oe_syscall_close_socket_ocall(&ret, sock->host_fd);
    sock->host_fd = -1;

    const uint16_t port = oe_ntohs(((struct oe_sockaddr_in*)addr)->sin_port);

    oe_result_t result = OE_FAILURE;
    internalsock_boundsock_t* bound = NULL;

    internalsock_connection_t* con = _connection_alloc(sock);

    oe_spin_lock(&_lock);

    if (!con)
        OE_RAISE_ERRNO(OE_ENOMEM);

    bound = _find_bound_socket(port);
    if (!bound)
        OE_RAISE_ERRNO(OE_ECONNREFUSED);

    oe_mutex_lock(&bound->backlog.mutex);

    if (!bound->backlog.buf)
        OE_RAISE_ERRNO(OE_ECONNREFUSED); // not listening

    // add connection to listener's backlog
    const size_t bytes_written =
        oe_ringbuffer_write(bound->backlog.buf, &con, sizeof con);
    if (!bytes_written)
        OE_RAISE_ERRNO(OE_ECONNREFUSED); // backlog full
    oe_assert(bytes_written == sizeof con);

    con->buf[CONNECTION_SERVER].refcount = 1;

    sock->base.ops.socket = _sock_ops; // override hostsock ops
    sock->internal.connection = con;
    con = NULL;

    // notify listener that a new connection is available
    oe_cond_signal(&bound->backlog.cond);
    _update_events(&bound->backlog, false);

    result = OE_OK;

done:
    if (bound)
        oe_mutex_unlock(&bound->backlog.mutex);
    oe_spin_unlock(&_lock);
    _connection_free(con);
    return result;
}

static int _sock_dup(oe_fd_t* sock_, oe_fd_t** new_sock_out)
{
    oe_assert(sock_);
    oe_assert(new_sock_out);

    const sock_t* const sock = (sock_t*)sock_;

    int result = -1;

    // only support connected sockets for now
    if (!sock->internal.connection)
        OE_RAISE_ERRNO(OE_ENOSYS);

    sock_t* const new_sock = oe_malloc(sizeof *new_sock);
    if (!new_sock)
        OE_RAISE_ERRNO(OE_ENOMEM);

    *new_sock = *sock;
    new_sock->host_fd = -1;
    new_sock->internal.event_notified = false;
    _connection_add(new_sock, true);
    *new_sock_out = (oe_fd_t*)new_sock;

    result = 0;

done:
    return result;
}

static int _sock_fcntl(oe_fd_t* sock_, int cmd, uint64_t arg)
{
    oe_assert(sock_);
    sock_t* const sock = (sock_t*)sock_;

    int result = -1;

    switch (cmd)
    {
        case OE_F_GETFL:
            result = sock->internal.flags;
            break;
        case OE_F_SETFL:
            sock->internal.flags = (int)arg;
            result = 0;
            break;
        default:
            OE_RAISE_ERRNO(OE_ENOSYS);
    }

done:
    return result;
}

static ssize_t _sock_read(oe_fd_t* sock_, void* buf, size_t count)
{
    return _sock_recvfrom(sock_, buf, count, 0, NULL, NULL);
}

static ssize_t _sock_write(oe_fd_t* sock_, const void* buf, size_t count)
{
    return _sock_sendto(sock_, buf, count, 0, NULL, 0);
}

static oe_host_fd_t _sock_get_host_fd(oe_fd_t* sock_)
{
    oe_assert(sock_);
    sock_t* const sock = (sock_t*)sock_;

    if (sock->host_fd == -1)
    {
        sock->host_fd = oe_host_eventfd(0, 0);

        if (sock->internal.connection)
            _update_events(
                &sock->internal.connection->buf[sock->internal.side], false);
    }

    return sock->host_fd;
}

static void _free_boundsock(internalsock_boundsock_t* bound)
{
    if (!bound)
        return;

    // remove from linked list of bound sockets
    oe_spin_lock(&_lock);
    for (internalsock_boundsock_t** p = &_bound_sockets; *p; p = &(*p)->next)
        if (*p == bound)
        {
            *p = bound->next;
            break;
        }
    oe_spin_unlock(&_lock);

    if (!bound->backlog.buf)
    {
        oe_free(bound);
        return;
    }

    // free backlog

    internalsock_connection_t* con = NULL;

    oe_mutex_lock(&bound->backlog.mutex);

    size_t bytes_read;
    while (
        (bytes_read = oe_ringbuffer_read(bound->backlog.buf, &con, sizeof con)))
    {
        oe_assert(bytes_read == sizeof con);
        oe_assert(con);

        oe_spin_lock(&con->lock);
        if (con->buf[CONNECTION_CLIENT].refcount)
        {
            con->buf[CONNECTION_SERVER].refcount = 0;
            con = NULL;
        }
        oe_spin_unlock(&con->lock);

        _connection_free(con);
    }

    oe_ringbuffer_free(bound->backlog.buf);

    oe_mutex_unlock(&bound->backlog.mutex);

    oe_free(bound);
}

static int _sock_close(oe_fd_t* sock_)
{
    oe_assert(sock_);
    sock_t* const sock = (sock_t*)sock_;

    // socket can be either bound or connected
    oe_assert(!sock->internal.boundsock != !sock->internal.connection);

    _free_boundsock(sock->internal.boundsock);
    _connection_remove(sock);

    // close eventfd
    if (sock->host_fd >= 0)
    {
        const int res = oe_close_hostfd(sock->host_fd);
        oe_assert(res == 0);
        (void)res;
    }

    oe_free(sock);

    return 0;
}

static oe_fd_t* _sock_accept(
    oe_fd_t* sock_,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen)
{
    oe_assert(sock_);

    oe_fd_t* result = NULL;

    const sock_t* const sock = (sock_t*)sock_;
    internalsock_boundsock_t* const bound = sock->internal.boundsock;
    if (!bound)
        OE_RAISE_ERRNO(OE_EINVAL);

    if ((addr && !addrlen) || (addrlen && !addr))
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!bound->backlog.buf)
        OE_RAISE_ERRNO(OE_EINVAL); // not listening

    const bool block = !(sock->internal.flags & OE_O_NONBLOCK);

    sock_t* const newsock = oe_hostsock_new_sock();
    if (!newsock)
        OE_RAISE_ERRNO(OE_ENOMEM);

    newsock->base.ops.socket = _sock_ops;
    newsock->internal.side = CONNECTION_SERVER;

    internalsock_connection_t* con = NULL;

    oe_mutex_lock(&bound->backlog.mutex);

    // wait for a client
    size_t bytes_read;
    while (!(bytes_read =
                 oe_ringbuffer_read(bound->backlog.buf, &con, sizeof con)) &&
           block)
        oe_cond_wait(&bound->backlog.cond, &bound->backlog.mutex);

    if (bytes_read)
    {
        oe_assert(bytes_read == sizeof con);
        _update_events(&bound->backlog, false);
    }

    oe_mutex_unlock(&bound->backlog.mutex);

    if (!bytes_read)
    {
        oe_errno = OE_EAGAIN;
        return result;
    }

    oe_assert(con);
    newsock->internal.connection = con;
    _connection_add(newsock, false);

    if (addr)
    {
        struct oe_sockaddr_in* const addr_in = (struct oe_sockaddr_in*)addr;
        if (*addrlen >= sizeof *addr_in)
        {
            addr_in->sin_family = OE_AF_INET;
            addr_in->sin_addr.s_addr = oe_htonl(_ipaddr);
            addr_in->sin_port = oe_htons(_client_port);
        }
        *addrlen = sizeof *addr_in;
    }

    result = (oe_fd_t*)newsock;

done:
    return result;
}

static int _sock_listen(oe_fd_t* sock_, int backlog)
{
    oe_assert(sock_);
    sock_t* const sock = (sock_t*)sock_;

    int result = -1;

    internalsock_boundsock_t* const bound = sock->internal.boundsock;
    if (!bound)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (bound->backlog.buf)
        OE_RAISE_ERRNO(OE_EADDRINUSE); // already listening

    if (backlog <= 0)
        backlog = 1;

    oe_ringbuffer_t* const rb = oe_ringbuffer_alloc(
        (size_t)backlog * sizeof(internalsock_connection_t*));
    if (!rb)
        OE_RAISE_ERRNO(OE_ENOMEM);

    bound->backlog.buf = rb;
    bound->backlog.socks[0] = sock;
    result = 0;

done:
    return result;
}

static int _sock_shutdown(oe_fd_t* sock_, int how)
{
    oe_assert(sock_);

    int result = -1;

    switch (how)
    {
        case OE_SHUT_RD:
        case OE_SHUT_WR:
        case OE_SHUT_RDWR:
            break;
        default:
            OE_RAISE_ERRNO(OE_EINVAL);
    }

    if (!((sock_t*)sock_)->internal.connection)
        OE_RAISE_ERRNO(OE_ENOTCONN);

    OE_TRACE_WARNING("not implemented");
    result = 0;

done:
    return result;
}

static int _sock_setsockopt(
    oe_fd_t* sock_,
    int level,
    int optname,
    const void* optval,
    oe_socklen_t optlen)
{
    oe_assert(sock_);
    (void)sock_;
    int result = -1;

    if (level == OE_SOL_SOCKET && optname == OE_SO_KEEPALIVE && optval &&
        optlen == sizeof(int))
    {
        // Setting SO_KEEPALIVE should succeed. Internal sockets have no timeout
        // so we don't have to do anything.
        result = 0;
    }
    else
        OE_RAISE_ERRNO(OE_ENOSYS);

done:
    return result;
}

static int _sock_getsockname(
    oe_fd_t* sock_,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen)
{
    oe_assert(sock_);

    int result = -1;

    if (!addrlen || (*addrlen && !addr))
        OE_RAISE_ERRNO(OE_EFAULT);

    if (addr)
    {
        const sock_t* const sock = (sock_t*)sock_;

        struct oe_sockaddr_in ad = {.sin_family = OE_AF_INET};
        ad.sin_addr.s_addr = oe_htonl(_ipaddr);

        if (sock->internal.boundsock)
            ad.sin_port = oe_htons(sock->internal.boundsock->port);
        else if (sock->internal.connection)
            ad.sin_port = oe_htons(_client_port);

        oe_socklen_t len = *addrlen;
        if (len > sizeof ad)
            len = sizeof ad;
        memcpy(addr, &ad, len);
    }

    *addrlen = sizeof(struct oe_sockaddr_in);
    result = 0;

done:
    return result;
}

static ssize_t _sock_recv(oe_fd_t* sock_, void* buf, size_t count, int flags)
{
    return _sock_recvfrom(sock_, buf, count, flags, NULL, NULL);
}

static ssize_t _sock_send(
    oe_fd_t* sock_,
    const void* buf,
    size_t count,
    int flags)
{
    return _sock_sendto(sock_, buf, count, flags, NULL, 0);
}

static ssize_t _sock_recvfrom(
    oe_fd_t* sock_,
    void* buf,
    size_t count,
    int flags,
    struct oe_sockaddr* src_addr,
    oe_socklen_t* addrlen)
{
    oe_assert(sock_);

    if (!count)
        return 0;

    ssize_t result = -1;

    if (!buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (flags || src_addr || addrlen)
        OE_RAISE_ERRNO(OE_ENOSYS); // not supported yet

    if (count > OE_SSIZE_MAX)
        count = OE_SSIZE_MAX;

    sock_t* const sock = (sock_t*)sock_;
    const con_t con = _get_con(sock);
    const bool block = !(sock->internal.flags & OE_O_NONBLOCK);

    oe_mutex_lock(&con.self->mutex);

    size_t bytes_read;
    while (!(bytes_read = oe_ringbuffer_read(con.self->buf, buf, count)) &&
           block && _get_refcount(sock->internal.connection, con.other))
        oe_cond_wait(&con.self->cond, &con.self->mutex);

    if (bytes_read)
    {
        oe_cond_broadcast(&con.self->cond);
        _update_events(con.self, false);
        oe_assert(bytes_read <= OE_SSIZE_MAX);
        result = (ssize_t)bytes_read;
    }
    else if (!block && _get_refcount(sock->internal.connection, con.other))
        oe_errno = OE_EAGAIN;
    else
        result = 0;

    oe_mutex_unlock(&con.self->mutex);

done:
    return result;
}

static ssize_t _sock_sendto(
    oe_fd_t* sock_,
    const void* buf,
    size_t count,
    int flags,
    const struct oe_sockaddr* dest_addr,
    oe_socklen_t addrlen)
{
    oe_assert(sock_);
    sock_t* const sock = (sock_t*)sock_;
    const con_t con = _get_con(sock);

    if (count > OE_SSIZE_MAX)
        count = OE_SSIZE_MAX;

    size_t written = 0;
    ssize_t result = -1;

    oe_mutex_lock(&con.other->mutex);

    if (!buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (dest_addr || addrlen)
        OE_RAISE_ERRNO(OE_EISCONN);

    if (flags)
        OE_RAISE_ERRNO(OE_ENOSYS); // not supported yet

    for (;;)
    {
        if (!_get_refcount(sock->internal.connection, con.other))
            OE_RAISE_ERRNO(OE_EPIPE);
        written += oe_ringbuffer_write(
            con.other->buf, (uint8_t*)buf + written, count - written);
        oe_cond_broadcast(&con.other->cond);
        _update_events(con.other, false);
        if (written == count)
            break;
        oe_cond_wait(&con.other->cond, &con.other->mutex);
    }

    result = (ssize_t)count;

done:
    oe_mutex_unlock(&con.other->mutex);
    return result;
}
