// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#ifndef _OE_BITS_PTHREAD_ATTR_H
#define _OE_BITS_PTHREAD_ATTR_H

OE_INLINE
int pthread_attr_init(pthread_attr_t* attr)
{
    return oe_pthread_attr_init((oe_pthread_attr_t*)attr);
}

OE_INLINE
int pthread_attr_destroy(pthread_attr_t* attr)
{
    return oe_pthread_attr_destroy((oe_pthread_attr_t*)attr);
}

OE_INLINE
int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate)
{
    return oe_pthread_attr_setdetachstate(
        (oe_pthread_attr_t*)attr, detachstate);
}

#endif /* _OE_BITS_PTHREAD_ATTR_H */
