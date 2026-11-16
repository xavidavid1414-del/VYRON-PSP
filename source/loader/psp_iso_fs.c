#include "psp_iso_fs.h"

#include <string.h>
#include <ctype.h>

#define ISO_SECTOR_SIZE 2048
#define ISO_PVD_SECTOR 16
#define ISO_PVD_OFFSET (ISO_PVD_SECTOR * ISO_SECTOR_SIZE)

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static bool name_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) !=
            toupper((unsigned char)*b)) {
            return false;
        }

        a++;
        b++;
    }

    if (*a == ';') {
        a++;
        while (isdigit((unsigned char)*a)) {
            a++;
        }
    }

    return *a == '\0' && *b == '\0';
}

bool psp_iso_fs_init(
    PSP_ISO_FS *fs,
    const uint8_t *data,
    size_t size
)
{
    if (!fs || !data || size < ISO_PVD_OFFSET + ISO_SECTOR_SIZE) {
        return false;
    }

    const uint8_t *pvd = data + ISO_PVD_OFFSET;

    if (pvd[0] != 1 ||
        memcmp(pvd + 1, "CD001", 5) != 0 ||
        pvd[6] != 1) {
        return false;
    }

    const uint8_t *root = pvd + 156;

    if (root[0] < 34) {
        return false;
    }

    uint32_t extent = read_le32(root + 2);
    uint32_t length = read_le32(root + 10);

    uint64_t offset = (uint64_t)extent * ISO_SECTOR_SIZE;

    if (offset > size || length > size - offset) {
        return false;
    }

    fs->data = data;
    fs->size = size;
    fs->root_extent = extent;
    fs->root_size = length;

    return true;
}

bool psp_iso_fs_find(
    PSP_ISO_FS *fs,
    const char *name,
    PSP_ISO_Entry *entry
)
{
    if (!fs || !fs->data || !name || !entry) {
        return false;
    }

    uint64_t directory_offset =
        (uint64_t)fs->root_extent * ISO_SECTOR_SIZE;

    if (directory_offset > fs->size ||
        fs->root_size > fs->size - directory_offset) {
        return false;
    }

    size_t position = 0;

    while (position < fs->root_size) {
        const uint8_t *record =
            fs->data + directory_offset + position;

        uint8_t record_length = record[0];

        if (record_length == 0) {
            position =
                ((position / ISO_SECTOR_SIZE) + 1) *
                ISO_SECTOR_SIZE;
            continue;
        }

        if (record_length < 34 ||
            record_length > fs->root_size - position) {
            return false;
        }

        uint8_t name_length = record[32];

        if (33U + name_length > record_length) {
            return false;
        }

        if (name_length > 0 && name_length < 128) {
            char filename[128];

            memcpy(filename, record + 33, name_length);
            filename[name_length] = '\0';

            if (name_equal(filename, name)) {
                entry->extent = read_le32(record + 2);
                entry->size = read_le32(record + 10);
                entry->directory = (record[25] & 0x02) != 0;

                strncpy(entry->name, filename,
                        sizeof(entry->name) - 1);
                entry->name[sizeof(entry->name) - 1] = '\0';

                return true;
            }
        }

        position += record_length;
    }

    return false;
}
