#include <openenclave/enclave.h>
#include <cassert>
#include <mutex>
#include "signal_manager.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

// enclave dev can enable signals by overriding this symbol
extern "C" bool ert_enable_signals;
OE_WEAK bool ert_enable_signals;

static once_flag _vectored_exception_handler_once;

static uint64_t _vectored_exception_handler(
    oe_exception_record_t* exception_context)
{
    assert(exception_context);
    if (SignalManager::get_instance().vectored_exception_handler(
            *exception_context))
        return OE_EXCEPTION_CONTINUE_EXECUTION;
    else
        return OE_EXCEPTION_CONTINUE_SEARCH;
}

void sc::rt_sigaction(int signum, const k_sigaction* act, k_sigaction* oldact)
{
    SignalManager::get_instance().sigaction(signum, act, oldact);

    // register the exception handler if a sigaction has been set
    if (ert_enable_signals && act)
        call_once(
            _vectored_exception_handler_once,
            oe_add_vectored_exception_handler,
            false,
            _vectored_exception_handler);
}

void sc::sigaltstack(const stack_t* ss, stack_t* oss)
{
    auto& manager = SignalManager::get_instance();

    if (oss)
    {
        const auto stack = manager.get_stack();
        *oss = {};
        if (stack.empty())
            oss->ss_flags = SS_DISABLE;
        else
        {
            oss->ss_sp = stack.data();
            oss->ss_size = stack.size();
        }
    }

    if (!ss)
        return;

    if (ss->ss_flags == SS_DISABLE)
    {
        manager.set_stack({});
        return;
    }

    if (ss->ss_flags)
        throw system_error(EINVAL, system_category(), "sigaltstack: ss_flags");

    manager.set_stack({static_cast<uint8_t*>(ss->ss_sp), ss->ss_size});
}
