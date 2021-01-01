#pragma once

#include <pthread.h>

namespace ert
{
class Spinlock final
{
  public:
    Spinlock() noexcept
    {
        pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
    }

    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;

    ~Spinlock()
    {
        pthread_spin_destroy(&lock_);
    };

    void lock() noexcept
    {
        pthread_spin_lock(&lock_);
    }

    void unlock() noexcept
    {
        pthread_spin_unlock(&lock_);
    }

  private:
    pthread_spinlock_t lock_;
};
} // namespace ert
