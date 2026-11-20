#include "psp_iso_fs.h"
#include <string.h>
#include <ctype.h>

#define ISO_SECTOR_SIZE 2048u
#define ISO_PVD_OFFSET (16u * ISO_SECTOR_SIZE)

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool name_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return false;
        a++;
        b++;
    }
    if (*a == ';') {
        a++;
        while (isdigit((unsigned char)*a))
            a++;
    }
    return *a == '\0' && *b == '\0';
}

bool psp_iso_fs_init(PSP_ISO_FS *fs, const uint8_t *data, size_t size)
{
    if (!fs || !data || size < ISO_PVD_OFFSET + ISO_SECTOR_SIZE)
        return false;

    const uint8_t *pvd = data + ISO_PVD_OFFSET;
    if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0 || pvd[6] != 1)
        return false;

    const uint8_t *root = pvd + 156;
    if (root[0] < 34)
        return false;

    uint32_t extent = read_le32(root + 2);
    uint32_t length = read_le32(root + 10);
    uint64_t offset = (uint64_t)extent * ISO_SECTOR_SIZE;
    if (offset > size || length > size - offset)
        return false;

    fs->data = data;
    fs->size = size;
    fs->root_extent = extent;
    fs->root_size = length;
    return true;
}

static bool find_in_directory(
    PSP_ISO_FS *fs, uint32_t extent, uint32_t length,
    const char *name, PSP_ISO_Entry *out)
{
    uint64_t base = (uint64_t)extent * ISO_SECTOR_SIZE;
    if (base > fs->size || length > fs->size - base)
        return false;

    size_t position = 0;
    while (position < length) {
        const uint8_t *record = fs->data + base + position;
        uint8_t record_length = record[0];

        if (record_length == 0) {
            position = ((position / ISO_SECTOR_SIZE) + 1u) * ISO_SECTOR_SIZE;
            continue;
        }
        if (record_length < 34 || record_length > length - position)
            return false;

        uint8_t name_length = record[32];
        if (33u + name_length > record_length)
            return false;

        if (name_length > 0 && name_length < sizeof(out->name)) {
            char filename[128];
            memcpy(filename, record + 33, name_length);
            filename[name_length] = '\0';

            if (name_equal(filename, name)) {
                memset(out, 0, sizeof(*out));
                out->extent = read_le32(record + 2);
                out->size = read_le32(record + 10);
                out->directory = (record[25] & 0x02u) != 0;
                memcpy(out->name, filename, name_length + 1u);
                return true;
            }
        }
        position += record_length;
    }
    return false;
}

bool psp_iso_fs_find(
    PSP_ISO_FS *fs, const char *name, PSP_ISO_Entry *entry)
{
    return fs && fs->data && name && entry &&
        find_in_directory(fs, fs->root_extent, fs->root_size, name, entry);
}

bool psp_iso_fs_find_path(
    PSP_ISO_FS *fs, const char *path, PSP_ISO_Entry *entry)
{
    if (!fs || !fs->data || !path || !entry)
        return false;

    while (*path == '/')
        path++;

    uint32_t extent = fs->root_extent;
    uint32_t length = fs->root_size;
    PSP_ISO_Entry current = {0};

    if (*path == '\0') {
        memset(entry, 0, sizeof(*entry));
        strcpy(entry->name, "/");
        entry->extent = extent;
        entry->size = length;
        entry->directory = true;
        return true;
    }

    char component[128];
    while (*path) {
        size_t component_length = 0;
        while (path[component_length] && path[component_length] != '/') {
            if (component_length >= sizeof(component) - 1u)
                return false;
            component[component_length] = path[component_length];
            component_length++;
        }
        component[component_length] = '\0';

        if (!find_in_directory(
                fs, extent, length, component, &current))
            return false;

        path += component_length;
        while (*path == '/')
            path++;

        if (*path) {
            if (!current.directory)
                return false;
            extent = current.extent;
            length = current.size;
        }
    }

    *entry = current;
    return true;
}

size_t psp_iso_fs_read(
    PSP_ISO_FS *fs, const PSP_ISO_Entry *entry,
    uint32_t offset, uint8_t *out, size_t length)
{
    if (!fs || !fs->data || !entry || !out ||
        entry->directory || offset >= entry->size)
        return 0;

    size_t amount = length;
    if (amount > entry->size - offset)
        amount = entry->size - offset;

    uint64_t base =
        (uint64_t)entry->extent * ISO_SECTOR_SIZE + offset;
    if (base > fs->size || amount > fs->size - base)
        return 0;

    memcpy(out, fs->data + base, amount);
    return amount;
}
