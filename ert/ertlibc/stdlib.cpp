// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <cstdlib>

extern "C" char* secure_getenv(const char* name)
{
    return getenv(name);
}
