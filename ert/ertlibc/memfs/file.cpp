// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "file.h"
#include <cassert>
#include <cstring>
#include <stdexcept>

using namespace std;
using namespace ert::memfs;

static uint64_t _check_size(uint64_t count, uint64_t offset)
{
    const auto size = offset + count;
    if (size < offset)
        throw overflow_error("memfs: integer overflow");
    return size;
}

void File::read(void* buf, uint64_t count, uint64_t offset) const
{
    assert(buf || !count);
    const auto size = _check_size(count, offset);
    const lock_guard lock(mutex_);
    if (size > content_.size())
        throw out_of_range("memfs read: out of range");
    memcpy(buf, content_.data() + offset, count);
}

void File::write(const void* buf, uint64_t count, uint64_t offset)
{
    assert(buf || !count);
    const auto size = _check_size(count, offset);
    const lock_guard lock(mutex_);
    if (size > content_.size())
        content_.resize(size);
    memcpy(content_.data() + offset, buf, count);
}

uint64_t File::size() const noexcept
{
    const lock_guard lock(mutex_);
    return content_.size();
}
