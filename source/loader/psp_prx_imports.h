#ifndef PSP_PRX_IMPORTS_H
#define PSP_PRX_IMPORTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Resolve PSP .lib.stub imports and patch each function stub with a MIPS
 * return/syscall pair. Unknown NIDs or malformed tables fail atomically. */
bool psp_prx_resolve_imports(
    const uint8_t *elf,
    size_t elf_size,
    uint8_t *ram,
    uint32_t ram_size
);

#endif
