// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <cstdio>
#include <cstdlib>

extern "C" size_t __fread_chk(
    void* buffer,
    size_t buffersize,
    size_t size,
    size_t count,
    FILE* stream)
{
    if (count * size > buffersize)
        abort();
    return fread(buffer, size, count, stream);
}

extern "C" int __isoc23_fscanf(FILE* stream, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    const int ret = vfscanf(stream, format, ap);
    va_end(ap);
    return ret;
}
