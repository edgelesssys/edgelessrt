// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <stdio.h>

int main(int argc, char* argv[])
{
    puts("Hello world from the enclave!\nCommandline arguments:");

    for (int i = 0; i < argc; ++i)
        puts(argv[i]);

    return 0;
}
