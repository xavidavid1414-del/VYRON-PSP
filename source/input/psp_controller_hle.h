#ifndef PSP_CONTROLLER_HLE_H
#define PSP_CONTROLLER_HLE_H

#include <stdint.h>

#include "psp_controller.h"
#include "../memory/psp_memory.h"
#include "../cpu/psp_cpu.h"

/* PSP SceCtrlData is 16 bytes: timestamp, buttons, Lx, Ly, reserved[6]. */
#define PSP_CTRL_DATA_SIZE 16u
#define PSP_CTRL_MAX_SAMPLES 64u

int psp_controller_hle_read_buffer_positive(
    const PSP_Controller *controller,
    PSP_Memory *memory,
    uint32_t output_address,
    uint32_t sample_count
);

int psp_controller_hle_peek_buffer_positive(
    const PSP_Controller *controller,
    PSP_Memory *memory,
    uint32_t output_address,
    uint32_t sample_count
);

#define PSP_NID_CTRL_READ_BUFFER_POSITIVE 0x1F4011E6u
#define PSP_NID_CTRL_PEEK_BUFFER_POSITIVE 0x3A622550u
#define PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE 0x100u
#define PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE 0x101u
bool psp_controller_hle_register(PSP_Controller *controller);
void psp_controller_hle_unregister(void);
#endif

