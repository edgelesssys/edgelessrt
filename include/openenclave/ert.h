// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include "bits/module.h"

namespace ert
{
namespace memfs
{
class Filesystem;
}

/**
 * In-enclave-memory filesystem.
 *
 * Usage:
 *
 * const Memfs memfs("my_fs");
 * mount("/", "/my/mount/point", "my_fs", 0, nullptr);
 *
 * Folders are not supported for now. The app may open a file by passing a full
 * path and the fs behaves as if all folders in the path exist.
 */
class Memfs
{
  public:
    Memfs(const std::string& devname);
    ~Memfs();
    Memfs(const Memfs&) = delete;
    Memfs& operator=(const Memfs&) = delete;

  private:
    const std::unique_ptr<memfs::Filesystem> impl_;
    oe_customfs_t ops_;
    uint64_t devid_;

    static memfs::Filesystem& to_fs(void* context);
};
} // namespace ert
