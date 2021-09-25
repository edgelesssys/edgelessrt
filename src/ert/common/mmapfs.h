// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <stddef.h>
#include <stdint.h>

/* size step to increase memory regions of mmap'ed files */
#define OE_MMAP_FILE_CHUNK_SIZE (1 << 12)

// This is appended to an opened, writable file to preserve the actual file size
// in case the file is not properly closed.
typedef struct
{
    char tag[sizeof(uint64_t)];
    size_t size;
} ert_mmapfs_file_size_t;
