#ifndef PSP_LOADER_H
#define PSP_LOADER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    PSP_IMAGE_UNKNOWN = 0,
    PSP_IMAGE_ELF,
    PSP_IMAGE_PBP
} PSP_ImageFormat;

typedef struct {
    uint8_t *data;
    size_t size;

    size_t executable_offset;
    size_t executable_size;

    PSP_ImageFormat format;

    bool loaded;
    bool executable_valid;
} PSP_Image;

bool psp_loader_load(
    const char *path,
    PSP_Image *image
);

void psp_loader_unload(PSP_Image *image);

bool psp_loader_is_elf(
    const uint8_t *data,
    size_t size
);

bool psp_loader_parse(
    PSP_Image *image
);

#endif
