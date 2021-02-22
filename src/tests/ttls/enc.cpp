#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <sys/socket.h>
#include "test_t.h"

void test_ecall()
{
    // TODO
    const char* const ttls_cfg = R"(
{
}
)";

    ert_init_ttls(ttls_cfg);

    // TODO
    // start test TLS server (ttls lib should provide a function for this)
    // test similar to Mbedtls.SendAndRecieve, but with POSIX connect() etc
    // calls
    OE_TEST(connect(0, 0, 0) == 2);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
