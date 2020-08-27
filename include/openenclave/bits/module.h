// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

/**
 * @file module.h
 *
 * This file defines functions to load the optional modules available.
 *
 */
#ifndef _OE_BITS_MODULE_H
#define _OE_BITS_MODULE_H

/*
**==============================================================================
**
** This file defines functions for loading internal modules that are part of
** the Open Enclave core.
**
**==============================================================================
*/

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/**
 * Load the host file system module.
 *
 * This function loads the host file system module
 * which is needed for an enclave application to perform operations
 * such as mount, fopen, fread and fwrite on files located on the host.
 *
 * @retval OE_OK The module was successfully loaded.
 * @retval OE_FAILURE Module failed to load.
 *
 */
oe_result_t oe_load_module_host_file_system(void);

/**
 * Load the host socket interface module.
 *
 * This function loads the host socket interface module
 * which is needed for an enclave application to be able to call socket APIs
 * which are routed through the host.
 *
 * @retval OE_OK The module was successfully loaded.
 * @retval OE_FAILURE Module failed to load.
 *
 */
oe_result_t oe_load_module_host_socket_interface(void);

/**
 * Load the host resolver module.
 *
 * This function loads the host resolver module which is needed
 * for an enclave application to be able to call
 * getaddrinfo and getnameinfo.
 *
 * @retval OE_OK The module was successfully loaded.
 * @retval OE_FAILURE Module failed to load.
 */
oe_result_t oe_load_module_host_resolver(void);

/**
 * Load the event polling (epoll) module.
 *
 * This function loads the host epoll module which is needed
 * for an enclave application to be able to call
 * epoll_create1, epoll_ctl, and epoll_wait
 *
 * @retval OE_OK The module was successfully loaded.
 * @retval OE_FAILURE Module failed to load.
 */
oe_result_t oe_load_module_host_epoll(void);

typedef struct _oe_customfs
{
    uint8_t reserved[4248];
    uintptr_t (*open)(void* context, const char* path, bool must_exist);
    void (*close)(void* context, uintptr_t handle);
    uint64_t (*get_size)(void* context, uintptr_t handle);
    void (*unlink)(void* context, const char* path);
    void (*read)(
        void* context,
        uintptr_t handle,
        void* buf,
        uint64_t count,
        uint64_t offset);
    bool (*write)(
        void* context,
        uintptr_t handle,
        const void* buf,
        uint64_t count,
        uint64_t offset);
} oe_customfs_t;

/**
 * Load a custom file system.
 *
 * The enclave application must implement the functions defined in
 * oe_customfs_t. Folders are not supported for now.
 *
 * @param devname An arbitrary but unique device name. The same name must be
 * passed to mount().
 * @param ops Pointer to a struct that contains the file operation function
 * pointers. Its memory must not be modified or freed.
 * @param context An arbitrary value that is passed to the file operation
 * functions.
 *
 * @retval device_id The module was successfully loaded.
 * @retval 0 Module failed to load.
 *
 */
uint64_t oe_load_module_custom_file_system(
    const char* devname,
    oe_customfs_t* ops,
    void* context);

OE_EXTERNC_END

#endif /* _OE_BITS_MODULE_H */
