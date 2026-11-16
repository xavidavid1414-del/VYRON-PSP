#ifndef PSP_LOADER_H
#define PSP_LOADER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *data;
    size_t size;
    bool loaded;
} PSP_Image;

bool psp_loader_load(
    const char *path,
    PSP_Image *image
);

void psp_loader_unload(PSP_Image *image);

#endif
