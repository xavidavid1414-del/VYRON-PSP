#include "psp_core.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define TEST_ELF_SIZE 0xA8u
#define TEST_ENTRY PSP_RAM_BASE
#define TEST_DATA_ENTRY (PSP_RAM_BASE + 0x100u)

static void write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void make_test_elf(uint8_t *elf)
{
    memset(elf, 0, TEST_ELF_SIZE);

    elf[0] = 0x7F;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 1;
    elf[5] = 1;
    elf[6] = 1;

    write_le16(elf + 16, 2); /* ET_EXEC */
    write_le16(elf + 18, 8); /* MIPS */
    write_le32(elf + 20, 1);
    write_le32(elf + 24, TEST_ENTRY);
    write_le32(elf + 28, 52);
    write_le16(elf + 40, 52);
    write_le16(elf + 42, 32);
    write_le16(elf + 44, 2);

    /* Executable segment: addiu t0, zero, 42; then four BSS bytes. */
    write_le32(elf + 52, 1); /* PT_LOAD */
    write_le32(elf + 56, 0xA0);
    write_le32(elf + 60, TEST_ENTRY);
    write_le32(elf + 68, 4); /* filesz */
    write_le32(elf + 72, 8); /* memsz */
    write_le32(elf + 76, 5); /* PF_R | PF_X */
    write_le32(elf + 80, 4);
    write_le32(elf + 0xA0, 0x2408002A);

    /* Separate writable data segment with its own BSS tail. */
    write_le32(elf + 84, 1); /* PT_LOAD */
    write_le32(elf + 88, 0xA0 + 4);
    write_le32(elf + 92, TEST_DATA_ENTRY);
    write_le32(elf + 100, 4); /* filesz */
    write_le32(elf + 104, 8); /* memsz */
    write_le32(elf + 108, 6); /* PF_R | PF_W */
    write_le32(elf + 112, 4);
    write_le32(elf + 0xA4, 0x12345678);
}


static void make_test_prx(uint8_t *elf, size_t size)
{
    memset(elf, 0, size);

    elf[0] = 0x7F;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 1;
    elf[5] = 1;
    elf[6] = 1;

    write_le16(elf + 16, 0xFFA0); /* PSP ET_PRX */
    write_le16(elf + 18, 8);      /* MIPS */
    write_le32(elf + 20, 1);
    write_le32(elf + 24, 0x1000);
    write_le32(elf + 28, 52);
    write_le16(elf + 40, 52);
    write_le16(elf + 42, 32);
    write_le16(elf + 44, 3);

    /* Executable PRX segment, virtual address 0x1000. */
    write_le32(elf + 52, 1);
    write_le32(elf + 56, 0x100);
    write_le32(elf + 60, 0x1000);
    write_le32(elf + 68, 24);
    write_le32(elf + 72, 24);
    write_le32(elf + 76, 5);
    write_le32(elf + 80, 4);
    write_le32(elf + 0x100, 0x2408002A);
    write_le32(elf + 0x104, 0x00002004); /* R_MIPS_32 */
    write_le32(elf + 0x108, 0x00002004); /* R_MIPS_16 */
    write_le32(elf + 0x10C, 0x08000800); /* J, R_MIPS_26 */
    write_le32(elf + 0x110, 0x3C080000); /* LUI, R_MIPS_HI16 */
    write_le32(elf + 0x114, 0x25082000); /* ADDIU, R_MIPS_LO16 */

    /* Data PRX segment, virtual address 0x2000. */
    write_le32(elf + 84, 1);
    write_le32(elf + 88, 0x118);
    write_le32(elf + 92, 0x2000);
    write_le32(elf + 100, 8);
    write_le32(elf + 104, 8);
    write_le32(elf + 108, 6);
    write_le32(elf + 112, 4);
    write_le32(elf + 0x118, 0xAABBCCDD);
    write_le32(elf + 0x11C, 0);

    /* PSP relocation table: R_MIPS_32, R_MIPS_16 and R_MIPS_26. */
    write_le32(elf + 116, 0x700000A0);
    write_le32(elf + 120, 0x128);
    write_le32(elf + 132, 40);
    write_le32(elf + 0x128, 4);
    write_le32(elf + 0x12C, (1u << 16) | 2u);
    write_le32(elf + 0x130, 8);
    write_le32(elf + 0x134, (1u << 16) | 1u);
    write_le32(elf + 0x138, 12);
    write_le32(elf + 0x13C, (1u << 16) | 4u);
    write_le32(elf + 0x140, 16);
    write_le32(elf + 0x144, (1u << 16) | 5u);
    write_le32(elf + 0x148, 20);
    write_le32(elf + 0x14C, (1u << 16) | 6u);
}

int main(void)
{
    PSP_Core core;
    uint8_t elf[TEST_ELF_SIZE];
    uint8_t prx[0x150];

    memset(&core, 0, sizeof(core));
    assert(psp_memory_init(&core.memory));
    psp_cpu_init(&core.cpu);
    core.initialized = true;

    make_test_elf(elf);

    assert(psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(core.image_loaded);
    assert(core.cpu.pc == TEST_ENTRY);
    assert(psp_memory_read32(&core.memory, TEST_ENTRY) == 0x2408002A);
    assert(psp_memory_read32(&core.memory, TEST_DATA_ENTRY) == 0x12345678);

    /* Both PT_LOAD segments must have zero-filled BSS tails. */
    for (uint32_t i = 4; i < 8; i++) {
        assert(psp_memory_read8(&core.memory, TEST_ENTRY + i) == 0);
        assert(psp_memory_read8(&core.memory, TEST_DATA_ENTRY + i) == 0);
    }

    /* Execute a real arithmetic instruction from the loaded segment. */
    assert(psp_core_run(&core, 1));
    assert(core.cpu.gpr[8] == 42);
    assert(core.cpu.pc == TEST_ENTRY + 4);

    /* Load a PRX with two segments and apply an R_MIPS_32 relocation. */
    make_test_prx(prx, sizeof(prx));
    assert(psp_core_load_executable_data(
        &core, prx, sizeof(prx)));
    assert(core.image_loaded);
    assert(core.cpu.pc == PSP_RAM_BASE);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 4) ==
           PSP_RAM_BASE + 0x1004);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 8) ==
           0x00001004);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 12) ==
           0x0A000400);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 0x1000) ==
           0xAABBCCDD);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 16) ==
           0x3C080800);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 20) ==
           0x25081000);

    /* An entry point outside executable memory must be rejected. */
    write_le32(elf + 24, TEST_DATA_ENTRY);
    core.cpu.pc = 0x12345678;
    assert(!psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(!core.image_loaded);
    assert(core.cpu.pc == 0x12345678);

    psp_core_unload_executable(&core);
    psp_memory_destroy(&core.memory);
    return 0;
}
