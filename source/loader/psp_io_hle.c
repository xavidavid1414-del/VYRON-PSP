#include "psp_io_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#include "../memory/psp_memory.h"
#include <string.h>

#define SC_OPEN 6u
#define SC_CLOSE 7u
#define SC_READ 8u
#define SC_LSEEK 9u
#define NID_OPEN 0x109F50BCu
#define NID_CLOSE 0x810C4BC3u
#define NID_READ 0x6A638D83u
#define NID_LSEEK 0x27EB27B8u

static PSP_IO_Service *active;

void psp_io_service_init(PSP_IO_Service *service, PSP_ISO_FS *fs)
{
    if (!service)
        return;
    memset(service, 0, sizeof(*service));
    service->fs = fs;
    service->next_fd = 3;
}

void psp_io_service_set_fs(PSP_IO_Service *service, PSP_ISO_FS *fs)
{
    if (!service)
        return;
    service->fs = fs;
    psp_io_service_reset(service);
}

void psp_io_service_reset(PSP_IO_Service *service)
{
    if (!service)
        return;
    memset(service->handles, 0, sizeof(service->handles));
    service->next_fd = 3;
}

static bool read_guest_path(
    PSP_Memory *memory, uint32_t address, char *out, size_t capacity)
{
    if (!memory || !out || capacity == 0)
        return false;

    for (size_t i = 0; i + 1u < capacity; i++) {
        uint64_t current = (uint64_t)address + i;
        if (current > UINT32_MAX ||
            !psp_memory_is_valid(memory, (uint32_t)current, 1))
            return false;

        char value = (char)psp_memory_read8(memory, (uint32_t)current);
        out[i] = value;
        if (value == '\0')
            return i > 0;
    }
    out[capacity - 1u] = '\0';
    return false;
}

static bool open_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    if (!cpu || !memory || !active || code != SC_OPEN)
        return false;

    char raw_path[256];
    if (!read_guest_path(memory, cpu->gpr[4], raw_path, sizeof(raw_path)) ||
        !active->fs) {
        cpu->gpr[2] = 0x80010002u;
        return true;
    }

    const char *path = raw_path;
    if (!strncmp(path, "disc0:/", 7) || !strncmp(path, "umd0:/", 6)) {
        path = strchr(path, ':') + 1;
        while (*path == '/')
            path++;
    } else {
        cpu->gpr[2] = 0x80010002u;
        return true;
    }

    /* ISO images are mounted read-only; only PSP O_RDONLY is accepted. */
    if ((cpu->gpr[5] & 3u) != 1u) {
        cpu->gpr[2] = 0x80010002u;
        return true;
    }

    PSP_ISO_Entry entry;
    if (!psp_iso_fs_find_path(active->fs, path, &entry) || entry.directory) {
        cpu->gpr[2] = 0x80010013u;
        return true;
    }

    for (unsigned i = 0; i < PSP_IO_MAX_HANDLES; i++) {
        if (!active->handles[i].active) {
            uint32_t fd = active->next_fd++;
            if (fd < 3u) {
                active->next_fd = 4u;
                fd = 3u;
            }
            active->handles[i] = (PSP_IO_Handle){fd, 0, entry, true};
            cpu->gpr[2] = fd;
            return true;
        }
    }

    cpu->gpr[2] = 0x80020190u;
    return true;
}

static PSP_IO_Handle *get_handle(uint32_t fd)
{
    if (!active)
        return NULL;
    for (unsigned i = 0; i < PSP_IO_MAX_HANDLES; i++) {
        if (active->handles[i].active && active->handles[i].fd == fd)
            return &active->handles[i];
    }
    return NULL;
}

static bool close_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_CLOSE)
        return false;

    PSP_IO_Handle *handle = get_handle(cpu->gpr[4]);
    if (!handle) {
        cpu->gpr[2] = 0x80010009u;
        return true;
    }

    memset(handle, 0, sizeof(*handle));
    cpu->gpr[2] = 0;
    return true;
}

static bool read_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    if (!cpu || !memory || code != SC_READ)
        return false;

    PSP_IO_Handle *handle = get_handle(cpu->gpr[4]);
    uint32_t address = cpu->gpr[5];
    uint32_t length = cpu->gpr[6];

    if (!handle || !psp_memory_is_valid(memory, address, length)) {
        cpu->gpr[2] = 0x80010009u;
        return true;
    }

    uint32_t remaining = handle->entry.size - handle->offset;
    if (length > remaining)
        length = remaining;
    if (length == 0) {
        cpu->gpr[2] = 0;
        return true;
    }

    uint8_t buffer[512];
    uint32_t completed = 0;
    while (completed < length) {
        uint32_t chunk = length - completed;
        if (chunk > sizeof(buffer))
            chunk = sizeof(buffer);

        size_t got = psp_iso_fs_read(
            active->fs, &handle->entry, handle->offset, buffer, chunk);
        if (got == 0)
            break;

        for (size_t i = 0; i < got; i++)
            psp_memory_write8(memory, address + completed + (uint32_t)i,
                              buffer[i]);

        completed += (uint32_t)got;
        handle->offset += (uint32_t)got;
        if (got < chunk)
            break;
    }

    cpu->gpr[2] = completed;
    return true;
}

static bool seek_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_LSEEK)
        return false;

    PSP_IO_Handle *handle = get_handle(cpu->gpr[4]);
    if (!handle) {
        cpu->gpr[2] = 0x80010009u;
        return true;
    }

    int32_t delta = (int32_t)cpu->gpr[5];
    uint32_t whence = cpu->gpr[6];
    int64_t base;
    if (whence == 0)
        base = 0;
    else if (whence == 1)
        base = (int64_t)handle->offset;
    else if (whence == 2)
        base = (int64_t)handle->entry.size;
    else
        base = -1;

    int64_t next = base + delta;
    if (base < 0 || next < 0 || next > handle->entry.size) {
        cpu->gpr[2] = 0x80010016u;
        return true;
    }

    handle->offset = (uint32_t)next;
    cpu->gpr[2] = handle->offset;
    return true;
}

bool psp_io_hle_register(PSP_IO_Service *service)
{
    if (!service)
        return false;
    active = service;

    if (!psp_syscall_register(SC_OPEN, open_call) ||
        !psp_syscall_register(SC_CLOSE, close_call) ||
        !psp_syscall_register(SC_READ, read_call) ||
        !psp_syscall_register(SC_LSEEK, seek_call) ||
        !psp_nid_register(NID_OPEN, SC_OPEN) ||
        !psp_nid_register(NID_CLOSE, SC_CLOSE) ||
        !psp_nid_register(NID_READ, SC_READ) ||
        !psp_nid_register(NID_LSEEK, SC_LSEEK)) {
        psp_io_hle_unregister();
        return false;
    }
    return true;
}

void psp_io_hle_unregister(void)
{
    psp_syscall_unregister(SC_OPEN);
    psp_syscall_unregister(SC_CLOSE);
    psp_syscall_unregister(SC_READ);
    psp_syscall_unregister(SC_LSEEK);
    psp_nid_unregister(NID_OPEN);
    psp_nid_unregister(NID_CLOSE);
    psp_nid_unregister(NID_READ);
    psp_nid_unregister(NID_LSEEK);
    active = NULL;
}
