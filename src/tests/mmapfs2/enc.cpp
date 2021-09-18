#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "test_t.h"

/* This test is mostly complementary to tests/syscall/fs/enc/enc.cpp to test
 * file region extension */

using namespace std;

static const char* fname = nullptr;
static const string buf = "ABCD";

static void _test_open_file()
{
    FILE* f = fopen(fname, "r");
    OE_TEST(!f);

    f = fopen(fname, "r+");
    OE_TEST(!f);

    f = fopen(fname, "w");
    OE_TEST(f);
    OE_TEST(fclose(f) == 0);

    f = fopen(fname, "w+");
    OE_TEST(f);
    OE_TEST(fclose(f) == 0);

    f = fopen(fname, "a");
    OE_TEST(f);
    OE_TEST(fclose(f) == 0);

    f = fopen(fname, "a+");
    OE_TEST(f);
    OE_TEST(fclose(f) == 0);

    OE_TEST(remove(fname) == 0);
}

static void _test_read_file()
{
    FILE* f = fopen(fname, "r");
    OE_TEST(f);
    for (size_t i = 0; i < OE_PAGE_SIZE; i += buf.size())
    {
        string tmp(buf.size(), '\0');
        OE_TEST(fread(tmp.data(), tmp.size(), 1, f) == 1);
        OE_TEST(tmp == buf);
    }

    char c;
    OE_TEST(fread(&c, 1, 1, f) == 0);
    OE_TEST(fclose(f) == 0);
    OE_TEST(remove(fname) == 0);
}

static void _test_write_file()
{
    FILE* f = fopen(fname, "w");
    OE_TEST(f);
    array<uint8_t, OE_PAGE_SIZE> content;
    content.fill(0x41);
    OE_TEST(fwrite(content.data(), 1, content.size(), f) == content.size());
    OE_TEST(fclose(f) == 0);
    OE_TEST(remove(fname) == 0);

    f = fopen(fname, "w+");
    OE_TEST(f);
    for (size_t i = 0; i < OE_PAGE_SIZE; i += 4)
    {
        OE_TEST(fwrite(buf.c_str(), buf.size(), 1, f) == 1);
    }
    OE_TEST(fclose(f) == 0);
}

static void _test_extend_file()
{
    const size_t file_pages = 0x100;
    const string header = "ABCDE";

    FILE* f = fopen(fname, "w");
    OE_TEST(f);
    OE_TEST(fwrite(header.data(), 1, header.size(), f) == header.size());
    OE_TEST(fclose(f) == 0);

    f = fopen(fname, "a");
    OE_TEST(f);
    for (size_t i = 0; i < file_pages; i++)
    {
        array<uint8_t, OE_PAGE_SIZE> content;
        content.fill(static_cast<uint8_t>(i));
        OE_TEST(fwrite(content.data(), 1, content.size(), f) == content.size());
    }

    OE_TEST(fseek(f, 0, SEEK_END) == 0);
    OE_TEST(
        ftell(f) ==
        static_cast<long>(file_pages * OE_PAGE_SIZE + header.size()));
    OE_TEST(fclose(f) == 0);

    f = fopen(fname, "r");
    string tmp(header.size(), '\0');
    OE_TEST(f);
    OE_TEST(fread(tmp.data(), 1, tmp.size(), f) == tmp.size());
    OE_TEST(tmp == header);
    for (size_t i = 0; i < file_pages; i++)
    {
        array<uint8_t, OE_PAGE_SIZE> content;
        OE_TEST(fread(content.data(), 1, content.size(), f) == content.size());
        OE_TEST(
            memcmp(
                content.data(),
                string(content.size(), static_cast<char>(i)).data(),
                content.size()) == 0);
    }

    OE_TEST(fclose(f) == 0);
    OE_TEST(remove(fname) == 0);
}

void _cleanup(string_view dir)
{
    const int res = remove(fname);
    OE_TEST(res == 0 || (res == -1 && errno == ENOENT));
    OE_TEST(remove(dir.data()) == 0);
}

void test_ecall()
{
    OE_TEST(oe_load_module_host_file_system() == OE_OK);
    OE_TEST(mount("/tmp", "/tmp", OE_HOST_FILE_SYSTEM_MMAP, 0, nullptr) == 0);

    string dir = "/tmp/tmp.XXXXXX";
    OE_TEST(mkdtemp(dir.data()) == dir.data());
    const string filename = dir + "/new_file";
    fname = filename.c_str();
    _test_open_file();
    _test_write_file();
    _test_read_file();
    _test_extend_file();
    _cleanup(dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
