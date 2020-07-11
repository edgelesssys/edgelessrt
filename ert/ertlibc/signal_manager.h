#pragma once

#include <openenclave/enclave.h>
#include <array>
#include <csignal>
#include "spinlock.h"

#define hidden
#include "../3rdparty/musl/musl/src/internal/ksigaction.h"
#undef hidden

namespace ert
{
class SignalManager final
{
  public:
    SignalManager(const SignalManager&) = delete;
    SignalManager& operator=(const SignalManager&) = delete;

    static SignalManager& get_instance() noexcept;

    void sigaction(int signum, const k_sigaction* act, k_sigaction* oldact);

    // Should be invoked by an OE exception handler. Returns true if execution
    // should be continued.
    bool vectored_exception_handler(
        const oe_exception_record_t& exception_context) noexcept;

  private:
    std::array<k_sigaction, NSIG> actions_;
    Spinlock spinlock_;

    SignalManager() noexcept;
};
} // namespace ert
