#include "psp_core.h"
#include "psp_nid.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define TEST_ELF_SIZE 0xA8u
#define TEST_PRX_SIZE 0x240u
#define TEST_ENTRY PSP_RAM_BASE
#define TEST_DATA_ENTRY (PSP_RAM_BASE + 0x00801000u)

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
    write_le16(elf + 44, 4);

    /* Executable PRX segment, virtual address 0x1000. */
    write_le32(elf + 52, 1);
    write_le32(elf + 56, 0x100);
    write_le32(elf + 60, 0x1000);
    write_le32(elf + 68, 28);
    write_le32(elf + 72, 28);
    write_le32(elf + 76, 5);
    write_le32(elf + 80, 4);
    write_le32(elf + 0x100, 0x2408002A);
    write_le32(elf + 0x104, 0x00002004); /* R_MIPS_32 */
    write_le32(elf + 0x108, 0x00002004); /* R_MIPS_16 */
    write_le32(elf + 0x10C, 0x08000800); /* J, R_MIPS_26 */
    write_le32(elf + 0x110, 0x3C080000); /* LUI, R_MIPS_HI16 */
    write_le32(elf + 0x114, 0x25082000); /* ADDIU, R_MIPS_LO16 */
    write_le32(elf + 0x118, 0x3C090000); /* second LUI, shared LO16 */

    /* Data PRX segment, virtual address 0x2000. */
    write_le32(elf + 84, 1);
    write_le32(elf + 88, 0x120);
    write_le32(elf + 92, 0x2000);
    write_le32(elf + 100, 8);
    write_le32(elf + 104, 8);
    write_le32(elf + 108, 6);
    write_le32(elf + 112, 4);
    write_le32(elf + 0x120, 0xAABBCCDD);
    write_le32(elf + 0x124, 0);

    /* PSPREL1 table: R_MIPS_32, R_MIPS_16, R_MIPS_26 and HI/LO pairs. */
    write_le32(elf + 116, 0x700000A0);
    write_le32(elf + 120, 0x200);
    write_le32(elf + 132, 48);
    write_le32(elf + 0x200, 4);
    write_le32(elf + 0x204, (1u << 16) | 2u);
    write_le32(elf + 0x208, 8);
    write_le32(elf + 0x20C, (1u << 16) | 1u);
    write_le32(elf + 0x210, 12);
    write_le32(elf + 0x214, (1u << 16) | 4u);
    write_le32(elf + 0x218, 16);
    write_le32(elf + 0x21C, (1u << 16) | 5u);
    write_le32(elf + 0x220, 24);
    write_le32(elf + 0x224, (1u << 16) | 5u);
    write_le32(elf + 0x228, 20);
    write_le32(elf + 0x22C, (1u << 16) | 6u);

    /* PSPREL2 segment #3: offset-base command, then compressed R_MIPS_32. */
    write_le32(elf + 148, 0x700000A1);
    write_le32(elf + 152, 0x230);
    write_le32(elf + 164, 13);
    write_le32(elf + 168, 13);
    elf[0x230] = 0;
    elf[0x231] = 0;
    elf[0x232] = 3; /* flag bits */
    elf[0x233] = 3; /* type bits */
    elf[0x234] = 2; /* flag table length */
    elf[0x235] = 0; /* set offset segment/base */
    elf[0x236] = 1; /* relocate this command */
    elf[0x237] = 1; /* type table length */
    elf[0x238] = 2; /* R_MIPS_32 */
    write_le16(elf + 0x239, 0x0008); /* offset segment 1, base 0 */
    write_le16(elf + 0x23B, 0x0401); /* add offset 4; target segment 0 */
    write_le32(elf + 0x124, 0x20); /* addend for PSPREL2 R_MIPS_32 */
}

int main(void)
{
    PSP_Core core;
    uint8_t elf[TEST_ELF_SIZE];
    uint8_t prx[TEST_PRX_SIZE];
    uint8_t iso_fixture[18u * 2048u] = {0};

    memset(&core, 0, sizeof(core));
    assert(psp_core_init(&core));

    /* Core startup must register controller, memory, and file HLE NIDs. */
    uint32_t service_code = 0;
    assert(psp_nid_resolve(0x237DBD4Fu, &service_code));
    assert(psp_nid_resolve(0xB6D61D02u, &service_code));
    assert(psp_nid_resolve(0x9D9A5BA1u, &service_code));
    assert(psp_nid_resolve(0x109F50BCu, &service_code));
    assert(psp_nid_resolve(0x810C4BC3u, &service_code));
    assert(psp_nid_resolve(0x6A638D83u, &service_code));
    assert(psp_nid_resolve(0x27EB27B8u, &service_code));

    /* Mounting an ISO must expose its filesystem to the sceIo HLE layer. */
    uint8_t *pvd = iso_fixture + 16u * 2048u;
    pvd[0] = 1;
    memcpy(pvd + 1, "CD001", 5);
    pvd[6] = 1;
    uint8_t *root_record = pvd + 156;
    root_record[0] = 34;
    write_le32(root_record + 2, 17);
    write_le32(root_record + 10, 2048);
    assert(psp_core_mount_iso(&core, iso_fixture, sizeof(iso_fixture)));
    assert(core.io_service.fs == &core.iso_fs);
    psp_core_unmount_iso(&core);
    assert(core.io_service.fs == NULL);

    make_test_elf(elf);

    assert(psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(core.image_loaded);
    assert(core.memory_blocks.arena_start > 0x00800000u);
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
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 0x1004) ==
           PSP_RAM_BASE + 0x20);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 16) ==
           0x3C080800);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 20) ==
           0x25081000);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 24) ==
           0x3C090800);

    /* A paired LO16 relocation outside its write segment must be rejected. */
    write_le32(prx + 0x228, 28);
    core.cpu.pc = 0x87654321;
    assert(!psp_core_load_executable_data(
        &core, prx, sizeof(prx)));
    /* Failed PRX loading must preserve RAM and the CPU program counter. */
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 4) ==
           PSP_RAM_BASE + 0x1004);
    assert(psp_memory_read32(&core.memory, PSP_RAM_BASE + 16) ==
           0x3C080800);
    assert(core.cpu.pc == 0x87654321);

    /* An entry point outside executable memory must be rejected. */
    write_le32(elf + 24, TEST_DATA_ENTRY);
    core.cpu.pc = 0x12345678;
    assert(!psp_core_load_executable_data(
        &core, elf, sizeof(elf)));
    assert(!core.image_loaded);
    assert(core.cpu.pc == 0x12345678);

    psp_core_destroy(&core);
    return 0;
}
