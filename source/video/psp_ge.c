#include "psp_ge.h"

#include <math.h>
#include <string.h>

#define GE_CMD_VADDR   0x01u
#define GE_CMD_PRIM    0x04u
#define GE_CMD_END     0x0Cu
#define GE_CMD_BASE    0x10u
#define GE_CMD_VTYPE   0x12u
#define GE_CMD_OFFSET  0x13u
#define GE_CMD_SCISSOR1 0x17u
#define GE_CMD_SCISSOR2 0x18u

typedef struct {
    float x;
    float y;
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

    if (texture_type != 0 || normal_type != 0 || weight_type != 0 ||
        weight_count != 1 || morph_count != 1 ||
        (position_type != 2 && position_type != 3) ||
        color_type > 4)
        return false;

    uint32_t stride = 0;
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
        fabsf(vertex->x) > 32768.0f || fabsf(vertex->y) > 32768.0f)
        return false;
    if (!transform_2d) {
        vertex->x = (vertex->x + 1.0f) * (PSP_VIDEO_WIDTH / 2.0f);
        vertex->y = (1.0f - vertex->y) * (PSP_VIDEO_HEIGHT / 2.0f);
    }
    return true;
}

static void ge_put_pixel(PSP_GE *ge, int x, int y, uint32_t color)
{
    if (x < ge->scissor_left || x > ge->scissor_right ||
        y < ge->scissor_top || y > ge->scissor_bottom)
        return;
    psp_video_put_pixel(ge->video, x, y, color);
}

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
    for (;;) {
        ge_put_pixel(ge, x0, y0, a->color);
        if (x0 == x1 && y0 == y1) break;
        int twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
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
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0))
                ge_put_pixel(ge, x, y,
                    ge_interpolate_color(a->color, b->color, c->color,
                        w0 / area, w1 / area, w2 / area));
        }
    }
}

static bool ge_draw_primitive(PSP_GE *ge, uint32_t primitive,
                              uint32_t count)
{
    if (count == 0 || count > 4096)
        return count == 0;
    GE_Vertex vertices[3];
    if (primitive == 0) {
        for (uint32_t i = 0; i < count; i++) {
            if (!ge_read_vertex(ge, i, &vertices[0])) return false;
            ge_put_pixel(ge, (int)lroundf(vertices[0].x),
                         (int)lroundf(vertices[0].y), vertices[0].color);
        }
    } else if (primitive == 1) {
        if (count & 1u) return false;
        for (uint32_t i = 0; i < count; i += 2) {
            if (!ge_read_vertex(ge, i, &vertices[0]) ||
                !ge_read_vertex(ge, i + 1, &vertices[1])) return false;
            ge_draw_line(ge, &vertices[0], &vertices[1]);
        }
    } else if (primitive == 2) {
        if (count < 2) return false;
        for (uint32_t i = 0; i + 1 < count; i++) {
            if (!ge_read_vertex(ge, i, &vertices[0]) ||
                !ge_read_vertex(ge, i + 1, &vertices[1])) return false;
            ge_draw_line(ge, &vertices[0], &vertices[1]);
        }
    } else if (primitive == 3) {
        if (count % 3u) return false;
        for (uint32_t i = 0; i < count; i += 3) {
            if (!ge_read_vertex(ge, i, &vertices[0]) ||
                !ge_read_vertex(ge, i + 1, &vertices[1]) ||
                !ge_read_vertex(ge, i + 2, &vertices[2])) return false;
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
        }
    } else if (primitive == 4) {
        if (count < 3) return false;
        for (uint32_t i = 0; i + 2 < count; i++) {
            if (!ge_read_vertex(ge, i, &vertices[0]) ||
                !ge_read_vertex(ge, i + 1, &vertices[1]) ||
                !ge_read_vertex(ge, i + 2, &vertices[2])) return false;
            if (i & 1u) {
                GE_Vertex swap = vertices[0];
                vertices[0] = vertices[1];
                vertices[1] = swap;
            }
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
        }
    } else if (primitive == 5) {
        if (count < 3 || !ge_read_vertex(ge, 0, &vertices[0]))
            return false;
        for (uint32_t i = 1; i + 1 < count; i++) {
            if (!ge_read_vertex(ge, i, &vertices[1]) ||
                !ge_read_vertex(ge, i + 1, &vertices[2])) return false;
            ge_draw_triangle(ge, &vertices[0], &vertices[1], &vertices[2]);
        }
    } else if (primitive == 6) {
        if (count & 1u) return false;
        for (uint32_t i = 0; i < count; i += 2) {
            if (!ge_read_vertex(ge, i, &vertices[0]) ||
                !ge_read_vertex(ge, i + 1, &vertices[1])) return false;
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
                for (int y = top; y <= bottom; y++)
                    for (int x = left; x <= right; x++)
                        ge_put_pixel(ge, x, y, vertices[0].color);
            }
        }
    } else {
        return false;
    }
    ge->primitives_drawn++;
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
    ge->vertex_type = (2u << 7) | (1u << 23);
    ge->draw_color = 0xFFFFFFFFu;
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
            case GE_CMD_VTYPE:
                ge->vertex_type = value;
                break;
            case GE_CMD_OFFSET:
                /* Offset is used by transformed 3D vertices; unsupported here. */
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
