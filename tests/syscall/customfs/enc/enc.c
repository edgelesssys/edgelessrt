#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <string.h>
#include <sys/mount.h>

static char _filebuf[27];

static uintptr_t _fs_open(const char* path, bool must_exist)
{
    (void)must_exist;
    OE_TEST(strcmp(path, "/foo") == 0);
    return 1;
}

static void _fs_close(uintptr_t handle)
{
    OE_TEST(handle == 1);
}

static uint64_t _fs_get_size(uintptr_t handle)
{
    OE_TEST(handle == 1);
    return sizeof _filebuf;
}

static void _fs_unlink(const char* path)
{
    OE_TEST(strcmp(path, "/foo") == 0);
}

static void _fs_read(
    uintptr_t handle,
    void* buf,
    uint64_t count,
    uint64_t offset)
{
    OE_TEST(handle == 1);
    OE_TEST(offset + count <= sizeof _filebuf);
    memcpy(buf, _filebuf + offset, count);
}

static bool _fs_write(
    uintptr_t handle,
    const void* buf,
    uint64_t count,
    uint64_t offset)
{
    OE_TEST(handle == 1);
    OE_TEST(offset + count <= sizeof _filebuf);
    memcpy(_filebuf + offset, buf, count);
    return true;
}

void test_customfs(void)
{
    extern int run_main(const char* path, bool readonly);

    const char* const rodev = "rodev";
    const char* const rwdev = "rwdev";

    oe_customfs_t rofs = {
        .open = _fs_open,
        .close = _fs_close,
        .get_size = _fs_get_size,
        .read = _fs_read,
    };

    oe_customfs_t rwfs = {
        .open = _fs_open,
        .close = _fs_close,
        .get_size = _fs_get_size,
        .unlink = _fs_unlink,
        .read = _fs_read,
        .write = _fs_write,
    };

    OE_TEST(oe_load_module_custom_file_system(rodev, &rofs) == OE_OK);
    OE_TEST(mount("/", "/ro", rodev, MS_RDONLY, NULL) == 0);
    OE_TEST(oe_load_module_custom_file_system(rwdev, &rwfs) == OE_OK);
    OE_TEST(mount("/", "/rw", rwdev, 0, NULL) == 0);
    OE_TEST(run_main("/rw/foo", false) == 0);
    OE_TEST(run_main("/ro/foo", true) == 0);
    OE_TEST(umount("/ro") == 0);
    OE_TEST(umount("/rw") == 0);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    1024, /* NumHeapPages */
    1024, /* NumStackPages */
    2);   /* NumTCS */
