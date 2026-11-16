#ifndef PSP_INTERPRETER_H
#define PSP_INTERPRETER_H

#include <stdint.h>
#include <stdbool.h>

#include "psp_cpu.h"
#include "../memory/psp_memory.h"

bool psp_interpreter_step(
    PSP_CPU *cpu,
    PSP_Memory *memory
);

#endif
