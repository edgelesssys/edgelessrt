#include <openenclave/internal/tests.h>
#include <cerrno>
#include <csignal>
#include "test_t.h"

extern "C" const bool ert_enable_signals = true;
extern "C" const bool ert_enable_sigsegv = true;

static int f()
{
    return 2;
}

static void handler(int sig, siginfo_t* info, void* ucontext)
{
    OE_TEST(sig == SIGSEGV);
    OE_TEST(info);

    // test_ecall called nullptr. Set rip to a function so that thread execution
    // can continue.
    static_cast<ucontext_t*>(ucontext)->uc_mcontext.gregs[REG_RIP] =
        reinterpret_cast<greg_t>(f);
}

void test_ecall()
{
    // Register signal handler.
    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    OE_TEST(sigaction(SIGSEGV, &sa, nullptr) == 0);

    // Call nullptr. The signal handler will fix this.
    int (*const volatile p)() = nullptr;
    OE_TEST(p() == 2);

    // Test invalid arguments.
    errno = 0;
    OE_TEST(sigaction(0, &sa, nullptr) == -1 && errno == EINVAL);
    errno = 0;
    OE_TEST(sigaction(-1, &sa, nullptr) == -1 && errno == EINVAL);
    errno = 0;
    OE_TEST(sigaction(-2, &sa, nullptr) == -1 && errno == EINVAL);

    // Get old sigaction.
    struct sigaction oldsa = {};
    OE_TEST(sigaction(SIGSEGV, nullptr, &oldsa) == 0);
    OE_TEST(oldsa.sa_flags & SA_SIGINFO);
    OE_TEST(oldsa.sa_sigaction == handler);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
