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

    if (length <= 0 ||
        (uint64_t)length > UINT32_MAX ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    iso->handle = file;
    iso->size = (uint32_t)length;
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
    uint32_t offset,
    void *buffer,
    uint32_t size)
{
    if (!iso || !iso->opened || !iso->handle || !buffer)
        return false;

    if (offset > iso->size || size > iso->size - offset)
        return false;

    if (fseek(iso->handle, (long)offset, SEEK_SET) != 0)
        return false;

    return fread(buffer, 1, size, iso->handle) == size;
}

bool psp_iso_file_read_sector(
    PSP_ISO_File *iso,
    uint32_t sector,
    void *buffer)
{
    uint64_t offset =
        (uint64_t)sector * PSP_ISO_FILE_SECTOR_SIZE;

    if (offset > UINT32_MAX)
        return false;

    return psp_iso_file_read_at(
        iso,
        (uint32_t)offset,
        buffer,
        PSP_ISO_FILE_SECTOR_SIZE
    );
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

    return b[i] == '\0' &&
           (a[i] == '\0' || a[i] == ';');
}

static bool find_directory_entry(
    PSP_ISO_File *iso,
    uint32_t extent,
    uint32_t directory_size,
    const char *name,
    PSP_ISO_FileEntry *entry)
{
    uint64_t directory_offset =
        (uint64_t)extent * PSP_ISO_FILE_SECTOR_SIZE;

    if (directory_offset > iso->size ||
        directory_size > iso->size - directory_offset)
        return false;

    uint8_t sector[PSP_ISO_FILE_SECTOR_SIZE];
    uint32_t position = 0;

    while (position < directory_size) {
        uint32_t sector_number =
            extent + position / PSP_ISO_FILE_SECTOR_SIZE;

        uint32_t sector_offset =
            position % PSP_ISO_FILE_SECTOR_SIZE;

        if (!psp_iso_file_read_sector(
                iso, sector_number, sector))
            return false;

        uint32_t remaining =
            directory_size - position;

        if (sector_offset >= PSP_ISO_FILE_SECTOR_SIZE)
            return false;

        uint8_t record_length = sector[sector_offset];

        if (record_length == 0) {
            uint32_t advance =
                PSP_ISO_FILE_SECTOR_SIZE - sector_offset;

            if (advance > remaining)
                break;

            position += advance;
            continue;
        }

        if (record_length < 34 ||
            record_length >
                PSP_ISO_FILE_SECTOR_SIZE - sector_offset ||
            record_length > remaining)
            return false;

        uint8_t name_length = sector[sector_offset + 32];

        if (name_length > 127 ||
            33U + name_length > record_length) {
            position += record_length;
            continue;
        }

        if (name_length > 0) {
            char filename[128];

            memcpy(
                filename,
                sector + sector_offset + 33,
                name_length
            );

            filename[name_length] = '\0';

            if (iso_name_equal(filename, name)) {
                const uint8_t *record =
                    sector + sector_offset;

                entry->extent = read_le32(record + 2);
                entry->size = read_le32(record + 10);
                entry->directory =
                    (record[25] & 0x02) != 0;

                memcpy(
                    entry->name,
                    filename,
                    name_length + 1
                );

                uint64_t file_offset =
                    (uint64_t)entry->extent *
                    PSP_ISO_FILE_SECTOR_SIZE;

                if (file_offset > iso->size ||
                    entry->size > iso->size - file_offset)
                    return false;

                return true;
            }
        }

        position += record_length;
    }

    return false;
}

static bool read_root_directory(
    PSP_ISO_File *iso,
    PSP_ISO_FileEntry *root)
{
    uint8_t pvd[PSP_ISO_FILE_SECTOR_SIZE];

    if (!psp_iso_file_read_sector(iso, 16, pvd))
        return false;

    if (pvd[0] != 1 ||
        memcmp(pvd + 1, "CD001", 5) != 0 ||
        pvd[6] != 1)
        return false;

    const uint8_t *record = pvd + 156;

    if (record[0] < 34 || !(record[25] & 0x02))
        return false;

    root->extent = read_le32(record + 2);
    root->size = read_le32(record + 10);
    root->directory = true;

    strcpy(root->name, "/");

    uint64_t offset =
        (uint64_t)root->extent * PSP_ISO_FILE_SECTOR_SIZE;

    return offset <= iso->size &&
           root->size <= iso->size - offset;
}

bool psp_iso_file_find_root_entry(
    PSP_ISO_File *iso,
    const char *name,
    PSP_ISO_FileEntry *entry)
{
    if (!iso || !iso->opened || !name || !entry)
        return false;

    PSP_ISO_FileEntry root;

    if (!read_root_directory(iso, &root))
        return false;

    return find_directory_entry(
        iso,
        root.extent,
        root.size,
        name,
        entry
    );
}

bool psp_iso_file_find_path(
    PSP_ISO_File *iso,
    const char *path,
    PSP_ISO_FileEntry *entry)
{
    if (!iso || !iso->opened || !path || !entry)
        return false;

    PSP_ISO_FileEntry current;

    if (!read_root_directory(iso, &current))
        return false;

    while (*path == '/')
        path++;

    if (*path == '\0') {
        *entry = current;
        return true;
    }

    char component[128];

    while (*path) {
        size_t length = 0;

        while (path[length] && path[length] != '/') {
            if (length >= sizeof(component) - 1)
                return false;

            component[length] = path[length];
            length++;
        }

        component[length] = '\0';

        if (length == 0)
            return false;

        if (strcmp(component, ".") == 0)
            return false;

        if (strcmp(component, "..") == 0)
            return false;

        if (!current.directory)
            return false;

        PSP_ISO_FileEntry next;

        if (!find_directory_entry(
                iso,
                current.extent,
                current.size,
                component,
                &next))
            return false;

        current = next;
        path += length;

        while (*path == '/')
            path++;

        if (*path && !current.directory)
            return false;
    }

    *entry = current;
    return true;
} 
