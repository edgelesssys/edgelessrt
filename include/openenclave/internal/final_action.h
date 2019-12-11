#pragma once

#include <utility>

namespace open_enclave
{
template <typename T>
class FinalAction final
{
  public:
    explicit FinalAction(T f) noexcept : f_(std::move(f)), invoke_(true)
    {
    }

    FinalAction(FinalAction&& other) noexcept
        : f_(std::move(other.f_)), invoke_(other.invoke_)
    {
        other.invoke_ = false;
    }

    FinalAction(const FinalAction&) = delete;
    FinalAction& operator=(const FinalAction&) = delete;

    ~FinalAction()
    {
        if (invoke_)
            f_();
    }

  private:
    T f_;
    bool invoke_;
};
} // namespace open_enclave
