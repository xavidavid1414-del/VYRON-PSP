#ifndef PSP_ISO_H
#define PSP_ISO_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    bool valid;
    uint32_t sector_size;
    size_t image_size;
} PSP_ISO_Info;

bool psp_iso_identify(
    const uint8_t *data,
    size_t size,
    PSP_ISO_Info *info
);

#endif
