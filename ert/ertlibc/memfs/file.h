// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace ert::memfs
{
class File
{
  public:
    void read(void* buf, uint64_t count, uint64_t offset) const;
    void write(const void* buf, uint64_t count, uint64_t offset);
    uint64_t size() const noexcept;

  private:
    mutable std::mutex mutex_;
    std::vector<uint8_t> content_;
};

typedef std::shared_ptr<File> FilePtr;
} // namespace ert::memfs
