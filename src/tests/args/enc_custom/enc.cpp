#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <array>
#include <string>
#include "test_t.h"

using namespace std;

static char** _argv;

__attribute__((constructor)) static int test_global_ctor(
    int argc,
    char* argv[],
    char* envp[])
{
    OE_TEST(argc == 2);
    OE_TEST(envp == argv + 3);
    _argv = argv;
    return 0;
}

ert_args_t ert_get_args()
{
    // test that ocall works at this point of initialization
    string s(3, 0);
    get_str_ocall(s.data());
    OE_TEST(s == "foo");

    static array<const char*, 2> argv{"argv0", "argv1"};
    static array<const char*, 2> envp{"envp0", "envp1"};
    static array<long, 6> auxv{AT_EXECFD, 2, AT_PAGESZ, 3, AT_PHDR, 4};

    ert_args_t args{};
    args.argc = argv.size();
    args.argv = argv.data();
    args.envc = envp.size();
    args.envp = envp.data();
    args.auxc = auxv.size() / 2;
    args.auxv = auxv.data();
    return args;
}

void test_ecall()
{
    OE_TEST(_argv);
    OE_TEST(ert_get_argc() == 2);
    OE_TEST(ert_get_argv() == _argv);
    OE_TEST(_argv[0] == "argv0"s);
    OE_TEST(_argv[1] == "argv1"s);
    OE_TEST(_argv[2] == NULL); // argv terminator
    OE_TEST(ert_get_envp() == _argv + 3);
    OE_TEST(_argv[3] == "OE_IS_ENCLAVE=1"s); // injected environment variable
    OE_TEST(_argv[4] == "envp0"s);
    OE_TEST(_argv[5] == "envp1"s);
    OE_TEST(_argv[6] == NULL); // envp terminator
    const long* const auxv = reinterpret_cast<long*>(_argv + 7);
    OE_TEST(auxv[0] == AT_EXECFD);
    OE_TEST(auxv[1] == 2);
    OE_TEST(auxv[2] == AT_PAGESZ);
    OE_TEST(auxv[3] == 3);
    OE_TEST(auxv[4] == AT_PHDR);
    OE_TEST(auxv[5] == 4);
    OE_TEST(auxv[6] == AT_NULL);
    OE_TEST(auxv[7] == 0);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
