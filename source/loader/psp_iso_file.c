
#include "psp_iso_file.h"

#include <string.h>
#include <ctype.h>
#include <limits.h>

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

bool psp_iso_file_open(PSP_ISO_File *iso, const char *path)
{
    if (!iso || !path)
        return false;

    memset(iso, 0, sizeof(*iso));

    FILE *file = fopen(path, "rb");
    if (!file)
        return false;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    iso->handle = file;
    iso->size = (uint64_t)length;
    iso->opened = true;

    return true;
}

void psp_iso_file_close(PSP_ISO_File *iso)
{
    if (!iso)
        return;

    if (iso->handle)
        fclose(iso->handle);

    memset(iso, 0, sizeof(*iso));
}

bool psp_iso_file_read_at(
    PSP_ISO_File *iso,
    uint64_t offset,
    void *buffer,
    size_t size)
{
    if (!iso || !iso->opened || !buffer)
        return false;

    if (offset > iso->size || size > iso->size - offset)
        return false;

    if (offset > (uint64_t)LONG_MAX)
        return false;

    if (fseek(iso->handle, (long)offset, SEEK_SET) != 0)
        return false;

    return fread(buffer, 1, size, iso->handle) == size;
}

bool psp_iso_file_read_sector(
    PSP_ISO_File *iso,
    uint32_t sector,
    uint8_t buffer[PSP_ISO_FILE_SECTOR_SIZE])
{
    uint64_t offset =
        (uint64_t)sector * PSP_ISO_FILE_SECTOR_SIZE;

    return psp_iso_file_read_at(
        iso, offset, buffer, PSP_ISO_FILE_SECTOR_SIZE);
}

static bool iso_name_equal(const char *a, const char *b)
{
    size_t i = 0;

    while (a[i] && b[i] && a[i] != ';') {
        if (toupper((unsigned char)a[i]) !=
            toupper((unsigned char)b[i]))
            return false;
        i++;
    }

    return b[i] == '\0' && (a[i] == '\0' || a[i] == ';');
}

bool psp_iso_file_find_root_entry(
    PSP_ISO_File *iso,
    const char *name,
    PSP_ISO_FileEntry *entry)
{
    if (!iso || !iso->opened || !name || !entry)
        return false;

    uint8_t pvd[PSP_ISO_FILE_SECTOR_SIZE];

    if (!psp_iso_file_read_sector(iso, 16, pvd))
        return false;

    if (pvd[0] != 1 ||
        memcmp(pvd + 1, "CD001", 5) != 0 ||
        pvd[6] != 1)
        return false;

    const uint8_t *root = pvd + 156;

    if (root[0] < 34)
        return false;

    uint32_t root_sector = read_le32(root + 2);
    uint32_t root_size = read_le32(root + 10);

    uint64_t root_offset =
        (uint64_t)root_sector * PSP_ISO_FILE_SECTOR_SIZE;

    if (root_offset > iso->size ||
        root_size > iso->size - root_offset)
        return false;

    uint8_t sector[PSP_ISO_FILE_SECTOR_SIZE];
    uint32_t position = 0;

    while (position < root_size) {
        uint32_t sector_number =
            root_sector + position / PSP_ISO_FILE_SECTOR_SIZE;

        uint32_t sector_offset =
            position % PSP_ISO_FILE_SECTOR_SIZE;

        if (!psp_iso_file_read_sector(
                iso, sector_number, sector))
            return false;

        uint8_t record_length = sector[sector_offset];

        if (record_length == 0) {
            position =
                ((position / PSP_ISO_FILE_SECTOR_SIZE) + 1)
                * PSP_ISO_FILE_SECTOR_SIZE;
            continue;
        }

        if (record_length < 34 ||
            record_length > PSP_ISO_FILE_SECTOR_SIZE - sector_offset ||
            record_length > root_size - position)
            return false;

        uint8_t name_length = sector[sector_offset + 32];

        if (name_length == 0 ||
            name_length > 127 ||
            33U + name_length > record_length) {
            position += record_length;
            continue;
        }

        char filename[128];

        memcpy(filename, sector + sector_offset + 33,
               name_length);
        filename[name_length] = '\0';

        if (iso_name_equal(filename, name)) {
            const uint8_t *record = sector + sector_offset;

            entry->extent = read_le32(record + 2);
            entry->size = read_le32(record + 10);
            entry->directory = (record[25] & 0x02) != 0;

            memcpy(entry->name, filename, name_length + 1);
            return true;
        }

        position += record_length;
    }

    return false;
}
