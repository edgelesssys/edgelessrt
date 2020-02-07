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
    } internal;
} sock_t;

sock_t* oe_hostsock_new_sock(void);

oe_result_t oe_internalsock_bind(sock_t* sock, const struct oe_sockaddr* addr);
oe_result_t oe_internalsock_connect(
    sock_t* sock,
    const struct oe_sockaddr* addr);
