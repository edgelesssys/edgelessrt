#include <openenclave/ert.h>
#include <sys/mount.h>
#include <cstdio>
#include <limits>
#include <string>
#include "../ert/ertlibc/memfs/file.h"
#include "../ert/ertlibc/memfs/filesystem.h"
#include "../tests_ex.h"
#include "test_t.h"

using namespace std;
using namespace ert;
using namespace ert::memfs;

static void _test_file()
{
    File f;
    OE_TEST(f.size() == 0);
    string buf(4, 'A');

    // try to read from empty file
    ASSERT_NO_THROW(f.read(nullptr, 0, 0));
    ASSERT_NO_THROW(f.read(buf.data(), 0, 0));
    ASSERT_THROW(f.read(buf.data(), 1, 0), out_of_range);
    ASSERT_THROW(f.read(nullptr, 0, 1), out_of_range);
    ASSERT_THROW(f.read(buf.data(), 0, 1), out_of_range);
    OE_TEST(buf == "AAAA");

    // write to file
    f.write(nullptr, 0, 0);
    f.write("foo", 0, 0);
    OE_TEST(f.size() == 0);
    f.write("foo", 3, 1);
    OE_TEST(f.size() == 4);

    // read from file
    f.read(buf.data(), 4, 0);
    OE_TEST(buf == "\0foo"s);
}

static void _test_filesystem()
{
    Filesystem fs;

    // try to access nonexistent files
    OE_TEST(!fs.open("foo", true));
    ASSERT_THROW(fs.get_file(1), out_of_range);

    // create new file
    auto handle = fs.open("foo", false);
    OE_TEST(handle);
    OE_TEST(fs.get_file(handle));
    fs.close(handle);
    ASSERT_THROW(fs.get_file(handle), out_of_range);

    // open existing file
    handle = fs.open("foo", true);
    OE_TEST(handle);
    fs.close(handle);
    fs.unlink("foo");
    OE_TEST(!fs.open("foo", true));

    // open multiple files
    const auto h1 = fs.open("foo", false);
    OE_TEST(h1);
    const auto h2 = fs.open("foo", true);
    OE_TEST(h2);
    OE_TEST(h1 != h2);
    OE_TEST(fs.get_file(h1) == fs.get_file(h2));
    const auto h3 = fs.open("bar", false);
    OE_TEST(h3);
    OE_TEST(fs.get_file(h3) != fs.get_file(h1));
}

static void _test_memfs()
{
    for (int i = 0; i < 3; ++i) // ensure Memfs can be un-/reloaded
    {
        const Memfs memfs("myfs");
        OE_TEST(mount("/", "/mnt", "myfs", 0, nullptr) == 0);
        const auto path = "/mnt/file";

        const auto f1 = fopen(path, "w");
        OE_TEST(f1);
        const auto f2 = fopen(path, "r");
        OE_TEST(f2);
        OE_TEST(remove(path) == 0);
        OE_TEST(putc('A', f1) == 'A');
        OE_TEST(fclose(f1) == 0);
        OE_TEST(getc(f2) == 'A');
        OE_TEST(fclose(f2) == 0);
        OE_TEST(!fopen(path, "r"));

        OE_TEST(umount("/mnt") == 0);
    }
}

void test_ecall()
{
    _test_file();
    _test_filesystem();
    _test_memfs();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
