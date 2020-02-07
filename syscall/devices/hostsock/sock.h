// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/result.h>
#include <openenclave/internal/ringbuffer.h>
#include <openenclave/internal/syscall/fd.h>
#include <openenclave/internal/syscall/sys/socket.h>
#include <openenclave/internal/syscall/types.h>
#include <openenclave/internal/thread.h>
#include <stdint.h>

typedef struct _internalsock_buffer
{
    unsigned int refcount;
    oe_ringbuffer_t* buf;
    oe_mutex_t mutex;
    oe_cond_t cond;

    // This array contains either client or server or bound sockets, but not
    // mixed types. Usually it contains only one socket, but there may be
    // dup()ed sockets. The array size can be increased or made dynamic if we
    // ever need to support more sockets.
    struct _sock* socks[2];
} internalsock_buffer_t;

typedef struct _internalsock_boundsock
{
    uint16_t port;
    internalsock_buffer_t backlog;
    struct _internalsock_boundsock* next;
} internalsock_boundsock_t;

typedef enum
{
    CONNECTION_CLIENT,
    CONNECTION_SERVER
} internalsock_connection_side_t;

typedef struct _internalsock_connection
{
    internalsock_buffer_t buf[2];
    oe_spinlock_t lock;
} internalsock_connection_t;

typedef struct _sock
{
    oe_fd_t base;
    uint32_t magic;
    oe_host_fd_t host_fd;

    struct
    {
        // used if the socket is bound internally; otherwise null
        internalsock_boundsock_t* boundsock;

        // used if the socket is connected internally; otherwise null
        internalsock_connection_t* connection;

        internalsock_connection_side_t side; // client or server
        int flags;                           // set by fcntl()
        bool event_notified; // state of the eventfd referred by host_fd
    } internal;
} sock_t;

sock_t* oe_hostsock_new_sock(void);

oe_result_t oe_internalsock_bind(sock_t* sock, const struct oe_sockaddr* addr);
oe_result_t oe_internalsock_connect(
    sock_t* sock,
    const struct oe_sockaddr* addr);
