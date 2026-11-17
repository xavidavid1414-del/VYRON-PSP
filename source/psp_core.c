#include "psp_core.h"
#include "cpu/psp_interpreter.h"

#include <string.h>

#define ELF_HEADER_SIZE 52
#define ELF_PROGRAM_HEADER_SIZE 32
#define ELF_PT_LOAD 1
#define ELF_MACHINE_MIPS 8
#define ELF_TYPE_EXEC 2
#define ELF_FLAG_EXECUTE 1

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] |
           ((uint16_t)p[1] << 8);
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static bool elf_memory_range(
    PSP_Memory *memory,
    uint32_t address,
    uint32_t size,
    uint32_t *offset
)
{
    if (memory == NULL || memory->ram == NULL ||
        offset == NULL || address < PSP_RAM_BASE)
        return false;

    uint32_t relative = address - PSP_RAM_BASE;

    if (relative > memory->size ||
        size > memory->size - relative)
        return false;

    *offset = relative;
    return true;
}

static bool load_elf_segments(
    PSP_Core *core,
    const uint8_t *elf,
    size_t elf_size
)
{
    if (core == NULL || elf == NULL ||
        elf_size < ELF_HEADER_SIZE)
        return false;

    if (!psp_loader_is_elf(elf, elf_size))
        return false;

    if (read_le16(elf + 16) != ELF_TYPE_EXEC ||
        read_le16(elf + 18) != ELF_MACHINE_MIPS ||
        read_le32(elf + 20) != 1)
        return false;

    uint32_t entry = read_le32(elf + 24);
    uint32_t phoff = read_le32(elf + 28);

    uint16_t phentsize = read_le16(elf + 42);
    uint16_t phnum = read_le16(elf + 44);

    if (phentsize < ELF_PROGRAM_HEADER_SIZE ||
        phnum == 0 || phoff > elf_size)
        return false;

    if ((size_t)phnum >
        (elf_size - phoff) / phentsize)
        return false;

    bool has_loadable_segment = false;
    bool entry_is_executable = false;

    /* Validate all loadable segments first. */
    for (uint16_t i = 0; i < phnum; i++)
    {
        const uint8_t *ph =
            elf + phoff + ((size_t)i * phentsize);

        if (read_le32(ph) != ELF_PT_LOAD)
            continue;

        uint32_t file_offset = read_le32(ph + 4);
        uint32_t virtual_address = read_le32(ph + 8);
        uint32_t file_size = read_le32(ph + 16);
        uint32_t memory_size = read_le32(ph + 20);
        uint32_t flags = read_le32(ph + 24);

        if (file_size > memory_size ||
            file_offset > elf_size ||
            file_size > elf_size - file_offset)
            return false;

        uint32_t memory_offset;

        if (!elf_memory_range(
                &core->memory,
                virtual_address,
                memory_size,
                &memory_offset))
            return false;

        (void)memory_offset;

        if (memory_size == 0)
            continue;

        has_loadable_segment = true;

        if ((flags & ELF_FLAG_EXECUTE) &&
            entry >= virtual_address &&
            (uint64_t)entry <
                (uint64_t)virtual_address + memory_size)
        {
            entry_is_executable = true;
        }
    }

    if (!has_loadable_segment || !entry_is_executable)
        return false;

    /* Clear RAM before loading the new executable. */
    memset(core->memory.ram, 0, core->memory.size);

    /* Copy segments and initialize their BSS areas. */
    for (uint16_t i = 0; i < phnum; i++)
    {
        const uint8_t *ph =
            elf + phoff + ((size_t)i * phentsize);

        if (read_le32(ph) != ELF_PT_LOAD)
            continue;

        uint32_t file_offset = read_le32(ph + 4);
        uint32_t virtual_address = read_le32(ph + 8);
        uint32_t file_size = read_le32(ph + 16);
        uint32_t memory_size = read_le32(ph + 20);

        if (memory_size == 0)
            continue;

        uint32_t memory_offset;

        if (!elf_memory_range(
                &core->memory,
                virtual_address,
                memory_size,
                &memory_offset))
            return false;

        if (file_size > 0)
        {
            memcpy(
                core->memory.ram + memory_offset,
                elf + file_offset,
                file_size
            );
        }

        if (memory_size > file_size)
        {
            memset(
                core->memory.ram + memory_offset + file_size,
                0,
                memory_size - file_size
            );
        }
    }

    core->cpu.pc = entry;
    core->cpu.halted = false;
    core->cpu.gpr[0] = 0;

    return true;
}

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

void psp_core_unload_executable(PSP_Core *core)
{
    if (core == NULL)
        return;

    psp_loader_unload(&core->image);

    core->image_loaded = false;
}

void psp_core_destroy(PSP_Core *core)
{
    if (core == NULL)
        return;

    psp_core_unload_executable(core);
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
        if (!psp_interpreter_step(
                &core->cpu,
                &core->memory))
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
        !core->iso_mounted || name == NULL ||
        entry == NULL)
        return false;

    return psp_iso_fs_find(
        &core->iso_fs,
        name,
        entry
    );
}

bool psp_core_load_executable(
    PSP_Core *core,
    const char *path
)
{
    if (core == NULL || !core->initialized ||
        path == NULL)
        return false;

    psp_core_unload_executable(core);

    if (!psp_loader_load(path, &core->image))
        return false;

    if (!core->image.executable_valid)
    {
        psp_core_unload_executable(core);
        return false;
    }

    const uint8_t *elf =
        core->image.data + core->image.executable_offset;

    if (!load_elf_segments(
            core,
            elf,
            core->image.executable_size))
    {
        psp_core_unload_executable(core);
        return false;
    }

    core->image_loaded = true;

    return true;
}
