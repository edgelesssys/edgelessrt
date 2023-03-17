#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/properties.h>
#include <openenclave/internal/tests.h>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    OE_TEST(argc == 2);
    OE_TEST(argv);
    OE_TEST(argv[0] == "arg0"s);
    OE_TEST(argv[1] == "arg1"s);
    OE_TEST(!argv[2]);
    OE_TEST(getenv("key") == "value"s);
    OE_TEST(getenv("EDG_FOO") == "bar"s);
    OE_TEST(!getenv("ABC_FOO"));

    string s;
    ifstream("/folder1/folder2/file") >> s;
    OE_TEST(s == "test");

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    4096, /* NumHeapPages */
    64,   /* NumStackPages */
    16);  /* NumTCS */
