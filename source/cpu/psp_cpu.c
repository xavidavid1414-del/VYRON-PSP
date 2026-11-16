#include "psp_cpu.h"
#include <string.h>

void psp_cpu_init(PSP_CPU *cpu)
{
    if (cpu == NULL)
        return;

    memset(cpu, 0, sizeof(PSP_CPU));

    cpu->pc = 0x00000000;
    cpu->halted = false;
}

void psp_cpu_reset(PSP_CPU *cpu)
{
    psp_cpu_init(cpu);
}
