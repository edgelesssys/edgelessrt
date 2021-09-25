#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <cstdio>
#include "test_t.h"

void test_ecall()
{
    OE_TEST(oe_load_module_host_file_system() == OE_OK);
    OE_TEST(mount("/tmp", "/", OE_HOST_FILE_SYSTEM_MMAP, 0, nullptr) == 0);

    const auto name_0 = "erttest_mmapfs_0";
    const auto name_1 = "erttest_mmapfs_1";
    const auto name_restart = "erttest_mmapfs_restart";

    // The test verifies that files have the correct size even if they are not
    // closed properly.

    if (remove(name_restart) != 0)
    {
        // Create two files. Don't close them.
        auto f = fopen(name_1, "w");
        OE_TEST(f);
        OE_TEST(fputc(2, f) == 2);
        OE_TEST(fflush(f) == 0);
        f = fopen(name_0, "w");
        OE_TEST(f);

        // Create a file that indicates the restart.
        f = fopen(name_restart, "w");
        OE_TEST(f);
        OE_TEST(fclose(f) == 0);

        ert_restart_host_process();
        OE_TEST(false);
    }

    // This is reached after restart. Check that files are correct.

    auto f = fopen(name_1, "r");
    OE_TEST(fgetc(f) == 2);
    OE_TEST(fgetc(f) == EOF);
    OE_TEST(fclose(f) == 0);

    f = fopen(name_1, "r+");
    OE_TEST(fgetc(f) == 2);
    OE_TEST(fgetc(f) == EOF);
    OE_TEST(fclose(f) == 0);

    OE_TEST(remove(name_1) == 0);

    f = fopen(name_0, "r");
    OE_TEST(f);
    OE_TEST(fgetc(f) == EOF);
    OE_TEST(fclose(f) == 0);

    f = fopen(name_0, "r+");
    OE_TEST(f);
    OE_TEST(fgetc(f) == EOF);
    OE_TEST(fclose(f) == 0);

    OE_TEST(remove(name_0) == 0);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
