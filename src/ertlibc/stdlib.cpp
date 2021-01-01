// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <cstdlib>
#include <stdexcept>
#include "ertlibc_t.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

extern "C" char* secure_getenv(const char* name)
{
    return getenv(name);
}

void sc::exit_group(int status)
{
    if (ert_exit_ocall(status) != OE_OK)
        throw logic_error("exit_group");
}
