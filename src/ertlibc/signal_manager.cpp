#include "signal_manager.h"
#include <openenclave/internal/sgx/td.h>
#include <mutex>
#include "signal.h"

using namespace std;
using namespace ert;

// currently only SIGSEGV is implemented
static constexpr array _code_to_sig{
    0,      // OE_EXCEPTION_DIVIDE_BY_ZERO
    0,      // OE_EXCEPTION_BREAKPOINT
    0,      // OE_EXCEPTION_BOUND_OUT_OF_RANGE
    0,      // OE_EXCEPTION_ILLEGAL_INSTRUCTION
    SIGSEGV // OE_EXCEPTION_ACCESS_VIOLATION
};

static_assert(_code_to_sig[OE_EXCEPTION_ACCESS_VIOLATION] == SIGSEGV);

static constexpr array _ureg_to_ereg{
    &oe_context_t::r8,
    &oe_context_t::r9,
    &oe_context_t::r10,
    &oe_context_t::r11,
    &oe_context_t::r12,
    &oe_context_t::r13,
    &oe_context_t::r14,
    &oe_context_t::r15,
    &oe_context_t::rdi,
    &oe_context_t::rsi,
    &oe_context_t::rbp,
    &oe_context_t::rbx,
    &oe_context_t::rdx,
    &oe_context_t::rax,
    &oe_context_t::rcx,
    &oe_context_t::rsp,
    &oe_context_t::rip,
};

static_assert(_ureg_to_ereg[REG_R8] == &oe_context_t::r8);
static_assert(_ureg_to_ereg[REG_R15] == &oe_context_t::r15);
static_assert(_ureg_to_ereg[REG_RDI] == &oe_context_t::rdi);
static_assert(_ureg_to_ereg[REG_RSI] == &oe_context_t::rsi);
static_assert(_ureg_to_ereg[REG_RBP] == &oe_context_t::rbp);
static_assert(_ureg_to_ereg[REG_RBX] == &oe_context_t::rbx);
static_assert(_ureg_to_ereg[REG_RDX] == &oe_context_t::rdx);
static_assert(_ureg_to_ereg[REG_RAX] == &oe_context_t::rax);
static_assert(_ureg_to_ereg[REG_RCX] == &oe_context_t::rcx);
static_assert(_ureg_to_ereg[REG_RSP] == &oe_context_t::rsp);
static_assert(_ureg_to_ereg[REG_RIP] == &oe_context_t::rip);

SignalManager& SignalManager::get_instance() noexcept
{
    static SignalManager instance;
    return instance;
}

void SignalManager::sigaction(
    int signum,
    const k_sigaction* act,
    k_sigaction* oldact)
{
    auto& action = actions_.at(static_cast<size_t>(signum));
    const lock_guard lock(spinlock_);
    if (oldact)
        *oldact = action;
    if (act)
        action = *act;
}

bool SignalManager::vectored_exception_handler(
    const oe_exception_record_t& exception_context) noexcept
{
    if (exception_context.code >= _code_to_sig.size())
        return false;
    const int sig = _code_to_sig[exception_context.code];

    // Get handler for this signal.
    void (*handler)(int, siginfo_t*, void*);
    {
        const lock_guard lock(spinlock_);
        handler = reinterpret_cast<decltype(handler)>(
            actions_[static_cast<size_t>(sig)].handler);
    }
    if (!handler)
        return false;

    // Populate siginfo and ucontext. Currently only what is needed by Go.

    siginfo_t info{};
    info.si_code = SEGV_MAPERR;

    ucontext_t uctx{};
    for (size_t i = 0; i < _ureg_to_ereg.size(); ++i)
        uctx.uc_mcontext.gregs[i] =
            static_cast<greg_t>(exception_context.context->*_ureg_to_ereg[i]);

    // Call the handler.
    handler(sig, &info, &uctx);

    // Copy the (possibly modified) context back.
    for (size_t i = 0; i < _ureg_to_ereg.size(); ++i)
        exception_context.context->*_ureg_to_ereg[i] =
            static_cast<uint64_t>(uctx.uc_mcontext.gregs[i]);

    return true;
}

SignalManager::StackBuffer SignalManager::get_stack() const noexcept
{
    const auto* const td = oe_sgx_get_td();
    return {
        reinterpret_cast<uint8_t*>(td->exception_handler_stack),
        td->exception_handler_stack_size};
}

void SignalManager::set_stack(StackBuffer buffer)
{
    const auto td = oe_sgx_get_td();

    if (buffer.empty())
    {
        if (!oe_sgx_td_set_exception_handler_stack(td, nullptr, 0))
            throw runtime_error("oe_sgx_td_set_exception_handler_stack failed");
        if (!oe_sgx_td_unregister_exception_handler_stack(
                td, OE_EXCEPTION_ACCESS_VIOLATION))
            throw runtime_error(
                "oe_sgx_td_unregister_exception_handler_stack failed");
        return;
    }

    size_t size = buffer.size();

    // This should actually be MINSIGSTKSZ, but it's too small for the enclave
    // exception handling.
    if (size < ERT_SIGSTKSZ)
        throw system_error(
            ENOMEM, system_category(), "sigaltstack: buffer too small");

    // ensure stack + size is 16-byte aligned
    size -= reinterpret_cast<uintptr_t>(buffer.data()) % 16;

    if (!oe_sgx_td_set_exception_handler_stack(td, buffer.data(), size))
        throw runtime_error("oe_sgx_td_set_exception_handler_stack failed");
    if (!oe_sgx_td_register_exception_handler_stack(
            td, OE_EXCEPTION_ACCESS_VIOLATION))
        throw runtime_error(
            "oe_sgx_td_register_exception_handler_stack failed");
}

SignalManager::SignalManager() noexcept : actions_(), spinlock_()
{
}
