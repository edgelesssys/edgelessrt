// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_ENCLAVELIBC_H
#define _OE_ENCLAVELIBC_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/**
 * Enclave implementation of the standard strlen() function.
 *
 * Refer to documentation for strlen() function.
 */
size_t oe_strlen(const char* s);

/**
 * Enclave implementation of the standard strnlen() function.
 *
 * Refer to documentation for strnlen() function.
 */
size_t oe_strnlen(const char* s, size_t n);

/**
 * Enclave implementation of the standard strcmp() function.
 *
 * Refer to documentation for strcmp() function.
 */
int oe_strcmp(const char* s1, const char* s2);

/**
 * Enclave implementation of the standard strncmp() function.
 *
 * Refer to documentation for strncmp() function.
 */
int oe_strncmp(const char* s1, const char* s2, size_t n);

/**
 * Enclave implementation of the standard strlcpy() function.
 *
 * Refer to documentation for strlcpy() function.
 */
size_t oe_strlcpy(char* dest, const char* src, size_t size);

/**
 * Enclave implementation of the standard strlcat() function.
 *
 * Refer to documentation for strlcat() function.
 */
size_t oe_strlcat(char* dest, const char* src, size_t size);

/**
 * Enclave implementation of the standard memcpy() function.
 *
 * Refer to documentation for memcpy() function.
 */
void* oe_memcpy(void* dest, const void* src, size_t n);

/**
 * Enclave implementation of the standard memset() function.
 *
 * Refer to documentation for memset() function.
 */
void* oe_memset(void* s, int c, size_t n);

/**
 * Enclave implementation of the standard memcmp() function.
 *
 * Refer to documentation for memcmp() function.
 */
int oe_memcmp(const void* s1, const void* s2, size_t n);

/**
 * Produce output according to a given format string.
 *
 * This function is similar to vsnprintf() but has limited support for format
 * types. It only supports the following without width specifiers.
 *     - "%s"
 *     - "%u"
 *     - "%d"
 *     - "%x"
 *     - "%lu"
 *     - "%ld"
 *     - "%lx"
 *     - "%zu"
 *     - "%zd"
 *     - "%p"
 *
 * @param str Write output to this string
 * @param size The size of **str** parameter.
 * @param fmt The limited printf style format.
 *
 * @returns The number of characters that would be written excluding the
 * zero-terminator. If this value is greater or equal to **size**, then the
 * string was truncated.
 *
 */
int oe_vsnprintf(char* str, size_t size, const char* fmt, oe_va_list ap);

/**
 * Produce output according to a given format string.
 *
 * This function is similar to snprintf() but has limited support for format
 * types. See oe_vsnprintf() for details on these limits.
 *
 * @param str Write output to this string.
 * @param size The size of **str** parameter.
 * @param fmt The limited printf style format.
 *
 * @returns The number of characters that would be written excluding the
 * zero-terminator. If this value is greater or equal to **size**, then the
 * string was truncated.
 *
 */
OE_PRINTF_FORMAT(3, 4)
int oe_snprintf(char* str, size_t size, const char* fmt, ...);

/**
 * Allocates space on the stack frame of the caller.
 *
 * This function allocates **size** bytes of space on the stack frame of the
 * caller. The returned address will be a multiple of **alignment** (if
 * non-zero). The allocated space is automatically freed when the calling
 * function returns. If the stack overflows, the behavior is undefined.
 *
 * @param size The number of bytes to allocate.
 * @param alignment The alignment requirement (see above).
 *
 * @returns Returns the address of the allocated space.
 *
 */
OE_ALWAYS_INLINE OE_INLINE void* oe_stack_alloc(size_t size, size_t alignment)
{
    void* ptr = __builtin_alloca(size + alignment);

    if (alignment)
        ptr = (void*)(((uint64_t)ptr + alignment - 1) / alignment * alignment);

    return ptr;
}

OE_EXTERNC_END

#endif /* _OE_ENCLAVELIBC_H */