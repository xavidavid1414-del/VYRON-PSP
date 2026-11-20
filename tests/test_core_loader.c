#include "psp_core.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define TEST_ELF_SIZE 0x84u
#define TEST_ENTRY PSP_RAM_BASE

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
    write_le16(elf + 44, 1);

    write_le32(elf + 52, 1); /* PT_LOAD */
    write_le32(elf + 56, 0x80);
    write_le32(elf + 60, TEST_ENTRY);
    write_le32(elf + 68, 4); /* filesz */
    write_le32(elf + 72, 8); /* memsz */
    write_le32(elf + 76, 5); /* PF_R | PF_X */
    write_le32(elf + 80, 4);

    write_le32(elf + 0x80, 0); /* MIPS NOP */
}

int main(void)
{
    PSP_Core core;
    uint8_t elf[TEST_ELF_SIZE];

    memset(&core, 0, sizeof(core));
    assert(psp_memory_init(&core.memory));
    psp_cpu_init(&core.cpu);
    core.initialized = true;

    make_test_elf(elf);

    assert(psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(core.image_loaded);
    assert(core.cpu.pc == TEST_ENTRY);
    assert(psp_memory_read32(&core.memory, TEST_ENTRY) == 0);
    assert(psp_memory_read8(&core.memory, TEST_ENTRY + 4) == 0);
    assert(psp_memory_read8(&core.memory, TEST_ENTRY + 5) == 0);
    assert(psp_memory_read8(&core.memory, TEST_ENTRY + 6) == 0);
    assert(psp_memory_read8(&core.memory, TEST_ENTRY + 7) == 0);

    /* The loaded entry point must execute a MIPS instruction. */
    assert(psp_core_run(&core, 1));
    assert(core.cpu.pc == TEST_ENTRY + 4);

    /* An entry point outside executable memory must be rejected. */
    write_le32(elf + 24, TEST_ENTRY + 0x100);
    core.cpu.pc = 0x12345678;
    assert(!psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(!core.image_loaded);
    assert(core.cpu.pc == 0x12345678);

    psp_core_unload_executable(&core);
    psp_memory_destroy(&core.memory);
    return 0;
}
