#include <openenclave/enclave.h>
#include <openenclave/enclave_args.h>
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
    OE_TEST(oe_get_argc() == 0);
    OE_TEST(oe_get_argv() == _argv);
    OE_TEST(_argv[0] == NULL); // argv terminator
    OE_TEST(oe_get_envp() == _argv + 1);
    OE_TEST(_argv[1] == "OE_IS_ENCLAVE=1"s); // injected environment variable
    OE_TEST(_argv[2] == NULL);               // envp terminator
    OE_TEST(_argv[3] == NULL);               // auxv key AT_NULL
    OE_TEST(_argv[4] == NULL);               // auxv value
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
