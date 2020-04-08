#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/properties.h>
#include <openenclave/internal/tests.h>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    OE_TEST(argc == 3);
    OE_TEST(argv);
    OE_TEST(argv[0] == "./devhost_enc"s);
    OE_TEST(argv[1] == "arg1"s);
    OE_TEST(argv[2] == "arg2"s);
    OE_TEST(getenv("OE_DEVHOST_TEST") == "test"s);
    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
