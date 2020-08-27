// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "file.h"

namespace ert::memfs
{
class Filesystem
{
  public:
    uintptr_t open(const std::string& pathname, bool must_exist);
    void close(uintptr_t handle);
    FilePtr get_file(uintptr_t handle);
    void unlink(const std::string& pathname);

  private:
    std::mutex mutex_;
    std::unordered_map<std::string, FilePtr> files_;
    std::unordered_map<uintptr_t, FilePtr> open_files_;
    uintptr_t handle_counter_ = 0;
};
} // namespace ert::memfs
