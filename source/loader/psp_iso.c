#include "psp_iso.h"

#include <string.h>

#define ISO_SECTOR_SIZE 2048
#define ISO_PVD_OFFSET (16 * ISO_SECTOR_SIZE)

bool psp_iso_identify(
    const uint8_t *data,
    size_t size,
    PSP_ISO_Info *info)
{
    if (data == NULL || info == NULL)
        return false;

    memset(info, 0, sizeof(*info));

    if (size < ISO_PVD_OFFSET + ISO_SECTOR_SIZE)
        return false;

    const uint8_t *pvd = data + ISO_PVD_OFFSET;

    if (pvd[0] != 1)
        return false;

    if (memcmp(pvd + 1, "CD001", 5) != 0)
        return false;

    if (pvd[6] != 1)
        return false;

    info->valid = true;
    info->sector_size = ISO_SECTOR_SIZE;
    info->image_size = size;

    return true;
}
