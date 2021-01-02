#include <openenclave/internal/syscall/unistd.h>
#include <openenclave/internal/tests.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "../../ert/enclave/eventfd.h"
#include "test_t.h"

static void _test_eventfd()
{
    const int fd = eventfd(2, 0);
    OE_TEST(fd >= 0);

    OE_TEST(eventfd_write(fd, 3) == 0);

    oe_eventfd_t value = 0;
    OE_TEST(eventfd_read(fd, &value) == 0);
    OE_TEST(value == 5);

    // Test that read has reset the value.
    OE_TEST(eventfd_write(fd, 1) == 0);
    OE_TEST(eventfd_read(fd, &value) == 0);
    OE_TEST(value == 1);

    OE_TEST(close(fd) == 0);
}

static void _test_host_eventfd()
{
    const oe_host_fd_t fd = oe_host_eventfd(2, 0);
    OE_TEST(fd >= 0);

    OE_TEST(oe_host_eventfd_write(fd, 3) == 0);

    oe_eventfd_t value = 0;
    OE_TEST(oe_host_eventfd_read(fd, &value) == 0);
    OE_TEST(value == 5);

    // Test that read has reset the value.
    OE_TEST(oe_host_eventfd_write(fd, 1) == 0);
    OE_TEST(oe_host_eventfd_read(fd, &value) == 0);
    OE_TEST(value == 1);

    OE_TEST(oe_close_hostfd(fd) == 0);
}

void test_ecall()
{
    _test_eventfd();
    _test_host_eventfd();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
