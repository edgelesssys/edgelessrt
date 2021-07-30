// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/bits/sgx/sgxtypes.h>
#include <openenclave/internal/crypto/sha.h>
#include <openenclave/internal/elf.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/types.h>
#include <iostream>
#include <string>
#include <string_view>
extern "C"
{
#include "../../../3rdparty/openenclave/openenclave/tools/oesign/oe_err.h"
#include "../../../3rdparty/openenclave/openenclave/tools/oesign/oeinfo.h"
}

using namespace std;

static string _get_unique_id(const sgx_sigstruct_t& sigstruct)
{
    const size_t chunk_size = OE_SHA256_SIZE;
    char buf[2 * chunk_size + 1];

    oe_hex_string(
        buf, sizeof(buf), sigstruct.enclavehash, sizeof(sigstruct.enclavehash));
    return buf;
}

static string _get_signer(const sgx_sigstruct_t& sigstruct)
{
    const size_t chunk_size = OE_SHA256_SIZE;
    char buf[2 * chunk_size + 1];
    OE_SHA256 mrsigner = {0};

    /* Check if modulus value is not set */
    size_t i = 0;
    while (i < sizeof(sigstruct.modulus) && sigstruct.modulus[i] == 0)
        i++;

    if (sizeof(sigstruct.modulus) > i)
        oe_sha256(sigstruct.modulus, sizeof(sigstruct.modulus), &mrsigner);

    oe_hex_string(buf, sizeof(buf), &mrsigner, sizeof(mrsigner.buf));
    return buf;
}

static void _extend_json(size_t indent, string& toExtend, string_view content)
{
    toExtend.append(indent, '\t');
    toExtend += content;
    toExtend += '\n';
}

static string _build_json(const oe_sgx_enclave_properties_t& props)
{
    string json;
    const auto sigstruct =
        reinterpret_cast<const sgx_sigstruct_t&>(props.sigstruct);

    _extend_json(0, json, "{");
    _extend_json(
        1,
        json,
        "\"SecurityVersion\": " + to_string(props.config.security_version) +
            ",");
    _extend_json(
        1, json, "\"ProductID\": " + to_string(props.config.product_id) + ",");
    _extend_json(
        1, json, "\"UniqueID\": \"" + _get_unique_id(sigstruct) + "\",");
    _extend_json(1, json, "\"SignerID\": \"" + _get_signer(sigstruct) + "\",");
    _extend_json(
        1,
        json,
        "\"Debug\": "s +
            (props.config.attributes & OE_SGX_FLAGS_DEBUG ? "true" : "false"));
    _extend_json(0, json, "}");
    return json;
}

extern "C" int eradump(const char* enc_bin)
{
    int ret = 1;
    elf64_t elf;
    oe_sgx_enclave_properties_t props;

    /* Load the ELF-64 object */
    if (elf64_load(enc_bin, &elf) != 0)
    {
        oe_err("Failed to load %s as ELF64", enc_bin);
        goto done;
    }

    /* Load the SGX enclave properties */
    if (oe_read_oeinfo_sgx(enc_bin, &props) != OE_OK)
    {
        oe_err(
            "Failed to load SGX enclave properties from %s section",
            OE_INFO_SECTION_NAME);
        goto done;
    }
    cout << _build_json(props);

    ret = 0;

done:
    elf64_unload(&elf);
    return ret;
}
