#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <string>
#include "test_t.h"

using namespace std;

static char** _argv;

__attribute__((constructor)) static int test_global_ctor(
    int argc,
    char* argv[],
    char* envp[])
{
    OE_TEST(argc == 0);
    OE_TEST(envp == argv + 1);
    _argv = argv;
    return 0;
}

void test_ecall()
{
    OE_TEST(_argv);
    OE_TEST(ert_get_argc() == 0);
    OE_TEST(ert_get_argv() == _argv);
    OE_TEST(_argv[0] == NULL); // argv terminator
    OE_TEST(ert_get_envp() == _argv + 1);
    OE_TEST(_argv[1] == "OE_IS_ENCLAVE=1"s); // injected environment variable
    OE_TEST(_argv[2] == NULL);               // envp terminator
    OE_TEST(reinterpret_cast<long>(_argv[3]) == AT_PAGESZ);
    OE_TEST(reinterpret_cast<long>(_argv[4]) == OE_PAGE_SIZE);
    OE_TEST(_argv[5] == AT_NULL);
    OE_TEST(_argv[6] == NULL);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
