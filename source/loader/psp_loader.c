#include "psp_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PBP_HEADER_SIZE 40
#define PBP_OFFSET_COUNT 8

#define PBP_DATA_PSP_INDEX 6
#define PBP_DATA_PSAR_INDEX 7

#define ELF_HEADER_SIZE 52

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

bool psp_loader_is_elf(
    const uint8_t *data,
    size_t size
)
{
    if (data == NULL || size < ELF_HEADER_SIZE)
        return false;

    if (data[0] != 0x7F ||
        data[1] != 'E' ||
        data[2] != 'L' ||
        data[3] != 'F')
        return false;

    /* ELF32 */
    if (data[4] != 1)
        return false;

    /* Little endian ELF with current identification version. */
    if (data[5] != 1 || data[6] != 1)
        return false;

    /* ELF32 header size must cover the complete standard header. */
    uint16_t header_size =
        (uint16_t)data[40] | ((uint16_t)data[41] << 8);

    if (header_size < ELF_HEADER_SIZE || header_size > size)
        return false;

    return true;
}

bool psp_loader_parse(PSP_Image *image)
{
    if (image == NULL ||
        image->data == NULL ||
        image->size == 0)
        return false;

    image->format = PSP_IMAGE_UNKNOWN;
    image->executable_offset = 0;
    image->executable_size = 0;
    image->executable_valid = false;

    /* Direct ELF file */
    if (psp_loader_is_elf(image->data, image->size))
    {
        image->format = PSP_IMAGE_ELF;
        image->executable_offset = 0;
        image->executable_size = image->size;
        image->executable_valid = true;

        return true;
    }

    /* PBP identification */
    if (image->size < PBP_HEADER_SIZE)
        return false;

    if (memcmp(image->data, "\0PBP", 4) != 0)
        return false;

    uint32_t offsets[PBP_OFFSET_COUNT];

    for (size_t i = 0; i < PBP_OFFSET_COUNT; i++)
    {
        offsets[i] = read_le32(
            image->data + 8 + (i * 4)
        );

        if (offsets[i] < PBP_HEADER_SIZE ||
            offsets[i] > image->size)
            return false;

        if (i > 0 && offsets[i] < offsets[i - 1])
            return false;
    }

    size_t executable_start =
        offsets[PBP_DATA_PSP_INDEX];

    size_t executable_end =
        offsets[PBP_DATA_PSAR_INDEX];

    if (executable_end <= executable_start)
        return false;

    size_t executable_size =
        executable_end - executable_start;

    image->format = PSP_IMAGE_PBP;
    image->executable_offset = executable_start;
    image->executable_size = executable_size;

    image->executable_valid = psp_loader_is_elf(
        image->data + executable_start,
        executable_size
    );

    return true;
}

bool psp_loader_load(
    const char *path,
    PSP_Image *image
)
{
    if (path == NULL || image == NULL)
        return false;

    image->data = NULL;
    image->size = 0;
    image->loaded = false;
    image->format = PSP_IMAGE_UNKNOWN;
    image->executable_offset = 0;
    image->executable_size = 0;
    image->executable_valid = false;

    FILE *file = fopen(path, "rb");

    if (file == NULL)
        return false;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    long file_size = ftell(file);

    if (file_size <= 0)
    {
        fclose(file);
        return false;
    }

    rewind(file);

    uint8_t *buffer = malloc((size_t)file_size);

    if (buffer == NULL)
    {
        fclose(file);
        return false;
    }

    size_t bytes_read = fread(
        buffer,
        1,
        (size_t)file_size,
        file
    );

    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(buffer);
        return false;
    }

    image->data = buffer;
    image->size = bytes_read;
    image->loaded = true;

    if (!psp_loader_parse(image))
    {
        psp_loader_unload(image);
        return false;
    }

    return true;
}

void psp_loader_unload(PSP_Image *image)
{
    if (image == NULL)
        return;

    free(image->data);

    image->data = NULL;
    image->size = 0;
    image->loaded = false;

    image->format = PSP_IMAGE_UNKNOWN;
    image->executable_offset = 0;
    image->executable_size = 0;
    image->executable_valid = false;
}
