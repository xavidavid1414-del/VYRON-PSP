#include "psp_ge_hle.h"

#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#include "../memory/psp_memory.h"
#include <string.h>

#define SC_GE_ENQUEUE 10u
#define SC_GE_ENQUEUE_HEAD 11u
#define SC_GE_STALL 12u
#define SC_GE_LIST_SYNC 13u
#define SC_GE_DRAW_SYNC 14u
#define SC_GE_DEQUEUE 15u
#define SC_DISPLAY_SET_FRAMEBUF 16u

#define NID_GE_ENQUEUE 0xAB49E76Au
#define NID_GE_ENQUEUE_HEAD 0x1C0D95A6u
#define NID_GE_STALL 0xE0D68148u
#define NID_GE_LIST_SYNC 0x03444EB4u
#define NID_GE_DRAW_SYNC 0xB287BD61u
#define NID_GE_DEQUEUE 0x5FB86AB0u
#define NID_DISPLAY_SET_FRAMEBUF 0x289D82FEu

#define GE_LIST_ID_MAGIC 0x35000000u
#define GE_ERROR_INVALID 0x80020190u
#define GE_ERROR_UNSUPPORTED 0x80020190u

static PSP_GE_HLE *active;

void psp_ge_hle_init(PSP_GE_HLE *service, PSP_GE *ge)
{
    if (!service)
        return;
    memset(service, 0, sizeof(*service));
    service->ge = ge;
    service->initialized = ge != NULL && ge->initialized;
}

void psp_ge_hle_reset(PSP_GE_HLE *service)
{
    if (!service || !service->initialized)
        return;
    memset(service->lists, 0, sizeof(service->lists));
}

static PSP_GE_HLE_List *find_list(uint32_t id)
{
    if (!active || (id & 0xFF000000u) != GE_LIST_ID_MAGIC)
        return NULL;
    uint32_t slot = id ^ GE_LIST_ID_MAGIC;
    if (slot >= PSP_GE_HLE_MAX_LISTS)
        return NULL;
    PSP_GE_HLE_List *list = &active->lists[slot];
    return list->active && list->id == id ? list : NULL;
}

static bool enqueue_call(PSP_CPU *cpu, PSP_Memory *memory,
                         uint32_t code)
{
    if (!cpu || !memory || !active || !active->initialized ||
        (code != SC_GE_ENQUEUE && code != SC_GE_ENQUEUE_HEAD))
        return false;

    uint32_t address = cpu->gpr[4];
    uint32_t stall = cpu->gpr[5];
    if (stall != 0 || !psp_memory_is_valid(memory, address, 4)) {
        cpu->gpr[2] = GE_ERROR_UNSUPPORTED;
        return true;
    }

    if (cpu->gpr[7] != 0 &&
        !psp_memory_is_valid(memory, cpu->gpr[7], 16)) {
        cpu->gpr[2] = GE_ERROR_INVALID;
        return true;
    }

    for (uint32_t i = 0; i < PSP_GE_HLE_MAX_LISTS; i++) {
        PSP_GE_HLE_List *list = &active->lists[i];
        if (list->active)
            continue;

        uint32_t id = GE_LIST_ID_MAGIC ^ i;
        bool success = psp_ge_execute_list(active->ge, address, 100000u);
        *list = (PSP_GE_HLE_List){id, address, true, true, success};
        cpu->gpr[2] = success ? id : GE_ERROR_INVALID;
        return true;
    }

    cpu->gpr[2] = GE_ERROR_INVALID;
    return true;
}

static bool stall_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_GE_STALL)
        return false;
    /* Partial renderer has no resumable stall-address execution yet. */
    cpu->gpr[2] = GE_ERROR_UNSUPPORTED;
    return true;
}

static bool list_sync_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_GE_LIST_SYNC)
        return false;

    PSP_GE_HLE_List *list = find_list(cpu->gpr[4]);
    uint32_t mode = cpu->gpr[5];
    if (!list || mode > 1u) {
        cpu->gpr[2] = GE_ERROR_INVALID;
        return true;
    }
    cpu->gpr[2] = list->completed && list->succeeded ? 0u :
        (list->completed ? GE_ERROR_INVALID : 1u);
    return true;
}

static bool draw_sync_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_GE_DRAW_SYNC)
        return false;
    if (cpu->gpr[4] > 1u) {
        cpu->gpr[2] = GE_ERROR_INVALID;
        return true;
    }
    cpu->gpr[2] = 0;
    return true;
}


static bool display_set_framebuf_call(PSP_CPU *cpu, PSP_Memory *memory,
                                      uint32_t code)
{
    if (!cpu || !memory || !active || !active->initialized ||
        code != SC_DISPLAY_SET_FRAMEBUF)
        return false;

    uint32_t address = cpu->gpr[4];
    uint32_t stride = cpu->gpr[5];
    uint32_t format = cpu->gpr[6];
    uint32_t sync = cpu->gpr[7];
    if (format > 3u || sync > 1u ||
        (address != 0u &&
         (stride < PSP_VIDEO_WIDTH || (stride & (stride - 1u)) != 0u))) {
        cpu->gpr[2] = GE_ERROR_INVALID;
        return true;
    }
    if (address != 0u) {
        uint32_t bpp = format == 3u ? 4u : 2u;
        uint64_t span = ((uint64_t)(PSP_VIDEO_HEIGHT - 1u) * stride +
                         PSP_VIDEO_WIDTH) * bpp;
        if (span > UINT32_MAX ||
            !psp_memory_is_valid(memory, address, (uint32_t)span)) {
            cpu->gpr[2] = GE_ERROR_INVALID;
            return true;
        }
    }
    active->ge->display_address = address;
    active->ge->display_stride = stride;
    active->ge->display_format = format;
    cpu->gpr[2] = 0u;
    return true;
}

static bool dequeue_call(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || code != SC_GE_DEQUEUE)
        return false;
    PSP_GE_HLE_List *list = find_list(cpu->gpr[4]);
    if (!list) {
        cpu->gpr[2] = GE_ERROR_INVALID;
        return true;
    }
    memset(list, 0, sizeof(*list));
    cpu->gpr[2] = 0;
    return true;
}

bool psp_ge_hle_register(PSP_GE_HLE *service)
{
    if (!service || !service->initialized)
        return false;
    active = service;

    if (!psp_syscall_register(SC_GE_ENQUEUE, enqueue_call) ||
        !psp_syscall_register(SC_GE_ENQUEUE_HEAD, enqueue_call) ||
        !psp_syscall_register(SC_GE_STALL, stall_call) ||
        !psp_syscall_register(SC_GE_LIST_SYNC, list_sync_call) ||
        !psp_syscall_register(SC_GE_DRAW_SYNC, draw_sync_call) ||
        !psp_syscall_register(SC_GE_DEQUEUE, dequeue_call) ||
        !psp_syscall_register(SC_DISPLAY_SET_FRAMEBUF, display_set_framebuf_call) ||
        !psp_nid_register(NID_GE_ENQUEUE, SC_GE_ENQUEUE) ||
        !psp_nid_register(NID_GE_ENQUEUE_HEAD, SC_GE_ENQUEUE_HEAD) ||
        !psp_nid_register(NID_GE_STALL, SC_GE_STALL) ||
        !psp_nid_register(NID_GE_LIST_SYNC, SC_GE_LIST_SYNC) ||
        !psp_nid_register(NID_GE_DRAW_SYNC, SC_GE_DRAW_SYNC) ||
        !psp_nid_register(NID_GE_DEQUEUE, SC_GE_DEQUEUE) ||
        !psp_nid_register(NID_DISPLAY_SET_FRAMEBUF, SC_DISPLAY_SET_FRAMEBUF)) {
        psp_ge_hle_unregister();
        return false;
    }
    return true;
}

void psp_ge_hle_unregister(void)
{
    psp_syscall_unregister(SC_GE_ENQUEUE);
    psp_syscall_unregister(SC_GE_ENQUEUE_HEAD);
    psp_syscall_unregister(SC_GE_STALL);
    psp_syscall_unregister(SC_GE_LIST_SYNC);
    psp_syscall_unregister(SC_GE_DRAW_SYNC);
    psp_syscall_unregister(SC_GE_DEQUEUE);
    psp_syscall_unregister(SC_DISPLAY_SET_FRAMEBUF);
    psp_nid_unregister(NID_GE_ENQUEUE);
    psp_nid_unregister(NID_GE_ENQUEUE_HEAD);
    psp_nid_unregister(NID_GE_STALL);
    psp_nid_unregister(NID_GE_LIST_SYNC);
    psp_nid_unregister(NID_GE_DRAW_SYNC);
    psp_nid_unregister(NID_GE_DEQUEUE);
    psp_nid_unregister(NID_DISPLAY_SET_FRAMEBUF);
    active = NULL;
}
