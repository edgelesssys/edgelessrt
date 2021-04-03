// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpuid.h"

#define EXTENDED_FEATURE_FLAGS_FUNCTION 0x7
#define SGX_CAPABILITY_ENUMERATION 0x12

#define HAVE_SGX(regs) (((regs.ebx) >> 2) & 1)
#define HAVE_FLC(regs) (((regs.ecx) >> 30) & 1)
#define HAVE_SGX1(regs) (((regs.eax) & 1))
#define HAVE_SGX2(regs) (((regs.eax) >> 1) & 1)
#define HAVE_KSS(regs) (((regs.eax) >> 7) & 1)
#define HAVE_EPC_SUBLEAF(regs) (((regs.eax) & 0x0f) == 0x01)

typedef struct _regs
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
} Regs;

void dump_regs(Regs* regs)
{
    printf("eax = 0x%x\n", regs->eax);
    printf("ebx = 0x%x\n", regs->ebx);
    printf("ecx = 0x%x\n", regs->ecx);
    printf("edx = 0x%x\n", regs->edx);
}

static unsigned int get_max_leaf()
{
    Regs regs = {0, 0, 0, 0};
    oe_get_cpuid(0, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
    return regs.eax;
}

static int _CPUID(Regs* regs)
{
    unsigned int leaf_requested = regs->eax;
    unsigned int max_leaf = get_max_leaf();
    int result = 0;

    if (leaf_requested > max_leaf)
    {
        printf(
            "Error: requested leaf %d  (> max_leaf(%d)) is not supported.\n",
            leaf_requested,
            max_leaf);
        return 1;
    }

    oe_get_cpuid(
        leaf_requested,
        regs->ecx,
        &regs->eax,
        &regs->ebx,
        &regs->ecx,
        &regs->edx);

    // Check if no sub-leaves are supported
    if (regs->eax == 0 && regs->ebx == 0 && regs->ecx == 0 && regs->edx == 0)
    {
        result = 1;
    }
    return result;
}

int main(int argc, const char* argv[])
{
    int result = 0;

    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(1);
    }

    /* Figure out whether CPU supports SGX */
    {
        Regs regs = {EXTENDED_FEATURE_FLAGS_FUNCTION, 0, 0x0, 0};

        result = _CPUID(&regs);
        if (result)
        {
            dump_regs(&regs);
            return result;
        }

        if (!HAVE_SGX(regs))
        {
            printf("CPU does not support SGX\n");
            return 0;
        }

        // Flexible Launch Control is available (bit 30 of the ECX return value
        // on leaf 0x7, subleaf 0x0)
        if (HAVE_FLC(regs))
        {
            printf("CPU supports SGX_FLC:Flexible Launch Control\n");
        }
    }

    /* Enumeration of Intel SGX Capabilities: figure out whether CPU
       supports SGX-1 or SGX-2 */
    {
        Regs regs = {SGX_CAPABILITY_ENUMERATION, 0, 0x0, 0};

        result = _CPUID(&regs);
        if (result)
        {
            printf("Read SGX_CAPABILITY_ENUMERATION failed:\n");
            dump_regs(&regs);
            return result;
        }

        printf("CPU supports Software Guard Extensions:");

        if (HAVE_SGX2(regs))
        {
            printf("SGX2\n");
        }
        if (HAVE_SGX1(regs))
        {
            printf("SGX1\n");
        }
        printf("MaxEnclaveSize_64: 2^(%d)\n", (regs.edx >> 8) & 0xFF);
    }

    /* Enumeration of Intel SGX Capabilities: figure out whether CPU
       supports KSS */
    {
        Regs regs = {SGX_CAPABILITY_ENUMERATION, 0, 0x1, 0};

        result = _CPUID(&regs);
        if (result)
        {
            printf("Read SGX_ATTRIBUTE_ENUMERATION failed:\n");
            dump_regs(&regs);
            return result;
        }

        printf(
            "CPU supports Key Sharing & Separation (KSS): %s\n",
            HAVE_KSS(regs) ? "true" : "false");
    }

    /* Enumeration of Intel SGX Capabilities: figure out EPC size */
    {
        unsigned int ecx = 0x2;
        Regs regs = {SGX_CAPABILITY_ENUMERATION, 0, ecx, 0};
        uint64_t epc_size = 0;
        result = _CPUID(&regs);
        if (result)
        {
            printf("Read SGX_CAPABILITY_ENUMERATION failed:\n");
            dump_regs(&regs);
            return result;
        }

        if (!HAVE_EPC_SUBLEAF(regs))
        {
            printf("No EPC section\n");
            dump_regs(&regs);
            return 0;
        }

        /*
           Find out if SUBLEAF exists by iterating over ecx incremented by 1.
           If SUBLEAF exists and _CPUID returns a valid response, then the EPC
           memory of the subleaf is add to the total EPC memory used.
           When _CPUID call fails for a ecx value, the loop is terminated.
        */
        while (!result && HAVE_EPC_SUBLEAF(regs))
        {
            epc_size +=
                ((regs.ecx & 0x0fffff000) |
                 ((uint64_t)(regs.edx & 0x0fffff) << 32));

            ecx++;
            /* re-populate regs to check if _CPUID is valid
               for current ecx */
            regs.eax = SGX_CAPABILITY_ENUMERATION;
            regs.ebx = 0;
            regs.ecx = ecx;
            regs.edx = 0;
            result = _CPUID(&regs);
        }
        printf(
            "EPC size on the platform: %llu\n", (unsigned long long)epc_size);
    }
    return 0;
}
