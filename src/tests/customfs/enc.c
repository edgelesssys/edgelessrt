#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/uio.h>

// mocked functions simulate a single file
static char _filebuf[27];
static size_t _offset;

// dummy values
static void* const _handle = (void*)2;
static void* const _context_ro = (void*)3;
static void* const _context_rw = (void*)4;

static int _fs_open(
    void* context,
    const char* pathname,
    int flags,
    unsigned int mode,
    void** handle_fs,
    void** handle)
{
    (void)flags;
    (void)mode;
    (void)handle_fs;
    OE_TEST(context == _context_ro || context == _context_rw);
    OE_TEST(strcmp(pathname, "/foo") == 0);
    OE_TEST(handle);
    _offset = 0;
    *handle = _handle;
    return 0;
}

static int _fs_close(void* context, void* handle)
{
    OE_TEST(context == _context_ro || context == _context_rw);
    OE_TEST(handle == _handle);
    return 0;
}

static ssize_t _fs_read(void* context, void* handle, void* buf, size_t count)
{
    OE_TEST(context == _context_ro || context == _context_rw);
    OE_TEST(handle == _handle);
    OE_TEST(buf);
    OE_TEST(count > 0);
    const size_t max = sizeof _filebuf - _offset;
    if (count > max)
        count = max;
    memcpy(buf, _filebuf + _offset, count);
    _offset += count;
    return count;
}

static ssize_t _v(void* handle, const void* iov, int iovcnt, bool write)
{
    OE_TEST(handle == _handle);
    OE_TEST(iov);
    OE_TEST(iovcnt > 0);

    const struct iovec* const v = (struct iovec*)iov;
    ssize_t result = 0;

    for (int i = 0; i < iovcnt; ++i)
    {
        const size_t len = v[i].iov_len;
        if (_offset + len > sizeof _filebuf)
            break;

        if (write)
            memcpy(_filebuf + _offset, v[i].iov_base, len);
        else
            memcpy(v[i].iov_base, _filebuf + _offset, len);

        _offset += len;
        result += len;
    }

    return result;
}

static ssize_t _fs_readv(
    void* context,
    void* handle,
    const void* iov,
    int iovcnt)
{
    OE_TEST(context == _context_ro || context == _context_rw);
    return _v(handle, iov, iovcnt, false);
}

static ssize_t _fs_writev(
    void* context,
    void* handle,
    const void* iov,
    int iovcnt)
{
    OE_TEST(context == _context_rw);
    return _v(handle, iov, iovcnt, true);
}

static int _fs_fstat(void* context, void* handle, void* statbuf)
{
    OE_TEST(context == _context_ro || context == _context_rw);
    OE_TEST(handle == _handle);
    OE_TEST(statbuf);
    struct stat* const buf = (struct stat*)statbuf;
    *buf = (struct stat){0};
    buf->st_size = sizeof _filebuf;
    return 0;
}

void test_ecall(void)
{
    extern int run_main(const char* path, bool readonly);

    const char* const rodev = "rodev";
    const char* const rwdev = "rwdev";

    oe_customfs_t rofs = {
        .open = _fs_open,
        .close = _fs_close,
        .read = _fs_read,
        .readv = _fs_readv,
        .fstat = _fs_fstat,
    };

    oe_customfs_t rwfs = {
        .open = _fs_open,
        .close = _fs_close,
        .read = _fs_read,
        .readv = _fs_readv,
        .writev = _fs_writev,
        .fstat = _fs_fstat,
    };

    OE_TEST(oe_load_module_custom_file_system(rodev, &rofs, _context_ro) > 0);
    OE_TEST(mount("/", "/ro", rodev, MS_RDONLY, NULL) == 0);
    OE_TEST(oe_load_module_custom_file_system(rwdev, &rwfs, _context_rw) > 0);
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
