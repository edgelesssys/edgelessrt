#include <openenclave/bits/defs.h>
#include <stdint.h>

// these are set when the host loads and patches the enclave image
OE_EXPORT volatile uint64_t _payload_reloc_rva;
OE_EXPORT volatile uint64_t _payload_reloc_size;
