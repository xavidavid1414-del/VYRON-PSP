#include <stdlib.h>
#include "psp_core.h"
#include "cpu/psp_interpreter.h"
#include "cpu/psp_syscall.h"
#include "input/psp_controller_hle.h"
#include "loader/psp_nid.h"
#include "loader/psp_prx_imports.h"
#include "memory/psp_memory_hle.h"
#include "loader/psp_io_hle.h"

#include <string.h>

#define ELF_HEADER_SIZE 52
#define ELF_PROGRAM_HEADER_SIZE 32
#define ELF_PT_LOAD 1
#define ELF_MACHINE_MIPS 8
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_PRX 0xFFA0
#define ELF_PT_PSPREL1 0x700000A0
#define ELF_PT_PSPREL2 0x700000A1

#define ELF_MAX_PROGRAM_HEADERS 32

#define MIPS_R_NONE 0
#define MIPS_R_16 1
#define MIPS_R_32 2
#define MIPS_R_26 4
#define MIPS_R_HI16 5
#define MIPS_R_LO16 6
#define MIPS_R_GPREL16 7
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
static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
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

static bool apply_psprel2(
    PSP_Core *core, const uint8_t *elf, size_t elf_size,
    uint16_t phnum, uint16_t phentsize, uint16_t rel_segment,
    const uint32_t *segment_base, const uint32_t *segment_size,
    const bool *segment_loaded)
{
    const uint8_t *ph = elf + read_le32(elf + 28) +
        (size_t)rel_segment * phentsize;
    uint32_t file_offset = read_le32(ph + 4);
    uint32_t file_size = read_le32(ph + 16);
    if (file_offset > elf_size || file_size > elf_size - file_offset ||
        file_size < 4)
        return false;

    const uint8_t *cursor = elf + file_offset;
    const uint8_t *end = cursor + file_size;
    uint32_t flag_bits = cursor[2], type_bits = cursor[3], seg_bits = 1;
    while ((1u << seg_bits) < rel_segment)
        seg_bits++;
    if (!flag_bits || !type_bits ||
        flag_bits + seg_bits + type_bits > 16u)
        return false;

    uint32_t total_bits = flag_bits + seg_bits + type_bits;
    uint32_t flag_mask = (1u << flag_bits) - 1u;
    uint32_t seg_mask = (1u << seg_bits) - 1u;
    uint32_t type_mask = (1u << type_bits) - 1u;
    cursor += 4;
    if (cursor >= end) return false;
    uint32_t flag_count = *cursor++;
    if ((size_t)(end - cursor) < flag_count) return false;
    const uint8_t *flag_table = cursor;
    cursor += flag_count;
    if (cursor >= end) return false;
    uint32_t type_count = *cursor++;
    if ((size_t)(end - cursor) < type_count) return false;
    const uint8_t *type_table = cursor;
    cursor += type_count;
    if (!flag_count || !type_count) return false;

    int32_t relative_base = 0, low_immediate = 0, last_type = -1;
    uint32_t offset_segment = UINT32_MAX;
    while (cursor < end) {
        if ((size_t)(end - cursor) < 2) return false;
        uint16_t command = read_le16(cursor);
        cursor += 2;
        uint32_t flag_index = command & flag_mask;
        uint32_t seg = (command >> flag_bits) & seg_mask;
        uint32_t type_index = (command >> (flag_bits + seg_bits)) & type_mask;
        if (flag_index >= flag_count || type_index >= type_count) return false;
        uint32_t flag = flag_table[flag_index], type = type_table[type_index];

        if (!(flag & 1u)) {
            if (seg >= phnum || !segment_loaded[seg]) return false;
            offset_segment = seg;
            if (!(flag & 6u)) {
                relative_base = (int32_t)(command >> (seg_bits + flag_bits));
            } else if ((flag & 6u) == 4u) {
                if ((size_t)(end - cursor) < 4) return false;
                relative_base = (int32_t)read_le32(cursor);
                cursor += 4;
            } else return false;
            continue;
        }

        if (offset_segment >= phnum || !segment_loaded[offset_segment] ||
            seg >= phnum || !segment_loaded[seg])
            return false;
        uint32_t relocate_to = segment_base[seg];
        uint32_t high_bits = 16u - total_bits;
        uint32_t encoded = command >> total_bits;
        int32_t delta;
        if (!(flag & 6u)) {
            if (high_bits && (encoded & (1u << (high_bits - 1u))))
                encoded |= ~((1u << high_bits) - 1u);
            delta = (int32_t)encoded;
            int64_t sum = (int64_t)relative_base + delta;
            if (sum < INT32_MIN || sum > INT32_MAX) return false;
            relative_base = (int32_t)sum;
        } else if ((flag & 6u) == 2u) {
            if (high_bits && (encoded & (1u << (high_bits - 1u))))
                encoded |= ~((1u << high_bits) - 1u);
            if ((size_t)(end - cursor) < 2) return false;
            delta = (int32_t)((encoded << 16) | read_le16(cursor));
            cursor += 2;
            int64_t sum = (int64_t)relative_base + delta;
            if (sum < INT32_MIN || sum > INT32_MAX) return false;
            relative_base = (int32_t)sum;
        } else if ((flag & 6u) == 4u) {
            if ((size_t)(end - cursor) < 4) return false;
            relative_base = (int32_t)read_le32(cursor);
            cursor += 4;
        } else return false;

        if (!(flag & 0x38u)) low_immediate = 0;
        else if ((flag & 0x38u) == 0x08u) {
            if (last_type != 4) low_immediate = 0;
        } else if ((flag & 0x38u) == 0x10u) {
            if ((size_t)(end - cursor) < 2) return false;
            low_immediate = (int16_t)read_le16(cursor);
            cursor += 2;
        } else return false;

        if (relative_base < 0 ||
            (uint32_t)relative_base > segment_size[offset_segment] ||
            segment_size[offset_segment] - (uint32_t)relative_base < 4)
            return false;
        uint64_t patch_address = (uint64_t)segment_base[offset_segment] +
            (uint32_t)relative_base;
        if (patch_address > UINT32_MAX || (patch_address & 3u) != 0)
            return false;
        uint32_t patch_offset;
        if (!elf_memory_range(&core->memory, (uint32_t)patch_address, 4,
                              &patch_offset))
            return false;

        uint32_t op = read_le32(core->memory.ram + patch_offset);
        uint32_t original = op;
        switch (type) {
            case 0: break;
            case 2: op += relocate_to; break;
            case 3:
            case 6:
            case 7:
                op = (op & 0xFC000000u) |
                    (((op & 0x03FFFFFFu) + (relocate_to >> 2)) & 0x03FFFFFFu);
                if (type == 6) op = (op & 0x03FFFFFFu) | 0x08000000u;
                if (type == 7) op = (op & 0x03FFFFFFu) | 0x0C000000u;
                break;
            case 4: {
                uint32_t address = ((op << 16) + (uint32_t)low_immediate) +
                    relocate_to;
                if (address & 0x8000u) address += 0x10000u;
                op = (op & 0xFFFF0000u) | ((address >> 16) & 0xFFFFu);
                break;
            }
            case 1:
            case 5:
                op = (op & 0xFFFF0000u) |
                    (((op & 0xFFFFu) + relocate_to) & 0xFFFFu);
                break;
            default: return false;
        }
        if (op != original)
            write_le32(core->memory.ram + patch_offset, op);
        last_type = (int32_t)type;
    }
    return true;
}

static bool load_elf_segments_in_place(
    PSP_Core *core,
    const uint8_t *elf,
    size_t elf_size
)
{
    uint32_t segment_base[ELF_MAX_PROGRAM_HEADERS] = {0};
    int64_t segment_delta[ELF_MAX_PROGRAM_HEADERS] = {0};
    uint32_t segment_size[ELF_MAX_PROGRAM_HEADERS] = {0};
    bool segment_loaded[ELF_MAX_PROGRAM_HEADERS] = {false};

    if (core == NULL || elf == NULL ||
        elf_size < ELF_HEADER_SIZE)
        return false;

    if (!psp_loader_is_elf(elf, elf_size))
        return false;

    uint16_t elf_type = read_le16(elf + 16);

    if ((elf_type != ELF_TYPE_EXEC &&
         elf_type != ELF_TYPE_PRX) ||
        read_le16(elf + 18) != ELF_MACHINE_MIPS ||
        read_le32(elf + 20) != 1)
        return false;

    bool is_prx = (elf_type == ELF_TYPE_PRX);

    uint32_t entry = read_le32(elf + 24);
    uint32_t phoff = read_le32(elf + 28);

    uint16_t phentsize = read_le16(elf + 42);
    uint16_t phnum = read_le16(elf + 44);

    if (phentsize < ELF_PROGRAM_HEADER_SIZE ||
        phnum == 0 ||
        phnum > ELF_MAX_PROGRAM_HEADERS ||
        phoff > elf_size)
        return false;

    if ((size_t)phnum >
        (elf_size - phoff) / phentsize)
        return false;

    uint32_t lowest_vaddr = UINT32_MAX;
    bool has_loadable_segment = false;

    /* Find the lowest virtual address for PRX mapping. */
    for (uint16_t i = 0; i < phnum; i++)
    {
        const uint8_t *ph =
            elf + phoff + ((size_t)i * phentsize);

        if (read_le32(ph) != ELF_PT_LOAD)
            continue;

        uint32_t vaddr = read_le32(ph + 8);
        uint32_t memsz = read_le32(ph + 20);

        if (memsz == 0)
            continue;

        if (vaddr < lowest_vaddr)
            lowest_vaddr = vaddr;

        has_loadable_segment = true;
    }

    if (!has_loadable_segment)
        return false;

    bool entry_is_executable = false;

    /* Validate and calculate every loadable segment. */
    for (uint16_t i = 0; i < phnum; i++)
    {
        const uint8_t *ph =
            elf + phoff + ((size_t)i * phentsize);

        if (read_le32(ph) != ELF_PT_LOAD)
            continue;

        uint32_t file_offset = read_le32(ph + 4);
        uint32_t vaddr = read_le32(ph + 8);
        uint32_t filesz = read_le32(ph + 16);
        uint32_t memsz = read_le32(ph + 20);
        uint32_t flags = read_le32(ph + 24);

        if (filesz > memsz ||
            file_offset > elf_size ||
            filesz > elf_size - file_offset)
            return false;

        if (memsz == 0)
            continue;

        uint64_t mapped_address;

        if (is_prx)
        {
            if (vaddr < lowest_vaddr)
                return false;

            mapped_address =
                (uint64_t)PSP_RAM_BASE +
                ((uint64_t)vaddr - lowest_vaddr);
        }
        else
        {
            mapped_address = vaddr;
        }

        if (mapped_address > UINT32_MAX)
            return false;

        uint32_t mapped = (uint32_t)mapped_address;
        uint32_t memory_offset;

        if (!elf_memory_range(
                &core->memory,
                mapped,
                memsz,
                &memory_offset))
            return false;

        segment_base[i] = mapped;
        segment_delta[i] =
            (int64_t)mapped - (int64_t)vaddr;
        segment_size[i] = memsz;
        segment_loaded[i] = true;

        if ((flags & ELF_FLAG_EXECUTE) &&
            entry >= vaddr &&
            (uint64_t)entry <
                (uint64_t)vaddr + memsz)
        {
            entry_is_executable = true;
        }

        (void)memory_offset;
    }

    if (!entry_is_executable)
        return false;

    uint64_t mapped_entry = entry;

    if (is_prx)
    {
        if (entry < lowest_vaddr)
            return false;

        mapped_entry =
            (uint64_t)PSP_RAM_BASE +
            ((uint64_t)entry - lowest_vaddr);
    }

    if (mapped_entry > UINT32_MAX)
        return false;

    /* Clear RAM and copy PT_LOAD segments. */
    memset(core->memory.ram, 0, core->memory.size);

    for (uint16_t i = 0; i < phnum; i++)
    {
        const uint8_t *ph =
            elf + phoff + ((size_t)i * phentsize);

        if (read_le32(ph) != ELF_PT_LOAD ||
            !segment_loaded[i])
            continue;

        uint32_t file_offset = read_le32(ph + 4);
        uint32_t filesz = read_le32(ph + 16);
        uint32_t memsz = read_le32(ph + 20);

        uint32_t memory_offset;

        if (!elf_memory_range(
                &core->memory,
                segment_base[i],
                memsz,
                &memory_offset))
            return false;

        if (filesz > 0)
        {
            memcpy(
                core->memory.ram + memory_offset,
                elf + file_offset,
                filesz
            );
        }

        if (memsz > filesz)
        {
            memset(
                core->memory.ram + memory_offset + filesz,
                0,
                memsz - filesz
            );
        }
    }

    /*
     * Apply PSP PT_PSPREL1 and compressed PT_PSPREL2 relocations.
     */
    if (is_prx)
    {
        for (uint16_t i = 0; i < phnum; i++)
        {
            const uint8_t *ph =
                elf + phoff + ((size_t)i * phentsize);

            uint32_t type = read_le32(ph);

            if (type == ELF_PT_PSPREL2) {
                if (!apply_psprel2(core, elf, elf_size, phnum, phentsize, i,
                                   segment_base, segment_size, segment_loaded))
                    return false;
                continue;
            }

            if (type != ELF_PT_PSPREL1)
                continue;

            uint32_t reloc_offset = read_le32(ph + 4);
            uint32_t reloc_size = read_le32(ph + 16);

            if (reloc_offset > elf_size ||
                reloc_size > elf_size - reloc_offset ||
                (reloc_size % 8) != 0)
                return false;

            uint32_t reloc_count = reloc_size / 8;

            for (uint32_t r = 0; r < reloc_count; r++)
            {
                const uint8_t *rel =
                    elf + reloc_offset + (r * 8);

                uint32_t r_offset = read_le32(rel);
                uint32_t r_info = read_le32(rel + 4);

                uint32_t reloc_type = r_info & 0xFF;
                uint32_t write_index =
                    (r_info >> 8) & 0xFF;
                uint32_t relative_index =
                    (r_info >> 16) & 0xFF;

                if (reloc_type == MIPS_R_NONE ||
                    reloc_type == MIPS_R_GPREL16)
                    continue;

                if (write_index >= phnum ||
                    relative_index >= phnum ||
                    !segment_loaded[write_index] ||
                    !segment_loaded[relative_index])
                    return false;

                if (segment_size[write_index] < 4 ||
                    r_offset > segment_size[write_index] - 4)
                    return false;

                uint64_t patch_address =
                    (uint64_t)segment_base[write_index] +
                    r_offset;

                if (patch_address > UINT32_MAX)
                    return false;

                uint32_t patch_offset;

                if (!elf_memory_range(
                        &core->memory,
                        (uint32_t)patch_address,
                        4,
                        &patch_offset))
                    return false;

                uint8_t *patch =
                    core->memory.ram + patch_offset;

                uint32_t instruction = read_le32(patch);

                int64_t delta =
                    segment_delta[relative_index];

                switch (reloc_type)
                {
                    case MIPS_R_32:
                    {
                        write_le32(
                            patch,
                            (uint32_t)(
                                (int64_t)instruction + delta
                            )
                        );
                        break;
                    }

                    case MIPS_R_26:
                    {
                        uint32_t target =
                            (instruction & 0x03FFFFFF) << 2;

                        target = (uint32_t)(
                            (int64_t)target + delta
                        );

                        instruction =
                            (instruction & 0xFC000000) |
                            ((target >> 2) & 0x03FFFFFF);

                        write_le32(patch, instruction);
                        break;
                    }

                    case MIPS_R_HI16:
                    {
                        bool found_pair = false;

                        for (uint32_t next = r + 1;
                             next < reloc_count;
                             next++)
                        {
                            const uint8_t *lo_rel =
                                elf + reloc_offset + (next * 8);

                            uint32_t lo_info =
                                read_le32(lo_rel + 4);

                            uint32_t lo_type =
                                lo_info & 0xFF;

                            uint32_t lo_write =
                                (lo_info >> 8) & 0xFF;

                            uint32_t lo_relative =
                                (lo_info >> 16) & 0xFF;

                            if (lo_type == MIPS_R_HI16)
                                continue;

                            if (lo_write != write_index ||
                                lo_relative != relative_index)
                                continue;

                            if (lo_type != MIPS_R_LO16 &&
                                lo_type != MIPS_R_16)
                                continue;

                            uint32_t lo_offset =
                                read_le32(lo_rel);

                            if (segment_size[lo_write] < 4 ||
                                lo_offset > segment_size[lo_write] - 4)
                                return false;

                            uint64_t lo_address =
                                (uint64_t)segment_base[lo_write] +
                                lo_offset;

                            if (lo_address > UINT32_MAX)
                                return false;

                            uint32_t lo_memory_offset;

                            if (!elf_memory_range(
                                    &core->memory,
                                    (uint32_t)lo_address,
                                    4,
                                    &lo_memory_offset))
                                return false;

                            uint32_t lo_instruction =
                                read_le32(
                                    core->memory.ram +
                                    lo_memory_offset
                                );

                            int64_t combined =
                                ((int64_t)(
                                    instruction & 0xFFFF
                                ) << 16) +
                                (int16_t)(
                                    lo_instruction & 0xFFFF
                                );

                            combined += delta;

                            uint32_t high =
                                (uint32_t)(
                                    (combined + 0x8000) >> 16
                                );

                            instruction =
                                (instruction & 0xFFFF0000) |
                                (high & 0xFFFF);

                            write_le32(patch, instruction);

                            found_pair = true;
                            break;
                        }

                        if (!found_pair)
                            return false;

                        break;
                    }

                    case MIPS_R_LO16:
                    case MIPS_R_16:
                    {
                        uint32_t low = (uint32_t)(
                            (int64_t)(instruction & 0xFFFF) +
                            delta
                        );

                        instruction =
                            (instruction & 0xFFFF0000) |
                            (low & 0xFFFF);

                        write_le32(patch, instruction);
                        break;
                    }

                    default:
                        return false;
                }
            }
        }
    }

    core->cpu.pc = (uint32_t)mapped_entry;
    core->cpu.halted = false;
    core->cpu.gpr[0] = 0;

    return true;
}


/*
 * Load into temporary RAM so malformed ELF/PRX data cannot partially
 * overwrite the currently mapped memory. Commit only after every
 * segment and relocation has been validated and applied successfully.
 */
static bool load_elf_segments(
    PSP_Core *core,
    const uint8_t *elf,
    size_t elf_size
)
{
    if (core == NULL || core->memory.ram == NULL ||
        core->memory.size == 0)
        return false;

    uint8_t *original_ram = core->memory.ram;
    uint8_t *staging_ram = malloc(core->memory.size);

    if (staging_ram == NULL)
        return false;

    core->memory.ram = staging_ram;

    bool loaded = load_elf_segments_in_place(
        core,
        elf,
        elf_size
    );

    /* Resolve PRX imports in staging RAM so unresolved NIDs cannot leave
       partially patched code or replace the currently loaded image. */
    if (loaded)
        loaded = psp_prx_resolve_imports(
            elf, elf_size, staging_ram, core->memory.size);

    core->memory.ram = original_ram;

    if (loaded)
        memcpy(original_ram, staging_ram, core->memory.size);

    free(staging_ram);
    return loaded;
}

static void set_memory_floor_from_elf(
    PSP_Core *core, const uint8_t *elf, size_t elf_size)
{
    if (!core || !elf || elf_size < ELF_HEADER_SIZE)
        return;

    bool is_prx = read_le16(elf + 16) == ELF_TYPE_PRX;
    uint32_t phoff = read_le32(elf + 28);
    uint16_t phentsize = read_le16(elf + 42);
    uint16_t phnum = read_le16(elf + 44);
    if (phentsize < ELF_PROGRAM_HEADER_SIZE || phoff > elf_size ||
        (size_t)phnum > (elf_size - phoff) / phentsize)
        return;

    uint32_t lowest = UINT32_MAX;
    for (uint16_t i = 0; i < phnum; i++) {
        const uint8_t *ph = elf + phoff + (size_t)i * phentsize;
        if (read_le32(ph) == ELF_PT_LOAD && read_le32(ph + 20) != 0 &&
            read_le32(ph + 8) < lowest)
            lowest = read_le32(ph + 8);
    }
    if (lowest == UINT32_MAX)
        return;

    uint64_t highest_end = PSP_RAM_BASE;
    for (uint16_t i = 0; i < phnum; i++) {
        const uint8_t *ph = elf + phoff + (size_t)i * phentsize;
        if (read_le32(ph) != ELF_PT_LOAD)
            continue;
        uint32_t vaddr = read_le32(ph + 8);
        uint32_t memsz = read_le32(ph + 20);
        if (!memsz)
            continue;

        uint64_t mapped = is_prx
            ? (uint64_t)PSP_RAM_BASE + ((uint64_t)vaddr - lowest)
            : (uint64_t)vaddr;
        uint64_t end = mapped + memsz;
        if (end > highest_end)
            highest_end = end;
    }

    if (highest_end < PSP_RAM_BASE)
        return;
    uint64_t used_end = highest_end - PSP_RAM_BASE;
    if (used_end > core->memory.size)
        used_end = core->memory.size;
    psp_memory_blocks_set_floor(
        &core->memory_blocks, (uint32_t)used_end);
}

    bool psp_core_init(PSP_Core *core)
{
    if (core == NULL)
        return false;

    memset(core, 0, sizeof(*core));

    psp_syscall_reset();
    psp_nid_reset();

    if (!psp_memory_init(&core->memory))
        return false;

    if (!psp_video_init(&core->video))
    {
        psp_memory_destroy(&core->memory);
        return false;
    }

    psp_cpu_init(&core->cpu);
    psp_controller_reset(&core->controller);
    psp_memory_blocks_init(&core->memory_blocks, &core->memory);
    psp_io_service_init(&core->io_service, &core->iso_fs);

    if (!psp_controller_hle_register(&core->controller) ||
        !psp_memory_hle_register(&core->memory_blocks) ||
        !psp_io_hle_register(&core->io_service)) {
        psp_io_hle_unregister();
        psp_memory_hle_unregister();
        psp_controller_hle_unregister();
        psp_video_destroy(&core->video);
        psp_memory_destroy(&core->memory);
        psp_syscall_reset();
        psp_nid_reset();
        return false;
    }

    core->initialized = true;

    return true;
}

void psp_core_reset(PSP_Core *core)
{
    if (core == NULL || !core->initialized)
        return;

    psp_cpu_reset(&core->cpu);
    psp_controller_reset(&core->controller);
    psp_memory_blocks_reset(&core->memory_blocks);
    psp_io_service_reset(&core->io_service);
    psp_video_clear(&core->video, 0xFF000000u);
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
    psp_io_service_set_fs(&core->io_service, NULL);
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

    psp_io_hle_unregister();
    psp_memory_hle_unregister();
    psp_controller_hle_unregister();
    psp_syscall_reset();
    psp_nid_reset();

    psp_core_unload_executable(core);
    psp_core_unmount_iso(core);

    psp_video_destroy(&core->video);
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
    psp_io_service_set_fs(&core->io_service, &core->iso_fs);

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

    set_memory_floor_from_elf(core, elf, core->image.executable_size);
    core->image_loaded = true;

    return true;
}
bool psp_core_load_executable_data(
    PSP_Core *core,
    const uint8_t *data,
    size_t size
)
{
    if (core == NULL || !core->initialized ||
        data == NULL || size == 0)
        return false;

    psp_core_unload_executable(core);

    uint8_t *copy = malloc(size);

    if (copy == NULL)
        return false;

    memcpy(copy, data, size);

    core->image.data = copy;
    core->image.size = size;
    core->image.loaded = true;

    if (!psp_loader_parse(&core->image) ||
        !core->image.executable_valid)
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

    set_memory_floor_from_elf(core, elf, core->image.executable_size);
    core->image_loaded = true;
    return true;
}
