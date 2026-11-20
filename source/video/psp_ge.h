#ifndef PSP_GE_H
#define PSP_GE_H

#include <stdbool.h>
#include <stdint.h>

#include "../memory/psp_memory.h"
#include "psp_video.h"

typedef struct {
    PSP_Memory *memory;
    PSP_Video *video;
    uint32_t base_address;
    uint32_t vertex_address;
    uint32_t index_address;
    uint32_t vertex_type;
    uint32_t draw_color;
    uint32_t texture_address;
    uint32_t texture_address_low;
    uint32_t texture_buffer_width_register;
    uint32_t texture_buffer_width;
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_format;
    uint32_t texture_wrap_u;
    uint32_t texture_wrap_v;
    bool texture_enabled;
    bool alpha_blend_enabled;
    uint32_t blend_operation;
    uint32_t blend_source_factor;
    uint32_t blend_destination_factor;
    uint32_t blend_fixed_a;
    uint32_t blend_fixed_b;
    int scissor_left;
    int scissor_top;
    int scissor_right;
    int scissor_bottom;
    uint32_t commands_executed;
    uint32_t primitives_drawn;
    bool initialized;
} PSP_GE;

bool psp_ge_init(PSP_GE *ge, PSP_Memory *memory, PSP_Video *video);
void psp_ge_reset(PSP_GE *ge);
bool psp_ge_execute_list(PSP_GE *ge, uint32_t list_address,
                         uint32_t max_commands);

#endif
