
#ifndef PSP_ISO_FILE_H
#define PSP_ISO_FILE_H
#define PSP_ISO_FILE_SECTOR_SIZE 2048
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *handle;
    uint32_t size;
    bool opened;
} PSP_ISO_File;

typedef struct {
    char name[128];
    uint32_t extent;
    uint32_t size;
    bool directory;
} PSP_ISO_FileEntry;

bool psp_iso_file_open(PSP_ISO_File *iso, const char *path);
void psp_iso_file_close(PSP_ISO_File *iso);

bool psp_iso_file_read_at(
    PSP_ISO_File *iso,
    uint32_t offset,
    void *buffer,
    uint32_t size
);

bool psp_iso_file_read_sector(
    PSP_ISO_File *iso,
    uint32_t sector,
    void *buffer
);

bool psp_iso_file_find_root_entry(
    PSP_ISO_File *iso,
    const char *name,
    PSP_ISO_FileEntry *entry
);

bool psp_iso_file_find_path(
    PSP_ISO_File *iso,
    const char *path,
    PSP_ISO_FileEntry *entry
);

#endif
