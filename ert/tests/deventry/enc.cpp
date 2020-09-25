#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/properties.h>
#include <openenclave/internal/tests.h>
#include <unistd.h>
#include <array>
#include <climits>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    OE_TEST(argc == 3);
    OE_TEST(argv);
    OE_TEST(argv[0] == "./erttest_deventry"s);
    OE_TEST(argv[1] == "arg1"s);
    OE_TEST(argv[2] == "arg2"s);
    OE_TEST(getenv("ERT_DEVENTRY_TEST") == "test"s);

    array<char, PATH_MAX> cwd{};
    OE_TEST(getcwd(cwd.data(), cwd.size()) == cwd.data());
    OE_TEST(cwd[0]);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
