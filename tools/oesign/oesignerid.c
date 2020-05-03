// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <assert.h>
#include <openenclave/attestation/sgx/report.h>
#include <openenclave/bits/report.h>
#include <openenclave/internal/raise.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int load_pem_file(const char* path, void** data, size_t* size);

int oesignerid(const char* keyfile)
{
    assert(keyfile && *keyfile);

    char* pem = NULL;
    size_t pem_size = 0;
    if (load_pem_file(keyfile, (void**)&pem, &pem_size) != 0 || !pem ||
        !pem_size)
    {
        fputs("Failed to load file\n", stderr);
        return EXIT_FAILURE;
    }

    assert(pem[pem_size - 1] == '\0');

    int rc = EXIT_FAILURE;
    oe_result_t result = OE_FAILURE;

    uint8_t signer_id[OE_SIGNER_ID_SIZE];
    size_t signer_id_size = sizeof signer_id;
    OE_CHECK(oe_sgx_get_signer_id_from_public_key(
        pem, pem_size, signer_id, &signer_id_size));

    // print script friendly
    for (size_t i = 0; i < signer_id_size; ++i)
        printf("%02x", signer_id[i]);

    rc = EXIT_SUCCESS;

done:
    free(pem);
    return rc;
}
