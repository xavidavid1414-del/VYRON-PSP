#ifndef PSP_ISO_EXTRACT_H
#define PSP_ISO_EXTRACT_H

#include <stdbool.h>
#include <stdint.h>

#include "psp_iso_file.h"

bool psp_iso_extract_entry(
    PSP_ISO_File *iso,
    const PSP_ISO_FileEntry *entry,
    void *buffer,
    uint32_t buffer_capacity
);

#endif
