#include "psp_iso_extract.h"

#include <stddef.h>
#include <stdint.h>

bool psp_iso_extract_entry(
    PSP_ISO_File *iso,
    const PSP_ISO_FileEntry *entry,
    void *buffer,
    uint32_t buffer_capacity
)
{
    if (iso == NULL ||
        entry == NULL ||
        buffer == NULL ||
        !iso->opened ||
        entry->directory ||
        entry->size == 0 ||
        buffer_capacity < entry->size)
    {
        return false;
    }

    uint64_t offset =
        (uint64_t)entry->extent *
        PSP_ISO_FILE_SECTOR_SIZE;

    if (offset > UINT32_MAX ||
        offset > iso->size ||
        entry->size > iso->size - offset)
    {
        return false;
    }

    return psp_iso_file_read_at(
        iso,
        (uint32_t)offset,
        buffer,
        entry->size
    );
}
