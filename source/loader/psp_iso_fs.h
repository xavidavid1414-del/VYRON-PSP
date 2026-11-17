#ifndef PSP_ISO_FS_H
#define PSP_ISO_FS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint8_t *data;
    size_t size;

    uint32_t root_extent;
    uint32_t root_size;
} PSP_ISO_FS;

typedef struct {
    char name[128];
    uint32_t extent;
    uint32_t size;
    bool directory;
} PSP_ISO_Entry;

bool psp_iso_fs_init(
    PSP_ISO_FS *fs,
    const uint8_t *data,
    size_t size
);

bool psp_iso_fs_find(
    PSP_ISO_FS *fs,
    const char *name,
    PSP_ISO_Entry *entry
);

#endif
