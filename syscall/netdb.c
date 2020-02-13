// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/internal/syscall/arpa/inet.h>
#include <openenclave/internal/syscall/netdb.h>
#include <openenclave/internal/syscall/raise.h>
#include <openenclave/internal/syscall/resolver.h>
#include <openenclave/internal/syscall/sys/socket.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/trace.h>

static oe_resolver_t* _resolver;
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;
static bool _installed_atexit_handler = false;

static void _atexit_handler(void)
{
    if (_resolver)
        _resolver->ops->release(_resolver);
}

/* Called by the public oe_load_module_host_resolver() function. */
int oe_register_resolver(oe_resolver_t* resolver)
{
    int ret = -1;
    bool locked = false;

    /* Check parameters. */
    if (!resolver || !resolver->ops || !resolver->ops->getaddrinfo ||
        !resolver->ops->getnameinfo)
    {
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    oe_spin_lock(&_lock);
    locked = true;

    /* This function can be called only once. */
    if (_resolver != NULL)
        OE_RAISE_ERRNO(OE_EINVAL);

    _resolver = resolver;

    if (!_installed_atexit_handler)
    {
        oe_atexit(_atexit_handler);
        _installed_atexit_handler = true;
    }

    ret = 0;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

// copied from musl/network/lookup.h
struct address
{
    int family;
    unsigned scopeid;
    uint8_t addr[16];
    int sortkey;
};
struct service
{
    uint16_t port;
    unsigned char proto, socktype;
};

// copied from musl/network/lookup_name.c
static int name_from_null(
    struct address buf[static 2],
    const char* name,
    int family,
    int flags)
{
    int cnt = 0;
    if (name)
        return 0;
    if (flags & OE_AI_PASSIVE)
    {
        if (family != OE_AF_INET6)
            buf[cnt++] = (struct address){.family = OE_AF_INET};
        if (family != OE_AF_INET)
            buf[cnt++] = (struct address){.family = OE_AF_INET6};
    }
    else
    {
        if (family != OE_AF_INET6)
            buf[cnt++] =
                (struct address){.family = OE_AF_INET, .addr = {127, 0, 0, 1}};
        if (family != OE_AF_INET)
            buf[cnt++] =
                (struct address){.family = OE_AF_INET6, .addr = {[15] = 1}};
    }
    return cnt;
}

// adapted from musl/network/lookup_name.c
// - Do not try to look up name in /etc/hosts and fail with OE_EAI_FAIL if the
// name cannot be resolved internally.
// - Do not sort results because it would require host calls.
static int _lookup_name(
    struct address buf[static 2],
    char canon[static 256],
    const char* name,
    int family,
    int flags)
{
    size_t strnlen(const char*, size_t);
    int __lookup_ipliteral(
        struct address buf[static 1], const char* name, int family);

    int cnt = 0, i, j;

    *canon = 0;
    if (name)
    {
        /* reject empty name and check len so it fits into temp bufs */
        size_t l = strnlen(name, 255);
        if (l - 1 >= 254)
            return OE_EAI_NONAME;
        memcpy(canon, name, l + 1);
    }

    /* Procedurally, a request for v6 addresses with the v4-mapped
     * flag set is like a request for unspecified family, followed
     * by filtering of the results. */
    if (flags & OE_AI_V4MAPPED)
    {
        if (family == OE_AF_INET6)
            family = OE_AF_UNSPEC;
        else
            flags -= OE_AI_V4MAPPED;
    }

    /* Try each backend until there's at least one result. */
    cnt = name_from_null(buf, name, family, flags);
    if (!cnt)
        cnt = __lookup_ipliteral(buf, name, family);
    if (!cnt && !(flags & OE_AI_NUMERICHOST))
    {
        // EDG: cannot be resolved internally (do not try /etc/hosts)
        return OE_EAI_FAIL;
    }
    if (cnt <= 0)
        return cnt ? cnt : OE_EAI_NONAME;

    /* Filter/transform results for v4-mapped lookup, if requested. */
    if (flags & OE_AI_V4MAPPED)
    {
        if (!(flags & OE_AI_ALL))
        {
            /* If any v6 results exist, remove v4 results. */
            for (i = 0; i < cnt && buf[i].family != OE_AF_INET6; i++)
                ;
            if (i < cnt)
            {
                for (j = 0; i < cnt; i++)
                {
                    if (buf[i].family == OE_AF_INET6)
                        buf[j++] = buf[i];
                }
                cnt = i = j;
            }
        }
        /* Translate any remaining v4 results to v6 */
        for (i = 0; i < cnt; i++)
        {
            if (buf[i].family != OE_AF_INET)
                continue;
            memcpy(buf[i].addr + 12, buf[i].addr, 4);
            memcpy(buf[i].addr, "\0\0\0\0\0\0\0\0\0\0\xff\xff", 12);
            buf[i].family = OE_AF_INET6;
        }
    }

    return cnt;
}

// adapted from musl/network/lookup_serv.c
// - do not try to look up name in /etc/services
static int _lookup_serv(
    struct service buf[static 2],
    const char* name,
    int proto,
    int socktype,
    int flags)
{
    int cnt = 0;
    char* z = "";
    unsigned long port = 0;

    switch (socktype)
    {
        case OE_SOCK_STREAM:
            switch (proto)
            {
                case 0:
                    proto = IPPROTO_TCP;
                case IPPROTO_TCP:
                    break;
                default:
                    return OE_EAI_SERVICE;
            }
            break;
        case OE_SOCK_DGRAM:
            switch (proto)
            {
                case 0:
                    proto = IPPROTO_UDP;
                case IPPROTO_UDP:
                    break;
                default:
                    return OE_EAI_SERVICE;
            }
        case 0:
            break;
        default:
            if (name)
                return OE_EAI_SERVICE;
            buf[0].port = 0;
            buf[0].proto = (unsigned char)proto;
            buf[0].socktype = (unsigned char)socktype;
            return 1;
    }

    if (name)
    {
        if (!*name)
            return OE_EAI_SERVICE;
        port = oe_strtoul(name, &z, 10);
    }
    if (!*z)
    {
        if (port > 65535)
            return OE_EAI_SERVICE;
        if (proto != IPPROTO_UDP)
        {
            buf[cnt].port = (uint16_t)port;
            buf[cnt].socktype = OE_SOCK_STREAM;
            buf[cnt++].proto = IPPROTO_TCP;
        }
        if (proto != IPPROTO_TCP)
        {
            buf[cnt].port = (uint16_t)port;
            buf[cnt].socktype = OE_SOCK_DGRAM;
            buf[cnt++].proto = IPPROTO_UDP;
        }
        return cnt;
    }

    if (flags & OE_AI_NUMERICSERV)
        return OE_EAI_NONAME;

    return OE_EAI_SERVICE;
}

// adapted from musl/network/getaddrinfo.c
// This function fails with OE_EAI_FAIL if the name cannot be resolved
// internally, i.e., is not an IP address. In this case the request may be
// forwarded to the host. On other errors the request must not be forwarded to
// the host. Consider these examples:
// 255.0.0.1:80 succeeds.
// myhost:80 fails with OE_EAI_FAIL and may be resolved by the host.
// myhost:http fails with OE_EAI_FAIL and may be resolved by the host.
// 255.0.0.1:http fails with OE_EAI_SERVICE and must not be resolved by the host
// because it could return another IP address. We could add additional logic if
// we ever need to resolve this combination of host/service.
static int _getaddrinfo(
    const char* restrict host,
    const char* restrict serv,
    const struct oe_addrinfo* restrict hint,
    struct oe_addrinfo** restrict res)
{
    struct service ports[2];
    struct address addrs[2];
    char canon[256];
    int nservs, naddrs, i, j;
    int family = OE_AF_UNSPEC, flags = 0, proto = 0, socktype = 0;

    if (!host && !serv)
        return OE_EAI_NONAME;

    if (hint)
    {
        family = hint->ai_family;
        flags = hint->ai_flags;
        proto = hint->ai_protocol;
        socktype = hint->ai_socktype;

        const int mask = OE_AI_PASSIVE | OE_AI_CANONNAME | OE_AI_NUMERICHOST |
                         OE_AI_V4MAPPED | OE_AI_ALL | OE_AI_ADDRCONFIG |
                         OE_AI_NUMERICSERV;
        if ((flags & mask) != flags)
            return OE_EAI_BADFLAGS;

        switch (family)
        {
            case OE_AF_INET:
            case OE_AF_INET6:
            case OE_AF_UNSPEC:
                break;
            default:
                return OE_EAI_FAMILY;
        }
    }

    // EDG: Ignore AI_ADDRCONFIG flag because it would require to create a host
    // socket.

    // EDG: Look up name first so that something like myhost:http fails with
    // OE_EAI_FAIL instead of OE_EAI_SERVICE and, thus, will be forwarded to the
    // host.
    naddrs = _lookup_name(addrs, canon, host, family, flags);
    if (naddrs < 0)
        return naddrs;

    nservs = _lookup_serv(ports, serv, proto, socktype, flags);
    if (nservs < 0)
        return nservs;

    // EDG: Memory allocation for the result differs from the musl
    // implementation such that it is compatible with the oe_freeaddrinfo
    // implementation.

    const size_t canon_len = oe_strlen(canon);

    struct oe_addrinfo* head = NULL;
    struct oe_addrinfo** next = &head;

    for (i = 0; i < naddrs; i++)
        for (j = 0; j < nservs; j++)
        {
            struct oe_addrinfo* const ai = oe_malloc(sizeof *ai);
            if (!ai)
            {
                oe_freeaddrinfo(head);
                return OE_EAI_MEMORY;
            }
            *next = ai;
            next = &ai->ai_next;

            *ai = (struct oe_addrinfo){
                .ai_family = addrs[i].family,
                .ai_socktype = ports[j].socktype,
                .ai_protocol = ports[j].proto,
                .ai_addrlen = addrs[i].family == OE_AF_INET
                                  ? sizeof(struct oe_sockaddr_in)
                                  : sizeof(struct oe_sockaddr_in6),
                .ai_addr = NULL,
                .ai_canonname = NULL,
                .ai_next = NULL};

            ai->ai_addr = oe_calloc(1, ai->ai_addrlen);
            if (!ai->ai_addr)
            {
                oe_freeaddrinfo(head);
                return OE_EAI_MEMORY;
            }

            switch (addrs[i].family)
            {
                case OE_AF_INET:
                {
                    struct oe_sockaddr_in* const sin =
                        (struct oe_sockaddr_in*)ai->ai_addr;
                    sin->sin_family = OE_AF_INET;
                    sin->sin_port = oe_htons(ports[j].port);
                    memcpy(&sin->sin_addr, &addrs[i].addr, 4);
                }
                break;
                case OE_AF_INET6:
                {
                    struct oe_sockaddr_in6* const sin6 =
                        (struct oe_sockaddr_in6*)ai->ai_addr;
                    sin6->sin6_family = OE_AF_INET6;
                    sin6->sin6_port = oe_htons(ports[j].port);
                    sin6->sin6_scope_id = addrs[i].scopeid;
                    memcpy(&sin6->sin6_addr, &addrs[i].addr, 16);
                }
                break;
            }

            if (canon_len)
            {
                char* const cn = oe_malloc(canon_len + 1);
                if (!cn)
                {
                    oe_freeaddrinfo(head);
                    return OE_EAI_MEMORY;
                }
                memcpy(cn, canon, canon_len + 1);
                ai->ai_canonname = cn;
            }
        }
    *res = head;
    return 0;
}

int oe_getaddrinfo(
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo** res_out)
{
    int ret = OE_EAI_FAIL;
    struct oe_addrinfo* res;
    bool locked = false;

    if (res_out)
        *res_out = NULL;
    else
        OE_RAISE_ERRNO(OE_EINVAL);

    // EDG: Try to resolve internally. This is important for internal sockets
    // because the host could otherwise hijack them.
    const int ret_getaddrinfo = _getaddrinfo(node, service, hints, res_out);
    if (ret_getaddrinfo != OE_EAI_FAIL)
        return ret_getaddrinfo;

    oe_spin_lock(&_lock);
    locked = true;

    if (!_resolver)
    {
        ret = OE_EAI_SYSTEM;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    ret = (_resolver->ops->getaddrinfo)(_resolver, node, service, hints, &res);

    if (ret == 0)
        *res_out = res;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

int oe_getnameinfo(
    const struct oe_sockaddr* sa,
    oe_socklen_t salen,
    char* host,
    oe_socklen_t hostlen,
    char* serv,
    oe_socklen_t servlen,
    int flags)
{
    ssize_t ret = OE_EAI_FAIL;
    bool locked = false;

    oe_spin_lock(&_lock);
    locked = true;

    if (!_resolver)
    {
        ret = OE_EAI_SYSTEM;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    ret = (*_resolver->ops->getnameinfo)(
        _resolver, sa, salen, host, hostlen, serv, servlen, flags);

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return (int)ret;
}

void oe_freeaddrinfo(struct oe_addrinfo* res)
{
    struct oe_addrinfo* p;

    for (p = res; p;)
    {
        struct oe_addrinfo* next = p->ai_next;

        oe_free(p->ai_addr);
        oe_free(p->ai_canonname);
        oe_free(p);

        p = next;
    }
}
