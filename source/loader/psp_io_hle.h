#ifndef PSP_IO_HLE_H
#define PSP_IO_HLE_H
#include <stdbool.h>
#include <stdint.h>
#include "psp_iso_fs.h"
#define PSP_IO_MAX_HANDLES 16u
typedef struct { uint32_t fd,offset; PSP_ISO_Entry entry; bool active; } PSP_IO_Handle;
typedef struct { PSP_ISO_FS *fs; PSP_IO_Handle handles[PSP_IO_MAX_HANDLES]; uint32_t next_fd; } PSP_IO_Service;
void psp_io_service_init(PSP_IO_Service *service,PSP_ISO_FS *fs);
void psp_io_service_set_fs(PSP_IO_Service *service,PSP_ISO_FS *fs);
void psp_io_service_reset(PSP_IO_Service *service);
bool psp_io_hle_register(PSP_IO_Service *service);
void psp_io_hle_unregister(void);
#endif
