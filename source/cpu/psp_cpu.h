#ifndef PSP_CPU_H
#define PSP_CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t gpr[32];
    uint32_t pc;
    uint32_t hi;
    uint32_t lo;

    uint32_t cop0[32];

    uint32_t branch_target;
    bool branch_pending;
    bool halted;
} PSP_CPU;

void psp_cpu_init(PSP_CPU *cpu);
void psp_cpu_reset(PSP_CPU *cpu);

#endif
