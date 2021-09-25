// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <unistd.h>

namespace ert
{
//! file descriptor wrapper that closes on destroy to support RAII pattern
class FileDescriptor final
{
  public:
    FileDescriptor(int fd) noexcept : fd_(fd)
    {
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    ~FileDescriptor()
    {
        if (fd_ >= 0)
            close(fd_);
    }

    explicit operator bool() const noexcept
    {
        return fd_ >= 0;
    }

    operator int() const noexcept
    {
        return fd_;
    }

    int release() noexcept
    {
        const int result = fd_;
        fd_ = -1;
        return result;
    }

  private:
    int fd_;
};
} // namespace ert
