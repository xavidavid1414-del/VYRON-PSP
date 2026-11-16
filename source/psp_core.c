#include "psp_core.h"
#include "cpu/psp_interpreter.h"

#include <string.h>

bool psp_core_init(PSP_Core *core)
{
    if (core == NULL)
        return false;

    memset(core, 0, sizeof(*core));

    if (!psp_memory_init(&core->memory))
        return false;

    psp_cpu_init(&core->cpu);
    core->initialized = true;

    return true;
}

void psp_core_reset(PSP_Core *core)
{
    if (core == NULL || !core->initialized)
        return;

    psp_cpu_reset(&core->cpu);
}

void psp_core_destroy(PSP_Core *core)
{
    if (core == NULL)
        return;

    psp_memory_destroy(&core->memory);
    memset(core, 0, sizeof(*core));
}

bool psp_core_run(PSP_Core *core, uint32_t cycles)
{
    if (core == NULL || !core->initialized)
        return false;

    for (uint32_t i = 0; i < cycles; i++)
    {
        if (!psp_interpreter_step(&core->cpu, &core->memory))
            return false;
    }

    return true;
}
