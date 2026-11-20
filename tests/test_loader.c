#include "psp_loader.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void make_elf(uint8_t *elf, size_t size)
{
    memset(elf, 0, size);
    elf[0] = 0x7F;
    elf[1] = 'E';
    elf[2] = 'L';
    elf[3] = 'F';
    elf[4] = 1;
    elf[5] = 1;
    elf[6] = 1;
    elf[40] = 52;
}

int main(void)
{
    uint8_t elf[52];
    make_elf(elf, sizeof(elf));

    assert(psp_loader_is_elf(elf, sizeof(elf)));

    uint8_t damaged[52];
    memcpy(damaged, elf, sizeof(damaged));
    damaged[4] = 2; /* ELF64 is not supported. */
    assert(!psp_loader_is_elf(damaged, sizeof(damaged)));

    memcpy(damaged, elf, sizeof(damaged));
    damaged[5] = 2; /* Big-endian ELF is not supported. */
    assert(!psp_loader_is_elf(damaged, sizeof(damaged)));

    memcpy(damaged, elf, sizeof(damaged));
    damaged[40] = 51; /* Truncated ELF header declaration. */
    damaged[41] = 0;
    assert(!psp_loader_is_elf(damaged, sizeof(damaged)));

    PSP_Image image = {
        .data = elf,
        .size = sizeof(elf)
    };

    assert(psp_loader_parse(&image));
    assert(image.format == PSP_IMAGE_ELF);
    assert(image.executable_valid);
    assert(image.executable_offset == 0);
    assert(image.executable_size == sizeof(elf));

    uint8_t pbp[40 + sizeof(elf)];
    memset(pbp, 0, sizeof(pbp));
    memcpy(pbp, "\\0PBP", 4);

    for (uint32_t i = 0; i < 8; i++)
        write_le32(pbp + 8 + i * 4, 40);

    write_le32(pbp + 8 + 6 * 4, 40);
    write_le32(pbp + 8 + 7 * 4, (uint32_t)sizeof(pbp));
    memcpy(pbp + 40, elf, sizeof(elf));

    image.data = pbp;
    image.size = sizeof(pbp);
    assert(psp_loader_parse(&image));
    assert(image.format == PSP_IMAGE_PBP);
    assert(image.executable_valid);
    assert(image.executable_offset == 40);
    assert(image.executable_size == sizeof(elf));

    /* Descending PBP offsets must be rejected. */
    write_le32(pbp + 8 + 5 * 4, 41);
    image.data = pbp;
    image.size = sizeof(pbp);
    assert(!psp_loader_parse(&image));

    /* A PBP with a non-ELF executable payload is identified but not loadable. */
    write_le32(pbp + 8 + 5 * 4, 40);
    pbp[40] = 0;
    image.data = pbp;
    image.size = sizeof(pbp);
    assert(psp_loader_parse(&image));
    assert(image.format == PSP_IMAGE_PBP);
    assert(!image.executable_valid);

    return 0;
}
