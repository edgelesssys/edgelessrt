// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "filesystem.h"
#include <cassert>
#include <tuple>

using namespace std;
using namespace ert::memfs;

uintptr_t Filesystem::open(const std::string& pathname, bool must_exist)
{
    assert(!pathname.empty());

    bool created = false;
    decltype(files_)::const_iterator it;

    const lock_guard lock(mutex_);

    if (must_exist)
    {
        it = files_.find(pathname);
        if (it == files_.cend())
            return 0;
    }
    else
        tie(it, created) = files_.emplace(pathname, make_shared<File>());

    try
    {
        open_files_.emplace(handle_counter_ + 1, it->second);
    }
    catch (...)
    {
        if (created)
            files_.erase(pathname);
        throw;
    }

    return ++handle_counter_;
}

void Filesystem::close(uintptr_t handle)
{
    assert(handle);
    const lock_guard lock(mutex_);
    open_files_.erase(handle);
}

FilePtr Filesystem::get_file(uintptr_t handle)
{
    assert(handle);
    const lock_guard lock(mutex_);
    return open_files_.at(handle);
}

void Filesystem::unlink(const std::string& pathname)
{
    assert(!pathname.empty());
    const lock_guard lock(mutex_);
    files_.erase(pathname);
}
