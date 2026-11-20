#include "psp_ge.h"

#include <math.h>
#include <string.h>

#define GE_CMD_VADDR   0x01u
#define GE_CMD_IADDR   0x02u
#define GE_CMD_PRIM    0x04u
#define GE_CMD_JUMP    0x08u
#define GE_CMD_CALL    0x0Au
#define GE_CMD_RET     0x0Bu
#define GE_CMD_END     0x0Cu
#define GE_CMD_BASE    0x10u
#define GE_CMD_VTYPE   0x12u
#define GE_CMD_OFFSET  0x13u
#define GE_CMD_SCISSOR1 0xD4u
#define GE_CMD_SCISSOR2 0xD5u
#define GE_CMD_TEXTUREMAPENABLE 0x1Eu
#define GE_CMD_ALPHABLENDENABLE 0x21u
#define GE_CMD_ALPHATESTENABLE 0x22u
#define GE_CMD_ALPHATEST 0xDBu
#define GE_CMD_BLENDMODE 0xDFu
#define GE_CMD_BLENDFIXEDA 0xE0u
#define GE_CMD_BLENDFIXEDB 0xE1u
#define GE_CMD_TEXADDR0 0xA0u
#define GE_CMD_TEXBUFWIDTH0 0xA8u
#define GE_CMD_TEXSIZE0 0xB8u
#define GE_CMD_TEXFORMAT 0xC3u
#define GE_CMD_TEXWRAP 0xC7u

typedef struct {
    float x;
    float y;
    float u;
    float v;
    uint32_t color;
} GE_Vertex;

static uint32_t ge_resolve_address(const PSP_GE *ge, uint32_t address)
{
    return (ge->base_address & 0xFF000000u) | (address & 0x00FFFFFFu);
}

static bool ge_read_vertex(PSP_GE *ge, uint32_t index, GE_Vertex *vertex)
{
    uint32_t type = ge->vertex_type;
    uint32_t color_type = (type >> 2) & 7u;
    uint32_t position_type = (type >> 7) & 3u;
    uint32_t texture_type = type & 3u;
    uint32_t normal_type = (type >> 5) & 3u;
    uint32_t weight_type = (type >> 9) & 3u;
    uint32_t weight_count = ((type >> 14) & 7u) + 1u;
    uint32_t morph_count = ((type >> 18) & 7u) + 1u;
    uint32_t transform_2d = (type >> 23) & 1u;

    if (normal_type != 0 || weight_type != 0 ||
        weight_count != 1 || morph_count != 1 || !transform_2d ||
        (position_type != 2 && position_type != 3) ||
        color_type > 4 || (ge->texture_enabled && texture_type == 0))
        return false;

    uint32_t stride = texture_type == 1 ? 2u :
        (texture_type == 2 ? 4u : (texture_type == 3 ? 8u : 0u));
    if (color_type == 1 || color_type == 2 || color_type == 3)
        stride += 2;
    else if (color_type == 4)
        stride += 4;
    stride += position_type == 2 ? 4u : 8u;

    uint64_t address64 = (uint64_t)ge->vertex_address +
        (uint64_t)index * stride;
    if (address64 > UINT32_MAX ||
        !psp_memory_is_valid(ge->memory, (uint32_t)address64, stride))
        return false;

    uint32_t address = (uint32_t)address64;
    vertex->u = 0.0f;
    vertex->v = 0.0f;
    if (texture_type == 1) {
        vertex->u = (float)psp_memory_read8(ge->memory, address);
        vertex->v = (float)psp_memory_read8(ge->memory, address + 1);
        address += 2;
    } else if (texture_type == 2) {
        uint32_t uv = psp_memory_read8(ge->memory, address) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 1) << 8);
        uint32_t vv = psp_memory_read8(ge->memory, address + 2) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 3) << 8);
        vertex->u = (float)uv;
        vertex->v = (float)vv;
        address += 4;
    } else if (texture_type == 3) {
        uint32_t ub = psp_memory_read32(ge->memory, address);
        uint32_t vb = psp_memory_read32(ge->memory, address + 4);
        memcpy(&vertex->u, &ub, sizeof(ub));
        memcpy(&vertex->v, &vb, sizeof(vb));
        address += 8;
    }

    vertex->color = ge->draw_color;
    if (color_type == 1 || color_type == 2 || color_type == 3) {
        uint32_t packed = psp_memory_read8(ge->memory, address) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 1) << 8);
        address += 2;
        if (color_type == 1) {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 11) & 31u) * 255u / 31u,
                ((packed >> 5) & 63u) * 255u / 63u,
                (packed & 31u) * 255u / 31u, 255);
        } else if (color_type == 2) {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 10) & 31u) * 255u / 31u,
                ((packed >> 5) & 31u) * 255u / 31u,
                (packed & 31u) * 255u / 31u,
                (packed & 0x8000u) ? 255 : 0);
        } else {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 8) & 15u) * 17u,
                ((packed >> 4) & 15u) * 17u,
                (packed & 15u) * 17u,
                ((packed >> 12) & 15u) * 17u);
        }
    } else if (color_type == 4) {
        uint32_t packed = psp_memory_read32(ge->memory, address);
        vertex->color = packed;
        address += 4;
    }

    if (position_type == 2) {
        int16_t x = (int16_t)(psp_memory_read8(ge->memory, address) |
            ((uint16_t)psp_memory_read8(ge->memory, address + 1) << 8));
        int16_t y = (int16_t)(psp_memory_read8(ge->memory, address + 2) |
            ((uint16_t)psp_memory_read8(ge->memory, address + 3) << 8));
        vertex->x = (float)x;
        vertex->y = (float)y;
    } else {
        uint32_t xb = psp_memory_read32(ge->memory, address);
        uint32_t yb = psp_memory_read32(ge->memory, address + 4);
        memcpy(&vertex->x, &xb, sizeof(xb));
        memcpy(&vertex->y, &yb, sizeof(yb));
    }

    if (!isfinite(vertex->x) || !isfinite(vertex->y) ||
        !isfinite(vertex->u) || !isfinite(vertex->v) ||
        fabsf(vertex->x) > 32768.0f || fabsf(vertex->y) > 32768.0f ||
        fabsf(vertex->u) > 65536.0f || fabsf(vertex->v) > 65536.0f)
        return false;
    return true;
}


static bool ge_read_primitive_vertex(PSP_GE *ge, uint32_t index,
                                     GE_Vertex *vertex)
{
    uint32_t index_type = (ge->vertex_type >> 11) & 3u;
    uint32_t vertex_index = index;
    if (index_type != 0u) {
        uint32_t index_size = index_type == 1u ? 1u :
            (index_type == 2u ? 2u : 4u);
        uint64_t address64 = (uint64_t)ge->index_address +
            (uint64_t)index * index_size;
        if (address64 > UINT32_MAX ||
            !psp_memory_is_valid(ge->memory, (uint32_t)address64,
                                 index_size))
            return false;
        uint32_t address = (uint32_t)address64;
        if (index_type == 1u) {
            vertex_index = psp_memory_read8(ge->memory, address);
        } else if (index_type == 2u) {
            vertex_index = psp_memory_read8(ge->memory, address) |
                ((uint32_t)psp_memory_read8(ge->memory, address + 1u) << 8);
        } else {
            vertex_index = psp_memory_read32(ge->memory, address);
        }
    }
    return ge_read_vertex(ge, vertex_index, vertex);
}

static uint32_t ge_blend_factor(uint32_t factor, unsigned int shift,
                                 uint32_t source, uint32_t destination,
                                 uint32_t fixed)
{
    uint32_t dst = (destination >> shift) & 0xFFu;
    uint32_t sa = (source >> 24) & 0xFFu;
    uint32_t da = (destination >> 24) & 0xFFu;
    switch (factor) {
        case 0: return dst;
        case 1: return 255u - dst;
        case 2: return sa;
        case 3: return 255u - sa;
        case 4: return da;
        case 5: return 255u - da;
        case 6: return sa > 127u ? 255u : sa * 2u;
        case 7: return 255u - (sa > 127u ? 255u : sa * 2u);
        case 8: return da > 127u ? 255u : da * 2u;
        case 9: return 255u - (da > 127u ? 255u : da * 2u);
        case 10: return (fixed >> shift) & 0xFFu;
        default: return 255u;
    }
}

static uint32_t ge_blend_color(const PSP_GE *ge, uint32_t source,
                               uint32_t destination)
{
    uint32_t result = 0;
    for (unsigned int shift = 0; shift <= 24; shift += 8) {
        uint32_t sf = ge_blend_factor(ge->blend_source_factor, shift,
            source, destination, ge->blend_fixed_a);
        uint32_t df = ge_blend_factor(ge->blend_destination_factor, shift,
            source, destination, ge->blend_fixed_b);
        int src = (int)(((source >> shift) & 0xFFu) * sf / 255u);
        int dst = (int)(((destination >> shift) & 0xFFu) * df / 255u);
        int value;
        switch (ge->blend_operation) {
            case 0: value = src + dst; break;
            case 1: value = src - dst; break;
            case 2: value = dst - src; break;
            case 3: value = src < dst ? src : dst; break;
            case 4: value = src > dst ? src : dst; break;
            case 5: value = src > dst ? src - dst : dst - src; break;
            default: value = src; break;
        }
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        result |= (uint32_t)value << shift;
    }
    return result;
}

static bool ge_compare_value(uint32_t function, uint32_t source,
                              uint32_t reference)
{
    switch (function) {
        case 0: return false;
        case 1: return true;
        case 2: return source == reference;
        case 3: return source != reference;
        case 4: return source < reference;
        case 5: return source <= reference;
        case 6: return source > reference;
        case 7: return source >= reference;
        default: return false;
    }
}

static void ge_put_pixel(PSP_GE *ge, int x, int y, uint32_t color)
{
    if (x < ge->scissor_left || x > ge->scissor_right ||
        y < ge->scissor_top || y > ge->scissor_bottom)
        return;
    if (ge->alpha_test_enabled) {
        uint32_t alpha = (color >> 24) & ge->alpha_test_mask;
        uint32_t reference = ge->alpha_test_reference & ge->alpha_test_mask;
        if (!ge_compare_value(ge->alpha_test_function, alpha, reference))
            return;
    }
    if (ge->alpha_blend_enabled) {
        uint32_t destination = psp_video_get_pixel(ge->video, x, y);
        color = ge_blend_color(ge, color, destination);
    }
    psp_video_put_pixel(ge->video, x, y, color);
}

static uint32_t ge_interpolate_color(uint32_t a, uint32_t b,
                                     uint32_t c, float wa,
                                     float wb, float wc);
static uint32_t ge_shade_fragment(PSP_GE *ge, uint32_t color,
                                  float u, float v);

static void ge_draw_line(PSP_GE *ge, const GE_Vertex *a,
                         const GE_Vertex *b)
{
    int x0 = (int)lroundf(a->x), y0 = (int)lroundf(a->y);
    int x1 = (int)lroundf(b->x), y1 = (int)lroundf(b->y);
    int dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    int total_steps = dx > -dy ? dx : -dy;
    int step = 0;
    for (;;) {
        float t = total_steps ? (float)step / (float)total_steps : 0.0f;
        uint32_t color = ge_interpolate_color(
            a->color, b->color, b->color, 1.0f - t, t, 0.0f);
        float u = a->u + (b->u - a->u) * t;
        float v = a->v + (b->v - a->v) * t;
        ge_put_pixel(ge, x0, y0, ge_shade_fragment(ge, color, u, v));
        if (x0 == x1 && y0 == y1) break;
        int twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
        step++;
    }
}

static float ge_edge(float ax, float ay, float bx, float by,
                     float px, float py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static uint32_t ge_interpolate_color(uint32_t a, uint32_t b, uint32_t c,
                                     float wa, float wb, float wc)
{
    uint32_t result = 0;
    for (unsigned int shift = 0; shift <= 24; shift += 8) {
        float channel = (float)((a >> shift) & 0xFFu) * wa +
            (float)((b >> shift) & 0xFFu) * wb +
            (float)((c >> shift) & 0xFFu) * wc;
        if (channel < 0.0f) channel = 0.0f;
        if (channel > 255.0f) channel = 255.0f;
        result |= (uint32_t)(channel + 0.5f) << shift;
    }
    return result;
}

static uint32_t ge_modulate_color(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    for (unsigned int shift = 0; shift <= 24; shift += 8) {
        uint32_t av = (a >> shift) & 0xFFu;
        uint32_t bv = (b >> shift) & 0xFFu;
        result |= ((av * bv + 127u) / 255u) << shift;
    }
    return result;
}

static bool ge_texture_sample(PSP_GE *ge, float u, float v,
                              uint32_t *color)
{
    if (!ge->texture_enabled || color == NULL ||
        ge->texture_width == 0 || ge->texture_height == 0 ||
        ge->texture_buffer_width < ge->texture_width ||
        ge->texture_format > 3u)
        return false;

    if (ge->texture_wrap_u == 0u) {
        u = fmodf(u, (float)ge->texture_width);
        if (u < 0.0f) u += (float)ge->texture_width;
    } else {
        if (u < 0.0f) u = 0.0f;
        if (u > (float)(ge->texture_width - 1u))
            u = (float)(ge->texture_width - 1u);
    }
    if (ge->texture_wrap_v == 0u) {
        v = fmodf(v, (float)ge->texture_height);
        if (v < 0.0f) v += (float)ge->texture_height;
    } else {
        if (v < 0.0f) v = 0.0f;
        if (v > (float)(ge->texture_height - 1u))
            v = (float)(ge->texture_height - 1u);
    }

    uint32_t x = (uint32_t)floorf(u);
    uint32_t y = (uint32_t)floorf(v);
    uint32_t bytes_per_pixel = ge->texture_format == 3u ? 4u : 2u;
    uint64_t offset = ((uint64_t)y * ge->texture_buffer_width + x) *
        bytes_per_pixel;
    uint64_t address = (uint64_t)ge->texture_address + offset;
    if (address > UINT32_MAX ||
        !psp_memory_is_valid(ge->memory, (uint32_t)address,
                             bytes_per_pixel))
        return false;

    uint32_t packed = psp_memory_read8(ge->memory, (uint32_t)address) |
        ((uint32_t)psp_memory_read8(ge->memory,
                                    (uint32_t)address + 1u) << 8);
    if (ge->texture_format == 3u) {
        *color = psp_memory_read32(ge->memory, (uint32_t)address);
    } else if (ge->texture_format == 0u) {
        *color = PSP_VIDEO_COLOR(
            ((packed >> 11) & 31u) * 255u / 31u,
            ((packed >> 5) & 63u) * 255u / 63u,
            (packed & 31u) * 255u / 31u, 255u);
    } else if (ge->texture_format == 1u) {
        *color = PSP_VIDEO_COLOR(
            ((packed >> 10) & 31u) * 255u / 31u,
            ((packed >> 5) & 31u) * 255u / 31u,
            (packed & 31u) * 255u / 31u,
            (packed & 0x8000u) ? 255u : 0u);
    } else {
        *color = PSP_VIDEO_COLOR(
            ((packed >> 8) & 15u) * 17u,
            ((packed >> 4) & 15u) * 17u,
            (packed & 15u) * 17u,
            ((packed >> 12) & 15u) * 17u);
    }
    return true;
}

static uint32_t ge_shade_fragment(PSP_GE *ge, uint32_t color,
                                  float u, float v)
{
    uint32_t texel;
    if (!ge->texture_enabled)
        return color;
    if (!ge_texture_sample(ge, u, v, &texel))
        return 0;
    return ge_modulate_color(color, texel);
}

static void ge_draw_triangle(PSP_GE *ge, const GE_Vertex *a,
                             const GE_Vertex *b, const GE_Vertex *c)
{
    int min_x = (int)floorf(fminf(a->x, fminf(b->x, c->x)));
    int max_x = (int)ceilf(fmaxf(a->x, fmaxf(b->x, c->x)));
    int min_y = (int)floorf(fminf(a->y, fminf(b->y, c->y)));
    int max_y = (int)ceilf(fmaxf(a->y, fmaxf(b->y, c->y)));
    if (min_x < ge->scissor_left) min_x = ge->scissor_left;
    if (max_x > ge->scissor_right) max_x = ge->scissor_right;
    if (min_y < ge->scissor_top) min_y = ge->scissor_top;
    if (max_y > ge->scissor_bottom) max_y = ge->scissor_bottom;
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= PSP_VIDEO_WIDTH) max_x = PSP_VIDEO_WIDTH - 1;
    if (max_y >= PSP_VIDEO_HEIGHT) max_y = PSP_VIDEO_HEIGHT - 1;

    float area = ge_edge(a->x, a->y, b->x, b->y, c->x, c->y);
    if (fabsf(area) < 0.0001f) return;
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = ge_edge(b->x, b->y, c->x, c->y, px, py);
            float w1 = ge_edge(c->x, c->y, a->x, a->y, px, py);
            float w2 = ge_edge(a->x, a->y, b->x, b->y, px, py);
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                float wa = w0 / area;
                float wb = w1 / area;
                float wc = w2 / area;
                uint32_t color = ge_interpolate_color(
                    a->color, b->color, c->color, wa, wb, wc);
                float u = a->u * wa + b->u * wb + c->u * wc;
                float v = a->v * wa + b->v * wb + c->v * wc;
                ge_put_pixel(ge, x, y,
                    ge_shade_fragment(ge, color, u, v));
            }
        }
    }
}

static bool ge_draw_primitive(PSP_GE *ge, uint32_t primitive,
                              uint32_t count)
{
    if (count == 0 || count > 4096)
        return count == 0;
    if (ge->texture_enabled &&
        (ge->texture_width == 0 || ge->texture_height == 0 ||
         ge->texture_buffer_width < ge->texture_width ||
         ge->texture_format > 3u))
        return false;
    GE_Vertex vertices[3];
    uint32_t drawn = 0;
    if (primitive == 0) {
        for (uint32_t i = 0; i < count; i++) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0])) return false;
            ge_put_pixel(ge, (int)lroundf(vertices[0].x),
                         (int)lroundf(vertices[0].y),
                         ge_shade_fragment(ge, vertices[0].color,
                                           vertices[0].u, vertices[0].v));
            drawn++;
        }
    } else if (primitive == 1) {
        if (count & 1u) return false;
        for (uint32_t i = 0; i < count; i += 2) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[1])) return false;
            ge_draw_line(ge, &vertices[0], &vertices[1]);
            drawn++;
        }
    } else if (primitive == 2) {
        if (count < 2) return false;
        for (uint32_t i = 0; i + 1 < count; i++) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[1])) return false;
            ge_draw_line(ge, &vertices[0], &vertices[1]);
            drawn++;
        }
    } else if (primitive == 3) {
        if (count % 3u) return false;
        for (uint32_t i = 0; i < count; i += 3) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[1]) ||
                !ge_read_primitive_vertex(ge, i + 2, &vertices[2])) return false;
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
            drawn++;
        }
    } else if (primitive == 4) {
        if (count < 3) return false;
        for (uint32_t i = 0; i + 2 < count; i++) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[1]) ||
                !ge_read_primitive_vertex(ge, i + 2, &vertices[2])) return false;
            if (i & 1u) {
                GE_Vertex swap = vertices[0];
                vertices[0] = vertices[1];
                vertices[1] = swap;
            }
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
            drawn++;
        }
    } else if (primitive == 5) {
        if (count < 3 || !ge_read_primitive_vertex(ge, 0, &vertices[0]))
            return false;
        for (uint32_t i = 1; i + 1 < count; i++) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[1]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[2])) return false;
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
            drawn++;
        }
    } else if (primitive == 6) {
        if (count & 1u) return false;
        for (uint32_t i = 0; i < count; i += 2) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0]) ||
                !ge_read_primitive_vertex(ge, i + 1, &vertices[1])) return false;
            int left = (int)floorf(fminf(vertices[0].x, vertices[1].x));
            int top = (int)floorf(fminf(vertices[0].y, vertices[1].y));
            int right = (int)ceilf(fmaxf(vertices[0].x, vertices[1].x));
            int bottom = (int)ceilf(fmaxf(vertices[0].y, vertices[1].y));
            if (left < ge->scissor_left) left = ge->scissor_left;
            if (top < ge->scissor_top) top = ge->scissor_top;
            if (right > ge->scissor_right) right = ge->scissor_right;
            if (bottom > ge->scissor_bottom) bottom = ge->scissor_bottom;
            if (left < 0) left = 0;
            if (top < 0) top = 0;
            if (right >= PSP_VIDEO_WIDTH) right = PSP_VIDEO_WIDTH - 1;
            if (bottom >= PSP_VIDEO_HEIGHT) bottom = PSP_VIDEO_HEIGHT - 1;
            if (right >= left && bottom >= top) {
                for (int y = top; y <= bottom; y++) {
                    float ty = vertices[1].y != vertices[0].y ?
                        ((float)y - vertices[0].y) /
                        (vertices[1].y - vertices[0].y) : 0.0f;
                    if (ty < 0.0f) ty = 0.0f;
                    if (ty > 1.0f) ty = 1.0f;
                    for (int x = left; x <= right; x++) {
                        float tx = vertices[1].x != vertices[0].x ?
                            ((float)x - vertices[0].x) /
                            (vertices[1].x - vertices[0].x) : 0.0f;
                        if (tx < 0.0f) tx = 0.0f;
                        if (tx > 1.0f) tx = 1.0f;
                        float u = vertices[0].u +
                            (vertices[1].u - vertices[0].u) * tx;
                        float v = vertices[0].v +
                            (vertices[1].v - vertices[0].v) * ty;
                        ge_put_pixel(ge, x, y,
                            ge_shade_fragment(ge, vertices[0].color, u, v));
                    }
                }
            }
            drawn++;
        }
    } else {
        return false;
    }
    ge->primitives_drawn += drawn;
    return true;
}

bool psp_ge_init(PSP_GE *ge, PSP_Memory *memory, PSP_Video *video)
{
    if (ge == NULL || memory == NULL || video == NULL ||
        memory->ram == NULL || !video->initialized)
        return false;
    memset(ge, 0, sizeof(*ge));
    ge->memory = memory;
    ge->video = video;
    ge->initialized = true;
    psp_ge_reset(ge);
    return true;
}

void psp_ge_reset(PSP_GE *ge)
{
    if (ge == NULL || !ge->initialized) return;
    ge->base_address = 0;
    ge->vertex_address = 0;
    ge->index_address = 0;
    ge->vertex_type = (2u << 7) | (1u << 23);
    ge->draw_color = 0xFFFFFFFFu;
    ge->texture_address = 0;
    ge->texture_address_low = 0;
    ge->texture_buffer_width_register = 0;
    ge->texture_buffer_width = 0;
    ge->texture_width = 0;
    ge->texture_height = 0;
    ge->texture_format = 3;
    ge->texture_wrap_u = 1;
    ge->texture_wrap_v = 1;
    ge->texture_enabled = false;
    ge->alpha_blend_enabled = false;
    ge->alpha_test_enabled = false;
    ge->alpha_test_function = 1u;
    ge->alpha_test_reference = 0;
    ge->alpha_test_mask = 0xFFu;
    ge->blend_operation = 0;
    ge->blend_source_factor = 2;
    ge->blend_destination_factor = 3;
    ge->blend_fixed_a = 0xFFFFFFFFu;
    ge->blend_fixed_b = 0xFFFFFFFFu;
    ge->scissor_left = 0;
    ge->scissor_top = 0;
    ge->scissor_right = PSP_VIDEO_WIDTH - 1;
    ge->scissor_bottom = PSP_VIDEO_HEIGHT - 1;
    ge->commands_executed = 0;
    ge->primitives_drawn = 0;
}

bool psp_ge_execute_list(PSP_GE *ge, uint32_t list_address,
                         uint32_t max_commands)
{
    if (ge == NULL || !ge->initialized || max_commands == 0 ||
        max_commands > 100000)
        return false;

    uint32_t return_stack[16];
    uint32_t stack_depth = 0;
    uint64_t pc64 = list_address;
    for (uint32_t i = 0; i < max_commands; i++, pc64 += 4u) {
        if (pc64 > UINT32_MAX) return false;
        uint32_t pc = (uint32_t)pc64;
        if (!psp_memory_is_valid(ge->memory, pc, 4)) return false;
        uint32_t word = psp_memory_read32(ge->memory, pc);
        uint32_t command = word >> 24;
        uint32_t value = word & 0x00FFFFFFu;
        ge->commands_executed++;

        switch (command) {
            case GE_CMD_BASE:
                ge->base_address = (value & 0x00FF0000u) << 8;
                break;
            case GE_CMD_VADDR:
                ge->vertex_address = ge_resolve_address(ge, value);
                break;
            case GE_CMD_IADDR:
                ge->index_address = ge_resolve_address(ge, value);
                break;
            case GE_CMD_VTYPE:
                ge->vertex_type = value;
                break;
            case GE_CMD_TEXTUREMAPENABLE:
                ge->texture_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_ALPHABLENDENABLE:
                ge->alpha_blend_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_ALPHATESTENABLE:
                ge->alpha_test_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_ALPHATEST:
                ge->alpha_test_function = value & 7u;
                ge->alpha_test_reference = (value >> 8) & 0xFFu;
                ge->alpha_test_mask = (value >> 16) & 0xFFu;
                break;
            case GE_CMD_BLENDMODE:
                ge->blend_operation = value & 7u;
                ge->blend_source_factor = (value >> 4) & 0xFu;
                ge->blend_destination_factor = (value >> 8) & 0xFu;
                if (ge->blend_operation > 5u ||
                    ge->blend_source_factor > 10u ||
                    ge->blend_destination_factor > 10u)
                    return false;
                break;
            case GE_CMD_BLENDFIXEDA:
                ge->blend_fixed_a = value;
                break;
            case GE_CMD_BLENDFIXEDB:
                ge->blend_fixed_b = value;
                break;
            case GE_CMD_TEXADDR0:
                ge->texture_address_low = value & 0x00FFFFFFu;
                ge->texture_address =
                    ((ge->texture_buffer_width_register & 0x000F0000u) << 8) |
                    ge->texture_address_low;
                break;
            case GE_CMD_TEXBUFWIDTH0:
                ge->texture_buffer_width_register = value;
                ge->texture_buffer_width = value & 0x3FFu;
                ge->texture_address =
                    ((value & 0x000F0000u) << 8) |
                    ge->texture_address_low;
                break;
            case GE_CMD_TEXSIZE0: {
                uint32_t width_exp = value & 0xFu;
                uint32_t height_exp = (value >> 8) & 0xFu;
                if (width_exp > 9u || height_exp > 9u)
                    return false;
                ge->texture_width = 1u << width_exp;
                ge->texture_height = 1u << height_exp;
                break;
            }
            case GE_CMD_TEXFORMAT:
                ge->texture_format = value & 0xFu;
                break;
            case GE_CMD_TEXWRAP:
                ge->texture_wrap_u = value & 3u;
                ge->texture_wrap_v = (value >> 8) & 3u;
                if (ge->texture_wrap_u > 1u || ge->texture_wrap_v > 1u)
                    return false;
                break;
            case GE_CMD_OFFSET:
                /* Offset is used by transformed 3D vertices; unsupported here. */
                break;
            case GE_CMD_JUMP: {
                uint32_t target = ge_resolve_address(ge, value);
                if (target < 4u) return false;
                pc64 = (uint64_t)target - 4u;
                break;
            }
            case GE_CMD_CALL: {
                if (stack_depth >= 16u) return false;
                uint32_t target = ge_resolve_address(ge, value);
                if (target < 4u || pc > UINT32_MAX - 4u) return false;
                return_stack[stack_depth++] = pc + 4u;
                pc64 = (uint64_t)target - 4u;
                break;
            }
            case GE_CMD_RET:
                if (stack_depth == 0 || return_stack[stack_depth - 1] < 4u)
                    return false;
                pc64 = (uint64_t)return_stack[--stack_depth] - 4u;
                break;
            case GE_CMD_SCISSOR1:
                ge->scissor_left = (int)(value & 0x3FFu);
                ge->scissor_top = (int)((value >> 10) & 0x3FFu);
                break;
            case GE_CMD_SCISSOR2:
                ge->scissor_right = (int)(value & 0x3FFu);
                ge->scissor_bottom = (int)((value >> 10) & 0x3FFu);
                if (ge->scissor_right >= PSP_VIDEO_WIDTH)
                    ge->scissor_right = PSP_VIDEO_WIDTH - 1;
                if (ge->scissor_bottom >= PSP_VIDEO_HEIGHT)
                    ge->scissor_bottom = PSP_VIDEO_HEIGHT - 1;
                break;
            case GE_CMD_PRIM: {
                uint32_t primitive = (value >> 16) & 7u;
                uint32_t count = value & 0xFFFFu;
                if (!ge_draw_primitive(ge, primitive, count))
                    return false;
                break;
            }
            case GE_CMD_END:
                return true;
            default:
                /* Unknown state commands are skipped without corrupting RAM. */
                break;
        }
    }
    return false;
}
