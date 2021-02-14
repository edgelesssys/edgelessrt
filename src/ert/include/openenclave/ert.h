// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

/**
 * @file ert.h
 */
#pragma once

#include <openenclave/enclave.h>
#include <openenclave/ert_args.h>

OE_EXTERNC_BEGIN

/**
 * The enclave entry point used by erthost.
 *
 * The enclave can implement this function instead of linking against one of the
 * ert*entry libraries.
 *
 * @return Exit code that erthost should return to the system.
 */
int emain(void);

/**
 * Get pointers to commandline arguments, environment variables, and auxiliary
 * vector from host.
 *
 * The caller is responsible to securely copy the arrays to enclave memory. This
 * can be done using ert_copy_strings_from_host_to_enclave().
 *
 * @param[out] retval Pointer to an ert_args_t object that will be filled with
 * pointers to host memory.
 *
 * @return OE_OK if the ocall succeeded
 */
oe_result_t ert_get_args_ocall(ert_args_t* retval);

/**
 * Securely deep-copy an array of strings from the host to the enclave.
 *
 * @param host_array An array in host memory.
 * @param[out] enclave_array An array that will be allocated on the enclave
 * heap. The array will include an additional terminating nullptr element. Free
 * with free().
 * @param count Number of elements to copy.
 */
void ert_copy_strings_from_host_to_enclave(
    const char* const* host_array,
    char*** enclave_array,
    size_t count);

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

#ifdef __cplusplus

#include <memory>
#include <string>
#include <utility>

namespace ert
{
namespace memfs
{
class Filesystem;
}

/**
 * In-enclave-memory filesystem.
 *
 * Usage:
 *
 * const Memfs memfs("my_fs");
 * mount("/", "/my/mount/point", "my_fs", 0, nullptr);
 *
 * Folders are not supported for now. The app may open a file by passing a full
 * path and the fs behaves as if all folders in the path exist.
 */
class Memfs
{
  public:
    Memfs(const std::string& devname);
    ~Memfs();
    Memfs(const Memfs&) = delete;
    Memfs& operator=(const Memfs&) = delete;

  private:
    const std::unique_ptr<memfs::Filesystem> impl_;
    oe_customfs_t ops_;
    uint64_t devid_;

    static memfs::Filesystem& to_fs(void* context);
};

/**
 * An enclave can optionally contain a second executable image that we call the
 * payload.
 */
namespace payload
{
/**
 * Get the base address of the payload image.
 *
 * @return base address
 */
const void* get_base() noexcept;

/**
 * Apply relocations to the payload image. The main image must call this
 * function before calling the payload's entry point.
 *
 * @param start_main The payload's libc_start_main will be relocated to this
 * function.
 *
 * @throw logic_error relocation failed
 */
void apply_relocations(void start_main(int payload_main(...)));

/**
 * Get the payload data.
 *
 * @return payload data
 */
std::pair<const void*, size_t> get_data() noexcept;
} // namespace payload

} // namespace ert

#endif // __cplusplus
