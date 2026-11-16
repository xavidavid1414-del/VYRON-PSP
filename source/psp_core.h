#ifndef PSP_CORE_H
#define PSP_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "cpu/psp_cpu.h"
#include "memory/psp_memory.h"

typedef struct {
    PSP_CPU cpu;
    PSP_Memory memory;
    bool initialized;
} PSP_Core;

bool psp_core_init(PSP_Core *core);
void psp_core_reset(PSP_Core *core);
void psp_core_destroy(PSP_Core *core);
bool psp_core_run(PSP_Core *core, uint32_t cycles);

#endif
