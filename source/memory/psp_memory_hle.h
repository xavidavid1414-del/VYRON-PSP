#ifndef PSP_MEMORY_HLE_H
#define PSP_MEMORY_HLE_H
#include <stdbool.h>
#include "psp_memory_blocks.h"
bool psp_memory_hle_register(PSP_MemoryBlocks *manager);
void psp_memory_hle_unregister(void);
#endif
