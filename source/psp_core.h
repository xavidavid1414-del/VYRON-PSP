#ifndef PSP_CORE_H
#define PSP_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "cpu/psp_cpu.h"
#include "memory/psp_memory.h"
#include "memory/psp_memory_blocks.h"
#include "loader/psp_io_hle.h"
#include "loader/psp_iso.h"
#include "loader/psp_iso_fs.h"
#include "loader/psp_loader.h"
#include "video/psp_video.h"
#include "video/psp_ge.h"
#include "video/psp_ge_hle.h"
#include "input/psp_controller.h"

typedef struct {
    PSP_CPU cpu;
    PSP_Memory memory;
    PSP_MemoryBlocks memory_blocks;
    PSP_IO_Service io_service;
    PSP_Video video;
    PSP_GE ge;
    PSP_GE_HLE ge_hle;
    PSP_Controller controller;

    PSP_ISO_Info iso_info;
    PSP_ISO_FS iso_fs;

    const uint8_t *iso_data;
    size_t iso_size;

    PSP_Image image;

    bool iso_mounted;
    bool image_loaded;
    bool initialized;
} PSP_Core;

bool psp_core_init(PSP_Core *core);
void psp_core_reset(PSP_Core *core);
void psp_core_destroy(PSP_Core *core);
bool psp_core_run(PSP_Core *core, uint32_t cycles);
bool psp_core_execute_ge_list(PSP_Core *core, uint32_t list_address,
                              uint32_t max_commands);

bool psp_core_mount_iso(
    PSP_Core *core,
    const uint8_t *data,
    size_t size
);

bool psp_core_find_iso_entry(
    PSP_Core *core,
    const char *name,
    PSP_ISO_Entry *entry
);

void psp_core_unmount_iso(PSP_Core *core);

bool psp_core_load_executable(
    PSP_Core *core,
    const char *path
);

void psp_core_unload_executable(PSP_Core *core);
bool psp_core_load_executable_data(
    PSP_Core *core,
    const uint8_t *data,
    size_t size
);
#endif
