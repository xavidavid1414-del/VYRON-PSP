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
    float world_matrix[12];
    float view_matrix[12];
    float projection_matrix[16];
    uint32_t world_matrix_index;
    uint32_t view_matrix_index;
    uint32_t projection_matrix_index;
    float viewport_scale[3];
    float viewport_center[3];
    float viewport_offset[2];
    uint16_t *depth_buffer;
    bool depth_test_enabled;
    bool cull_enabled;
    uint32_t cull_mode;
    bool depth_write_disabled;
    uint32_t depth_function;
    uint32_t vertex_type;
    uint32_t draw_color;
    bool lighting_enabled;
    bool light0_enabled;
    uint32_t light0_type;
    float light0_direction[3];
    uint32_t light0_diffuse;
    uint32_t ambient_color;
    bool light_enabled[4];
    uint32_t light_type[4];
    float light_direction[4][3];
    uint32_t light_diffuse[4];
    uint32_t texture_address;
    uint32_t texture_level_address[8];
    uint32_t texture_level_buffer_width[8];
    uint32_t texture_level_width[8];
    uint32_t texture_level_height[8];
    uint32_t texture_address_low;
    uint32_t texture_buffer_width_register;
    uint32_t texture_buffer_width;
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_format;
    uint32_t clut_address;
    uint32_t clut_format;
    uint32_t clut_shift;
    uint32_t clut_mask;
    uint32_t clut_start;
    uint32_t clut_entries_loaded;
    uint32_t clut[1024];
    uint32_t texture_function;
    uint32_t texture_environment_color;
    bool texture_color_component;
    uint32_t texture_wrap_u;
    uint32_t texture_wrap_v;
    uint32_t texture_min_filter;
    uint32_t texture_mag_filter;
    bool texture_enabled;
    bool alpha_blend_enabled;
    bool clear_mode;
    uint32_t clear_mode_mask;
    bool alpha_test_enabled;
    bool color_test_enabled;
    uint32_t color_test_function;
    uint32_t color_test_reference;
    uint32_t color_test_mask;
    uint32_t alpha_test_function;
    uint32_t alpha_test_reference;
    uint32_t alpha_test_mask;
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
void psp_ge_destroy(PSP_GE *ge);
void psp_ge_reset(PSP_GE *ge);
bool psp_ge_execute_list(PSP_GE *ge, uint32_t list_address,
                         uint32_t max_commands);

#endif
