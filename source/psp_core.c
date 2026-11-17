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

void psp_core_unmount_iso(PSP_Core *core)
{
    if (core == NULL)
        return;

    memset(&core->iso_info, 0, sizeof(core->iso_info));
    memset(&core->iso_fs, 0, sizeof(core->iso_fs));
    core->iso_data = NULL;
    core->iso_size = 0;
    core->iso_mounted = false;
}

void psp_core_destroy(PSP_Core *core)
{
    if (core == NULL)
        return;

    psp_core_unmount_iso(core);
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

bool psp_core_mount_iso(
    PSP_Core *core,
    const uint8_t *data,
    size_t size
)
{
    PSP_ISO_Info info;
    PSP_ISO_FS fs;

    if (core == NULL || !core->initialized ||
        data == NULL || size == 0)
        return false;

    memset(&info, 0, sizeof(info));
    memset(&fs, 0, sizeof(fs));

    if (!psp_iso_identify(data, size, &info))
        return false;

    if (!psp_iso_fs_init(&fs, data, size))
        return false;

    core->iso_info = info;
    core->iso_fs = fs;
    core->iso_data = data;
    core->iso_size = size;
    core->iso_mounted = true;

    return true;
}

bool psp_core_find_iso_entry(
    PSP_Core *core,
    const char *name,
    PSP_ISO_Entry *entry
)
{
    if (core == NULL || !core->initialized ||
        !core->iso_mounted || name == NULL || entry == NULL)
        return false;

    return psp_iso_fs_find(&core->iso_fs, name, entry);
}
