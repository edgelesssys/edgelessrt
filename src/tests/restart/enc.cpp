#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <cstdio>
#include "test_t.h"

void test_ecall()
{
    OE_TEST(oe_load_module_host_file_system() == OE_OK);
    OE_TEST(mount("/tmp", "/", OE_HOST_FILE_SYSTEM, 0, nullptr) == 0);

    // Restart two times. Use a file to count.

    int count = 0;
    const char* const filename = "erttest_restart";
    FILE* f = fopen(filename, "r+");

    if (f)
    {
        count = getc(f);
        OE_TEST(count > 0);
        rewind(f);

        if (count > 2)
        {
            // Unlikely to get here. File might not have been deleted in a prior
            // test run.
            count = 0;
        }
    }
    else
    {
        f = fopen(filename, "w");
        OE_TEST(f);
    }

    ++count;
    printf("%d\n", count);
    OE_TEST(putc(count, f) == count);
    OE_TEST(fclose(f) == 0);

    if (count < 3)
    {
        ert_restart_host_process();
        OE_TEST(false);
    }

    OE_TEST(remove(filename) == 0);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
