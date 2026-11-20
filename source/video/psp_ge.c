#include "psp_ge.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GE_CMD_VADDR   0x01u
#define GE_CMD_IADDR   0x02u
#define GE_CMD_WORLDMATRIXNUMBER 0x3Au
#define GE_CMD_WORLDMATRIXDATA 0x3Bu
#define GE_CMD_VIEWMATRIXNUMBER 0x3Cu
#define GE_CMD_VIEWMATRIXDATA 0x3Du
#define GE_CMD_PROJMATRIXNUMBER 0x3Eu
#define GE_CMD_PROJMATRIXDATA 0x3Fu
#define GE_CMD_VIEWPORTXSCALE 0x42u
#define GE_CMD_VIEWPORTYSCALE 0x43u
#define GE_CMD_VIEWPORTZSCALE 0x44u
#define GE_CMD_VIEWPORTXCENTER 0x45u
#define GE_CMD_VIEWPORTYCENTER 0x46u
#define GE_CMD_VIEWPORTZCENTER 0x47u
#define GE_CMD_OFFSETX 0x4Cu
#define GE_CMD_OFFSETY 0x4Du
#define GE_CMD_ZTESTENABLE 0x23u
#define GE_CMD_STENCILTEST 0xDCu
#define GE_CMD_STENCILOP 0xDDu
#define GE_CMD_PIXELMASKALPHA 0xE9u
#define GE_CMD_STENCILTESTENABLE 0x24u
#define GE_CMD_CULLFACEENABLE 0x1Du
#define GE_CMD_CULL 0x9Bu
#define GE_CMD_LIGHTINGENABLE 0x17u
#define GE_CMD_MATERIALSPECULAR 0x55u
#define GE_CMD_MATERIALSPECULARCOEF 0x58u
#define GE_CMD_FOGENABLE 0x1Fu
#define GE_CMD_LIGHTENABLE0 0x18u
#define GE_CMD_LIGHTENABLE1 0x19u
#define GE_CMD_LIGHTENABLE2 0x1Au
#define GE_CMD_LIGHTENABLE3 0x1Bu
#define GE_CMD_LIGHTTYPE1 0x60u
#define GE_CMD_LIGHTTYPE2 0x61u
#define GE_CMD_LIGHTTYPE3 0x62u
#define GE_CMD_LDX1 0x72u
#define GE_CMD_LDY1 0x73u
#define GE_CMD_LDZ1 0x74u
#define GE_CMD_LDX2 0x75u
#define GE_CMD_LDY2 0x76u
#define GE_CMD_LDZ2 0x77u
#define GE_CMD_LDX3 0x78u
#define GE_CMD_LDY3 0x79u
#define GE_CMD_LDZ3 0x7Au
#define GE_CMD_LSC0 0x91u
#define GE_CMD_LDC1 0x93u
#define GE_CMD_LSC1 0x94u
#define GE_CMD_LDC2 0x96u
#define GE_CMD_LSC2 0x97u
#define GE_CMD_LDC3 0x99u
#define GE_CMD_LSC3 0x9Au
#define GE_CMD_AMBIENTCOLOR 0x5Cu
#define GE_CMD_LIGHTTYPE0 0x5Fu
#define GE_CMD_LX0 0x63u
#define GE_CMD_LY0 0x64u
#define GE_CMD_LZ0 0x65u
#define GE_CMD_LX1 0x66u
#define GE_CMD_LY1 0x67u
#define GE_CMD_LZ1 0x68u
#define GE_CMD_LX2 0x69u
#define GE_CMD_LY2 0x6Au
#define GE_CMD_LZ2 0x6Bu
#define GE_CMD_LX3 0x6Cu
#define GE_CMD_LY3 0x6Du
#define GE_CMD_LZ3 0x6Eu
#define GE_CMD_LKA0 0x7Bu
#define GE_CMD_LKB0 0x7Cu
#define GE_CMD_LKC0 0x7Du
#define GE_CMD_LKA1 0x7Eu
#define GE_CMD_LKB1 0x7Fu
#define GE_CMD_LKC1 0x80u
#define GE_CMD_LKA2 0x81u
#define GE_CMD_LKB2 0x82u
#define GE_CMD_LKC2 0x83u
#define GE_CMD_LKA3 0x84u
#define GE_CMD_LKB3 0x85u
#define GE_CMD_LKC3 0x86u
#define GE_CMD_LKS0 0x87u
#define GE_CMD_LKS1 0x88u
#define GE_CMD_LKS2 0x89u
#define GE_CMD_LKS3 0x8Au
#define GE_CMD_LKO0 0x8Bu
#define GE_CMD_LKO1 0x8Cu
#define GE_CMD_LKO2 0x8Du
#define GE_CMD_LKO3 0x8Eu
#define GE_CMD_LDX0 0x6Fu
#define GE_CMD_LDY0 0x70u
#define GE_CMD_LDZ0 0x71u
#define GE_CMD_LDC0 0x90u
#define GE_CMD_ZTEST 0xDEu
#define GE_CMD_ZWRITEDISABLE 0xE7u
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
#define GE_CMD_COLORTESTENABLE 0x27u
#define GE_CMD_COLORTEST 0xD8u
#define GE_CMD_COLORREF 0xD9u
#define GE_CMD_COLORTESTMASK 0xDAu
#define GE_CMD_CLEARMODE 0xD3u
#define GE_CMD_ALPHATEST 0xDBu
#define GE_CMD_BLENDMODE 0xDFu
#define GE_CMD_BLENDFIXEDA 0xE0u
#define GE_CMD_BLENDFIXEDB 0xE1u
#define GE_CMD_TEXADDR0 0xA0u
#define GE_CMD_TEXBUFWIDTH0 0xA8u
#define GE_CMD_TEXSIZE0 0xB8u
#define GE_CMD_TEXFORMAT 0xC3u
#define GE_CMD_TEXFILTER 0xC6u
#define GE_CMD_TEXFUNC 0xC9u
#define GE_CMD_TEXENV 0xCAu
#define GE_CMD_TEXWRAP 0xC7u
#define GE_CMD_CLUTADDR 0xB0u
#define GE_CMD_CLUTADDRUPPER 0xB1u
#define GE_CMD_CLUTLOAD 0xC4u
#define GE_CMD_CLUTFORMAT 0xC5u
#define GE_CMD_FOG1 0xCDu
#define GE_CMD_FOG2 0xCEu
#define GE_CMD_FOGCOLOR 0xCFu
#define GE_CMD_TRXSBP 0xB2u
#define GE_CMD_TRXSBW 0xB3u
#define GE_CMD_TRXDBP 0xB4u
#define GE_CMD_TRXDBW 0xB5u
#define GE_CMD_TRXKICK 0xEAu
#define GE_CMD_TRXSPOS 0xEBu
#define GE_CMD_TRXDPOS 0xECu
#define GE_CMD_TRXSIZE 0xEEu
#define GE_CMD_FRAMEBUFPTR 0x9Cu
#define GE_CMD_FRAMEBUFWIDTH 0x9Du
#define GE_CMD_FRAMEBUFPIXFORMAT 0xD2u

typedef struct {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float u;
    float v;
    float reciprocal_w;
    float clip_x;
    float clip_y;
    float clip_z;
    float clip_w;
    float view_x;
    float view_y;
    float view_z;
    bool clip_valid;
    uint8_t fog_factor;
    uint32_t color;
} GE_Vertex;

static uint32_t ge_resolve_address(const PSP_GE *ge, uint32_t address)
{
    return (ge->base_address & 0xFF000000u) | (address & 0x00FFFFFFu);
}

static float ge_decode_float24(uint32_t value)
{
    uint32_t bits = (value & 0x00FFFFFFu) << 8;
    float result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

static bool ge_transform_vertex(PSP_GE *ge, GE_Vertex *vertex)
{
    float x = vertex->x, y = vertex->y, z = vertex->z;
    const float *w = ge->world_matrix;
    const float *v = ge->view_matrix;
    const float *p = ge->projection_matrix;

    float wx = w[0] * x + w[1] * y + w[2] * z + w[3];
    float wy = w[4] * x + w[5] * y + w[6] * z + w[7];
    float wz = w[8] * x + w[9] * y + w[10] * z + w[11];

    float vx = v[0] * wx + v[1] * wy + v[2] * wz + v[3];
    float vy = v[4] * wx + v[5] * wy + v[6] * wz + v[7];
    float vz = v[8] * wx + v[9] * wy + v[10] * wz + v[11];
    vertex->view_x = vx;
    vertex->view_y = vy;
    vertex->view_z = vz;

    float nwx = w[0] * vertex->nx + w[1] * vertex->ny +
        w[2] * vertex->nz;
    float nwy = w[4] * vertex->nx + w[5] * vertex->ny +
        w[6] * vertex->nz;
    float nwz = w[8] * vertex->nx + w[9] * vertex->ny +
        w[10] * vertex->nz;
    float nnx = v[0] * nwx + v[1] * nwy + v[2] * nwz;
    float nny = v[4] * nwx + v[5] * nwy + v[6] * nwz;
    float nnz = v[8] * nwx + v[9] * nwy + v[10] * nwz;
    float normal_length = sqrtf(nnx * nnx + nny * nny + nnz * nnz);
    if (normal_length > 0.000001f) {
        vertex->nx = nnx / normal_length;
        vertex->ny = nny / normal_length;
        vertex->nz = nnz / normal_length;
    }

    float cx = p[0] * vx + p[1] * vy + p[2] * vz + p[3];
    float cy = p[4] * vx + p[5] * vy + p[6] * vz + p[7];
    float cz = p[8] * vx + p[9] * vy + p[10] * vz + p[11];
    float cw = p[12] * vx + p[13] * vy + p[14] * vz + p[15];

    if (!isfinite(cx) || !isfinite(cy) || !isfinite(cz) ||
        !isfinite(cw))
        return false;
    vertex->fog_factor = 255u;
    if (ge->fog_enabled) {
        float fog = (vz + ge->fog_end) * ge->fog_slope;
        if (isnan(fog) || fog <= 0.0f) {
            vertex->fog_factor = 0u;
        } else if (fog >= 1.0f) {
            vertex->fog_factor = 255u;
        } else {
            float factor = floorf(fog * 256.0f);
            vertex->fog_factor = (uint8_t)(factor > 255.0f ? 255u : factor);
        }
    }
    vertex->clip_x = cx;
    vertex->clip_y = cy;
    vertex->clip_z = cz;
    vertex->clip_w = cw;
    vertex->clip_valid = true;
    if (fabsf(cw) < 0.000001f) {
        /* Keep the homogeneous vertex for polygon clipping; its temporary
           screen position is unused unless the primitive is a line/point. */
        vertex->reciprocal_w = 0.0f;
        vertex->x = vertex->y = vertex->z = 0.0f;
        return true;
    }
    float inv_w = 1.0f / cw;
    vertex->reciprocal_w = inv_w;
    float nx = cx * inv_w, ny = cy * inv_w, nz = cz * inv_w;
    /* Homogeneous triangle clipping uses clip coordinates, not these
       temporary projected values. Keep extreme finite vertices available. */
    if (!isfinite(nx) || !isfinite(ny) || !isfinite(nz) ||
        fabsf(nx) > 1000000.0f || fabsf(ny) > 1000000.0f ||
        fabsf(nz) > 1000000.0f) {
        vertex->x = vertex->y = vertex->z = 0.0f;
        return true;
    }

    vertex->x = ge->viewport_center[0] +
        nx * ge->viewport_scale[0] - ge->viewport_offset[0];
    vertex->y = ge->viewport_center[1] +
        ny * ge->viewport_scale[1] - ge->viewport_offset[1];
    vertex->z = ge->viewport_center[2] +
        nz * ge->viewport_scale[2];
    if (!isfinite(vertex->x) || !isfinite(vertex->y) ||
        !isfinite(vertex->z))
        return false;
    if (vertex->z < 0.0f) vertex->z = 0.0f;
    if (vertex->z > 65535.0f) vertex->z = 65535.0f;
    return true;
}

static void ge_apply_lighting(const PSP_GE *ge, GE_Vertex *vertex)
{
    if (!ge->lighting_enabled || ge->clear_mode)
        return;

    float diffuse_amount[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float specular_amount[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float normal_length = sqrtf(vertex->nx * vertex->nx +
        vertex->ny * vertex->ny + vertex->nz * vertex->nz);
    if (normal_length > 0.000001f && isfinite(normal_length)) {
        float normal[3] = {
            vertex->nx / normal_length,
            vertex->ny / normal_length,
            vertex->nz / normal_length
        };
        float view_length = sqrtf(vertex->view_x * vertex->view_x +
            vertex->view_y * vertex->view_y +
            vertex->view_z * vertex->view_z);
        float vx = view_length > 0.000001f ?
            -vertex->view_x / view_length : 0.0f;
        float vy = view_length > 0.000001f ?
            -vertex->view_y / view_length : 0.0f;
        float vz = view_length > 0.000001f ?
            -vertex->view_z / view_length : 1.0f;

        for (unsigned int light = 0; light < 4u; light++) {
            bool enabled = light == 0u ? ge->light0_enabled :
                ge->light_enabled[light];
            uint32_t type = light == 0u ? ge->light0_type :
                ge->light_type[light];
            const float *direction = light == 0u ? ge->light0_direction :
                ge->light_direction[light];
            if (!enabled || type > 2u)
                continue;

            float lx, ly, lz, distance = 0.0f;
            float attenuation = 1.0f;
            if (type == 0u) {
                float direction_length = sqrtf(direction[0] * direction[0] +
                    direction[1] * direction[1] +
                    direction[2] * direction[2]);
                if (direction_length <= 0.000001f ||
                    !isfinite(direction_length))
                    continue;
                lx = -direction[0] / direction_length;
                ly = -direction[1] / direction_length;
                lz = -direction[2] / direction_length;
            } else {
                const float *position = ge->light_position[light];
                float px = ge->view_matrix[0] * position[0] +
                    ge->view_matrix[1] * position[1] +
                    ge->view_matrix[2] * position[2] + ge->view_matrix[3];
                float py = ge->view_matrix[4] * position[0] +
                    ge->view_matrix[5] * position[1] +
                    ge->view_matrix[6] * position[2] + ge->view_matrix[7];
                float pz = ge->view_matrix[8] * position[0] +
                    ge->view_matrix[9] * position[1] +
                    ge->view_matrix[10] * position[2] + ge->view_matrix[11];
                float dx = px - vertex->view_x;
                float dy = py - vertex->view_y;
                float dz = pz - vertex->view_z;
                distance = sqrtf(dx * dx + dy * dy + dz * dz);
                if (distance <= 0.000001f || !isfinite(distance))
                    continue;
                lx = dx / distance;
                ly = dy / distance;
                lz = dz / distance;

                const float *coeff = ge->light_attenuation[light];
                float denominator = coeff[0] + coeff[1] * distance +
                    coeff[2] * distance * distance;
                if (!isfinite(denominator) || denominator <= 0.000001f)
                    continue;
                attenuation = 1.0f / denominator;
                if (type == 2u) {
                    float spot_length = sqrtf(direction[0] * direction[0] +
                        direction[1] * direction[1] +
                        direction[2] * direction[2]);
                    if (spot_length <= 0.000001f ||
                        !isfinite(spot_length))
                        continue;
                    float spot = (direction[0] * -lx +
                        direction[1] * -ly + direction[2] * -lz) /
                        spot_length;
                    float cutoff = ge->light_spot_cutoff[light];
                    if (spot < cutoff)
                        continue;
                    float exponent = ge->light_spot_exponent[light];
                    if (spot <= 0.0f)
                        continue;
                    attenuation *= powf(spot, exponent);
                }
            }

            float ndotl = normal[0] * lx + normal[1] * ly +
                normal[2] * lz;
            diffuse_amount[light] = ndotl > 0.0f ? ndotl : 0.0f;
            diffuse_amount[light] *= attenuation;
            if (diffuse_amount[light] > 1.0f)
                diffuse_amount[light] = 1.0f;

            if (view_length > 0.000001f && isfinite(view_length) &&
                ge->material_specular_coefficient > 0.0f &&
                (ge->material_specular & 0x00FFFFFFu) != 0u &&
                (ge->light_specular[light] & 0x00FFFFFFu) != 0u) {
                float hx = lx + vx, hy = ly + vy, hz = lz + vz;
                float half_length = sqrtf(hx * hx + hy * hy + hz * hz);
                if (half_length > 0.000001f && isfinite(half_length)) {
                    float ndoth = (normal[0] * hx + normal[1] * hy +
                        normal[2] * hz) / half_length;
                    if (ndoth > 0.0f) {
                        float power = powf(ndoth,
                            ge->material_specular_coefficient);
                        if (isfinite(power))
                            specular_amount[light] = power * attenuation;
                    }
                }
            }
        }
    }

    uint32_t result = vertex->color & 0xFF000000u;
    for (unsigned int shift = 0; shift <= 16; shift += 8) {
        float factor = (float)((ge->ambient_color >> shift) & 0xFFu) / 255.0f;
        float specular = 0.0f;
        for (unsigned int light = 0; light < 4u; light++) {
            uint32_t diffuse_color = light == 0u ? ge->light0_diffuse :
                ge->light_diffuse[light];
            float diffuse = (float)((diffuse_color >> shift) & 0xFFu) / 255.0f;
            factor += diffuse * diffuse_amount[light];
            float material = (float)((ge->material_specular >> shift) & 0xFFu) / 255.0f;
            float light_color = (float)((ge->light_specular[light] >> shift) & 0xFFu) / 255.0f;
            specular += material * light_color * specular_amount[light];
        }
        if (factor > 1.0f) factor = 1.0f;
        float channel = (float)((vertex->color >> shift) & 0xFFu) * factor +
            specular * 255.0f;
        if (channel > 255.0f) channel = 255.0f;
        result |= (uint32_t)(channel + 0.5f) << shift;
    }
    vertex->color = result;
}

static bool ge_read_vertex(PSP_GE *ge, uint32_t index, GE_Vertex *vertex)
{
    uint32_t type = ge->vertex_type;
    vertex->clip_valid = false;
    uint32_t color_type = (type >> 2) & 7u;
    uint32_t position_type = (type >> 7) & 3u;
    uint32_t texture_type = type & 3u;
    uint32_t normal_type = (type >> 5) & 3u;
    uint32_t weight_type = (type >> 9) & 3u;
    uint32_t weight_count = ((type >> 14) & 7u) + 1u;
    uint32_t morph_count = ((type >> 18) & 7u) + 1u;
    bool transform_2d = ((type >> 23) & 1u) != 0;

    if (weight_type != 0 || weight_count != 1 || morph_count != 1 ||
        (position_type < 1 || position_type > 3) ||
        (color_type != 0u && (color_type < 4u || color_type > 7u)) ||
        (ge->texture_enabled && texture_type == 0))
        return false;

    uint32_t position_size = position_type == 1 ? 1u :
        (position_type == 2 ? 2u : 4u);
    uint32_t position_components = transform_2d ? 2u : 3u;
    uint32_t stride = texture_type == 1 ? 2u :
        (texture_type == 2 ? 4u : (texture_type == 3 ? 8u : 0u));
    if (color_type >= 4u && color_type <= 6u)
        stride += 2;
    else if (color_type == 7u)
        stride += 4;
    uint32_t normal_size = normal_type == 1u ? 1u :
        (normal_type == 2u ? 2u : 4u);
    if (normal_type != 0u)
        stride += normal_size * 3u;
    stride += position_size * position_components;

    uint64_t address64 = (uint64_t)ge->vertex_address +
        (uint64_t)index * stride;
    if (address64 > UINT32_MAX ||
        !psp_memory_is_valid(ge->memory, (uint32_t)address64, stride))
        return false;

    uint32_t address = (uint32_t)address64;
    vertex->u = 0.0f;
    vertex->v = 0.0f;
    vertex->fog_factor = 255u;
    vertex->reciprocal_w = 1.0f;
    if (texture_type == 1) {
        vertex->u = (float)psp_memory_read8(ge->memory, address);
        vertex->v = (float)psp_memory_read8(ge->memory, address + 1u);
        address += 2u;
    } else if (texture_type == 2) {
        uint32_t uv = psp_memory_read8(ge->memory, address) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 1u) << 8);
        uint32_t vv = psp_memory_read8(ge->memory, address + 2u) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 3u) << 8);
        vertex->u = (float)uv;
        vertex->v = (float)vv;
        address += 4u;
    } else if (texture_type == 3) {
        uint32_t ub = psp_memory_read32(ge->memory, address);
        uint32_t vb = psp_memory_read32(ge->memory, address + 4u);
        memcpy(&vertex->u, &ub, sizeof(ub));
        memcpy(&vertex->v, &vb, sizeof(vb));
        address += 8u;
    }

    vertex->color = ge->draw_color;
    if (color_type >= 4u && color_type <= 6u) {
        uint32_t packed = psp_memory_read8(ge->memory, address) |
            ((uint32_t)psp_memory_read8(ge->memory, address + 1u) << 8);
        address += 2u;
        if (color_type == 4u) {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 11) & 31u) * 255u / 31u,
                ((packed >> 5) & 63u) * 255u / 63u,
                (packed & 31u) * 255u / 31u, 255u);
        } else if (color_type == 5u) {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 10) & 31u) * 255u / 31u,
                ((packed >> 5) & 31u) * 255u / 31u,
                (packed & 31u) * 255u / 31u,
                (packed & 0x8000u) ? 255u : 0u);
        } else {
            vertex->color = PSP_VIDEO_COLOR(
                ((packed >> 8) & 15u) * 17u,
                ((packed >> 4) & 15u) * 17u,
                (packed & 15u) * 17u,
                ((packed >> 12) & 15u) * 17u);
        }
    } else if (color_type == 7u) {
        vertex->color = psp_memory_read32(ge->memory, address);
        address += 4u;
    }

    vertex->nx = 0.0f;
    vertex->ny = 0.0f;
    vertex->nz = 1.0f;
    if (normal_type != 0u) {
        float *normal[3] = {&vertex->nx, &vertex->ny, &vertex->nz};
        for (uint32_t component = 0; component < 3u; component++) {
            if (normal_type == 1u) {
                *normal[component] =
                    (float)(int8_t)psp_memory_read8(ge->memory, address);
                address += 1u;
            } else if (normal_type == 2u) {
                uint16_t packed = psp_memory_read8(ge->memory, address) |
                    ((uint16_t)psp_memory_read8(ge->memory, address + 1u) << 8);
                *normal[component] = (float)(int16_t)packed;
                address += 2u;
            } else {
                uint32_t bits = psp_memory_read32(ge->memory, address);
                memcpy(normal[component], &bits, sizeof(bits));
                address += 4u;
            }
        }
    }

    float position[3] = {0.0f, 0.0f, 0.0f};
    for (uint32_t component = 0; component < position_components;
         component++) {
        if (position_type == 1) {
            position[component] =
                (float)(int8_t)psp_memory_read8(ge->memory, address);
            address += 1u;
        } else if (position_type == 2) {
            uint16_t packed = psp_memory_read8(ge->memory, address) |
                ((uint16_t)psp_memory_read8(ge->memory, address + 1u) << 8);
            position[component] = (float)(int16_t)packed;
            address += 2u;
        } else {
            uint32_t bits = psp_memory_read32(ge->memory, address);
            memcpy(&position[component], &bits, sizeof(bits));
            address += 4u;
        }
    }
    vertex->x = position[0];
    vertex->y = position[1];
    vertex->z = position[2];

    if (!isfinite(vertex->x) || !isfinite(vertex->y) ||
        !isfinite(vertex->z) || !isfinite(vertex->nx) ||
        !isfinite(vertex->ny) || !isfinite(vertex->nz) ||
        !isfinite(vertex->u) ||
        !isfinite(vertex->v) || fabsf(vertex->x) > 32768.0f ||
        fabsf(vertex->y) > 32768.0f || fabsf(vertex->z) > 32768.0f ||
        fabsf(vertex->u) > 65536.0f || fabsf(vertex->v) > 65536.0f)
        return false;

    if (!transform_2d) {
        if (!ge_transform_vertex(ge, vertex))
            return false;
        ge_apply_lighting(ge, vertex);
    }
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
            /* GE's 32-bit index format has a 32-bit stride, but only
               the low 16 bits are used by the hardware. */
            vertex_index = psp_memory_read8(ge->memory, address) |
                ((uint32_t)psp_memory_read8(ge->memory, address + 1u) << 8);
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

static uint8_t ge_apply_stencil_operation(uint32_t operation,
                                             uint8_t current,
                                             uint8_t reference)
{
    switch (operation & 7u) {
        case 1: return 0u;
        case 2: return reference;
        case 3: return (uint8_t)~current;
        case 4: return current == 0xFFu ? 0xFFu : (uint8_t)(current + 1u);
        case 5: return current == 0u ? 0u : (uint8_t)(current - 1u);
        case 0:
        default: return current;
    }
}

static uint32_t ge_merge_alpha(uint32_t destination, uint8_t alpha,
                               uint32_t write_mask)
{
    uint32_t writable = (~write_mask & 0xFFu) << 24;
    return (destination & ~writable) | (((uint32_t)alpha << 24) & writable);
}

static void ge_store_framebuffer_pixel(PSP_GE *ge, int x, int y,
                                          uint32_t color)
{
    if (ge == NULL || ge->memory == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= ge->display_stride)
        return;
    uint32_t bpp = ge->display_format == 3u ? 4u : 2u;
    uint64_t address = (uint64_t)ge->display_address +
        ((uint64_t)(uint32_t)y * ge->framebuffer_stride +
         (uint32_t)x) * bpp;
    if (address > UINT32_MAX ||
        !psp_memory_is_valid(ge->memory, (uint32_t)address, bpp))
        return;
    if (bpp == 4u) {
        psp_memory_write32(ge->memory, (uint32_t)address, color);
        return;
    }
    uint32_t packed;
    if (ge->framebuffer_format == 0u)
        packed = (((color >> 19) & 31u) << 11) |
                 (((color >> 10) & 63u) << 5) | ((color >> 3) & 31u);
    else if (ge->framebuffer_format == 1u)
        packed = (((color >> 19) & 31u) << 11) |
                 (((color >> 11) & 31u) << 6) |
                 (((color >> 3) & 31u) << 1) | (color >> 31);
    else
        packed = (((color >> 28) & 15u) << 12) |
                 (((color >> 20) & 15u) << 8) |
                 (((color >> 12) & 15u) << 4) | ((color >> 4) & 15u);
    psp_memory_write8(ge->memory, (uint32_t)address, (uint8_t)packed);
    psp_memory_write8(ge->memory, (uint32_t)address + 1u,
                      (uint8_t)(packed >> 8));
}

void psp_ge_present_framebuffer(PSP_GE *ge)
{
    if (ge == NULL || !ge->initialized || ge->memory == NULL ||
        ge->video == NULL || ge->display_stride < PSP_VIDEO_WIDTH ||
        ge->display_format > 3u)
        return;
    if (ge->display_address == 0u) {
        psp_video_clear(ge->video, 0xFF000000u);
        return;
    }

    const uint32_t bpp = ge->display_format == 3u ? 4u : 2u;
    /* Validate the complete visible rectangle before touching the host
       framebuffer. This prevents partially presented frames on bad FBP/FBW. */
    uint64_t last_pixel = (uint64_t)(PSP_VIDEO_HEIGHT - 1u) *
        ge->framebuffer_stride + (PSP_VIDEO_WIDTH - 1u);
    uint64_t byte_span = (last_pixel + 1u) * bpp;
    if (byte_span > UINT32_MAX ||
        (uint64_t)ge->framebuffer_address + byte_span > UINT32_MAX + 1ull ||
        !psp_memory_is_valid(ge->memory, ge->framebuffer_address,
                             (uint32_t)byte_span))
        return;

    for (uint32_t y = 0; y < PSP_VIDEO_HEIGHT; y++) {
        uint64_t row_address = (uint64_t)ge->framebuffer_address +
            (uint64_t)y * ge->framebuffer_stride * bpp;
        for (uint32_t x = 0; x < PSP_VIDEO_WIDTH; x++) {
            uint32_t address = (uint32_t)(row_address + (uint64_t)x * bpp);
            uint32_t color;
            if (bpp == 4u) {
                color = psp_memory_read32(ge->memory, address);
            } else {
                uint32_t packed = psp_memory_read8(ge->memory, address);
                packed |= (uint32_t)psp_memory_read8(
                    ge->memory, address + 1u) << 8;
                uint32_t red, green, blue, alpha = 0xFFu;
                if (ge->framebuffer_format == 0u) {
                    red = (packed >> 11) & 31u;
                    green = (packed >> 5) & 63u;
                    blue = packed & 31u;
                    red = (red << 3) | (red >> 2);
                    green = (green << 2) | (green >> 4);
                    blue = (blue << 3) | (blue >> 2);
                } else if (ge->framebuffer_format == 1u) {
                    red = (packed >> 11) & 31u;
                    green = (packed >> 6) & 31u;
                    blue = (packed >> 1) & 31u;
                    alpha = (packed & 1u) ? 0xFFu : 0u;
                    red = (red << 3) | (red >> 2);
                    green = (green << 3) | (green >> 2);
                    blue = (blue << 3) | (blue >> 2);
                } else {
                    red = ((packed >> 12) & 15u) * 17u;
                    green = ((packed >> 8) & 15u) * 17u;
                    blue = ((packed >> 4) & 15u) * 17u;
                    alpha = (packed & 15u) * 17u;
                }
                color = (alpha << 24) | (red << 16) |
                        (green << 8) | blue;
            }
            psp_video_put_pixel(ge->video, (int)x, (int)y, color);
        }
    }
}

static void ge_put_pixel(PSP_GE *ge, int x, int y, uint32_t color,
                          float depth)
{
    if (x < ge->scissor_left || x > ge->scissor_right ||
        y < ge->scissor_top || y > ge->scissor_bottom ||
        x < 0 || y < 0 || x >= PSP_VIDEO_WIDTH || y >= PSP_VIDEO_HEIGHT)
        return;
    uint32_t depth_value = depth <= 0.0f ? 0u :
        (depth >= 65535.0f ? 65535u : (uint32_t)(depth + 0.5f));
    uint32_t depth_index = (uint32_t)y * PSP_VIDEO_WIDTH + (uint32_t)x;
    if (ge->clear_mode) {
        uint32_t destination = psp_video_get_pixel(ge->video, x, y);
        uint32_t write_mask = 0;
        if ((ge->clear_mode_mask & 0x100u) == 0)
            write_mask |= 0x00FFFFFFu;
        if ((ge->clear_mode_mask & 0x200u) == 0)
            write_mask |= 0xFF000000u;
        color = (color & write_mask) | (destination & ~write_mask);
        if ((ge->clear_mode_mask & 0x400u) == 0)
            ge->depth_buffer[depth_index] = (uint16_t)depth_value;
        psp_video_put_pixel(ge->video, x, y, color);
        ge_store_framebuffer_pixel(ge, x, y, color);
        return;
    }
    if (ge->color_test_enabled) {
        uint32_t source = color & 0x00FFFFFFu;
        uint32_t reference = ge->color_test_reference & 0x00FFFFFFu;
        uint32_t mask = ge->color_test_mask & 0x00FFFFFFu;
        bool equal = (source & mask) == (reference & mask);
        if ((ge->color_test_function == 0u && !equal) ||
            (ge->color_test_function == 1u && equal))
            return;
    }
    if (ge->alpha_test_enabled) {
        uint32_t alpha = (color >> 24) & ge->alpha_test_mask;
        uint32_t reference = ge->alpha_test_reference & ge->alpha_test_mask;
        if (!ge_compare_value(ge->alpha_test_function, alpha, reference))
            return;
    }
    uint32_t destination = psp_video_get_pixel(ge->video, x, y);
    uint8_t stencil_current = (uint8_t)(destination >> 24);
    if (ge->stencil_test_enabled) {
        uint32_t stencil_value = stencil_current & ge->stencil_mask;
        uint32_t stencil_reference = ge->stencil_reference & 0xFFu;
        if (!ge_compare_value(ge->stencil_test_function,
                              stencil_reference, stencil_value)) {
            uint8_t updated = ge_apply_stencil_operation(
                ge->stencil_fail_operation, stencil_current,
                (uint8_t)ge->stencil_reference);
            uint32_t result = ge_merge_alpha(destination, updated,
                                             ge->alpha_write_mask);
            psp_video_put_pixel(ge->video, x, y, result);
            ge_store_framebuffer_pixel(ge, x, y, result);
            return;
        }
    }
    if (ge->depth_test_enabled) {
        uint32_t current = ge->depth_buffer[depth_index];
        if (!ge_compare_value(ge->depth_function, depth_value, current)) {
            if (ge->stencil_test_enabled) {
                uint8_t updated = ge_apply_stencil_operation(
                    ge->stencil_depth_fail_operation, stencil_current,
                    (uint8_t)ge->stencil_reference);
                uint32_t result = ge_merge_alpha(destination, updated,
                                                 ge->alpha_write_mask);
                psp_video_put_pixel(ge->video, x, y, result);
                ge_store_framebuffer_pixel(ge, x, y, result);
            }
            return;
        }
    }
    uint8_t stencil_result = stencil_current;
    if (ge->stencil_test_enabled) {
        stencil_result = ge_apply_stencil_operation(
            ge->stencil_pass_operation, stencil_current,
            (uint8_t)ge->stencil_reference);
        destination = ge_merge_alpha(destination, stencil_result,
                                     ge->alpha_write_mask);
    }
    if (ge->depth_test_enabled && !ge->depth_write_disabled)
        ge->depth_buffer[depth_index] = (uint16_t)depth_value;
    if (ge->alpha_blend_enabled)
        color = ge_blend_color(ge, color, destination);
    /* Stencil replaces the output alpha; otherwise alpha comes from the
       fragment. The pixel alpha mask preserves masked destination bits. */
    uint8_t alpha_result = ge->stencil_test_enabled ? stencil_result :
        (uint8_t)(color >> 24);
    uint32_t alpha_merged = ge_merge_alpha(destination, alpha_result,
                                           ge->alpha_write_mask);
    color = (color & 0x00FFFFFFu) | (alpha_merged & 0xFF000000u);
    psp_video_put_pixel(ge->video, x, y, color);
    ge_store_framebuffer_pixel(ge, x, y, color);
}

static uint32_t ge_interpolate_color(uint32_t a, uint32_t b,
                                     uint32_t c, float wa,
                                     float wb, float wc);
static uint32_t ge_shade_fragment(PSP_GE *ge, uint32_t color,
                                  float u, float v, float lod);

static uint32_t ge_apply_fog(const PSP_GE *ge, uint32_t color,
                             uint8_t fog_factor)
{
    if (!ge->fog_enabled || ge->clear_mode || fog_factor == 255u)
        return color;
    uint32_t result = color & 0xFF000000u;
    for (unsigned int shift = 0u; shift <= 16u; shift += 8u) {
        uint32_t source = (color >> shift) & 0xFFu;
        uint32_t fog = (ge->fog_color >> shift) & 0xFFu;
        uint32_t channel = (source * fog_factor +
            fog * (255u - fog_factor) + 127u) / 255u;
        result |= channel << shift;
    }
    return result;
}

static void ge_draw_line(PSP_GE *ge, const GE_Vertex *a,
                         const GE_Vertex *b)
{
    /* Liang-Barsky clipping keeps off-screen lines bounded before Bresenham. */
    float dx = b->x - a->x, dy = b->y - a->y;
    float t0 = 0.0f, t1 = 1.0f;
    float left = (float)(ge->scissor_left > 0 ? ge->scissor_left : 0);
    float top = (float)(ge->scissor_top > 0 ? ge->scissor_top : 0);
    float right = (float)(ge->scissor_right < PSP_VIDEO_WIDTH - 1 ?
        ge->scissor_right : PSP_VIDEO_WIDTH - 1);
    float bottom = (float)(ge->scissor_bottom < PSP_VIDEO_HEIGHT - 1 ?
        ge->scissor_bottom : PSP_VIDEO_HEIGHT - 1);
    if (right < left || bottom < top) return;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {a->x - left, right - a->x,
                  a->y - top, bottom - a->y};
    for (unsigned int i = 0; i < 4u; i++) {
        if (fabsf(p[i]) < 0.000001f) {
            if (q[i] < 0.0f) return;
            continue;
        }
        float r = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (r > t1) return;
            if (r > t0) t0 = r;
        } else {
            if (r < t0) return;
            if (r < t1) t1 = r;
        }
    }
    GE_Vertex clipped_a = *a, clipped_b = *b;
    if (t1 < t0) return;
    GE_Vertex *ends[2] = {&clipped_a, &clipped_b};
    float ts[2] = {t0, t1};
    for (unsigned int i = 0; i < 2u; i++) {
        float t = ts[i];
        GE_Vertex *v = ends[i];
        v->x = a->x + dx * t;
        v->y = a->y + dy * t;
        v->z = a->z + (b->z - a->z) * t;
        v->u = a->u + (b->u - a->u) * t;
        v->v = a->v + (b->v - a->v) * t;
        v->reciprocal_w = a->reciprocal_w +
            (b->reciprocal_w - a->reciprocal_w) * t;
        uint32_t color = 0;
        for (unsigned int shift = 0; shift <= 24; shift += 8) {
            float av = (float)((a->color >> shift) & 0xFFu);
            float bv = (float)((b->color >> shift) & 0xFFu);
            float channel = av + (bv - av) * t;
            if (channel < 0.0f) channel = 0.0f;
            if (channel > 255.0f) channel = 255.0f;
            color |= (uint32_t)(channel + 0.5f) << shift;
        }
        v->color = color;
        float fog = (float)a->fog_factor +
            ((float)b->fog_factor - (float)a->fog_factor) * t;
        if (fog < 0.0f) fog = 0.0f;
        if (fog > 255.0f) fog = 255.0f;
        v->fog_factor = (uint8_t)(fog + 0.5f);
    }
    int x0 = (int)lroundf(clipped_a.x), y0 = (int)lroundf(clipped_a.y);
    int x1 = (int)lroundf(clipped_b.x), y1 = (int)lroundf(clipped_b.y);
    int dx_i = x1 >= x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy_i = y1 >= y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx_i + dy_i;
    int total_steps = dx_i > -dy_i ? dx_i : -dy_i;
    int step = 0;
    for (;;) {
        float t = total_steps ? (float)step / (float)total_steps : 0.0f;
        uint32_t color = ge_interpolate_color(
            clipped_a.color, clipped_b.color, clipped_b.color,
            1.0f - t, t, 0.0f);
        float u = clipped_a.u + (clipped_b.u - clipped_a.u) * t;
        float v = clipped_a.v + (clipped_b.v - clipped_a.v) * t;
        float depth = clipped_a.z + (clipped_b.z - clipped_a.z) * t;
        float fog = (float)clipped_a.fog_factor +
            ((float)clipped_b.fog_factor - (float)clipped_a.fog_factor) * t;
        if (fog < 0.0f) fog = 0.0f;
        if (fog > 255.0f) fog = 255.0f;
        uint32_t shaded = ge_shade_fragment(ge, color, u, v, 0.0f);
        ge_put_pixel(ge, x0, y0,
            ge_apply_fog(ge, shaded, (uint8_t)(fog + 0.5f)), depth);
        if (x0 == x1 && y0 == y1) break;
        int twice = 2 * error;
        if (twice >= dy_i) { error += dy_i; x0 += sx; }
        if (twice <= dx_i) { error += dx_i; y0 += sy; }
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

static uint32_t ge_decode_texture_color(uint32_t format, uint32_t packed)
{
    if (format == 0u)
        return PSP_VIDEO_COLOR(((packed >> 11) & 31u) * 255u / 31u,
            ((packed >> 5) & 63u) * 255u / 63u,
            (packed & 31u) * 255u / 31u, 255u);
    if (format == 1u)
        return PSP_VIDEO_COLOR(((packed >> 10) & 31u) * 255u / 31u,
            ((packed >> 5) & 31u) * 255u / 31u,
            (packed & 31u) * 255u / 31u,
            (packed & 0x8000u) ? 255u : 0u);
    if (format == 2u)
        return PSP_VIDEO_COLOR(((packed >> 8) & 15u) * 17u,
            ((packed >> 4) & 15u) * 17u, (packed & 15u) * 17u,
            ((packed >> 12) & 15u) * 17u);
    return packed;
}

static bool ge_texture_sample_nearest(PSP_GE *ge, unsigned int level,
                                         int x, int y, uint32_t *color)
{
    uint32_t width = ge->texture_level_width[level];
    uint32_t height = ge->texture_level_height[level];
    uint32_t buffer_width = ge->texture_level_buffer_width[level];
    uint32_t texture_address = ge->texture_level_address[level];
    if (width == 0u || height == 0u || buffer_width < width)
        return false;
    if (ge->texture_wrap_u == 0u) {
        x %= (int)width;
        if (x < 0) x += (int)width;
    } else {
        if (x < 0) x = 0;
        if (x >= (int)width) x = (int)width - 1;
    }
    if (ge->texture_wrap_v == 0u) {
        y %= (int)height;
        if (y < 0) y += (int)height;
    } else {
        if (y < 0) y = 0;
        if (y >= (int)height) y = (int)height - 1;
    }

    if (ge->texture_format == 4u || ge->texture_format == 5u) {
        uint64_t row_bytes = ge->texture_format == 4u ?
            ((uint64_t)buffer_width + 1u) / 2u : buffer_width;
        uint64_t offset = (uint64_t)(uint32_t)y * row_bytes;
        offset += ge->texture_format == 4u ? (uint32_t)x / 2u :
            (uint32_t)x;
        uint64_t address = (uint64_t)texture_address + offset;
        if (address > UINT32_MAX ||
            !psp_memory_is_valid(ge->memory, (uint32_t)address, 1u))
            return false;
        uint32_t packed = psp_memory_read8(ge->memory, (uint32_t)address);
        uint32_t index = ge->texture_format == 4u ?
            ((packed >> (((uint32_t)x & 1u) * 4u)) & 0xFu) : packed;
        index = ((index >> ge->clut_shift) & ge->clut_mask) +
            (ge->clut_start << 4);
        if (index >= ge->clut_entries_loaded || index >= 1024u)
            return false;
        *color = ge->clut[index];
        return true;
    }

    uint32_t bytes_per_pixel = ge->texture_format == 3u ? 4u : 2u;
    uint64_t offset = ((uint64_t)(uint32_t)y * buffer_width +
        (uint32_t)x) * bytes_per_pixel;
    uint64_t address = (uint64_t)texture_address + offset;
    if (address > UINT32_MAX ||
        !psp_memory_is_valid(ge->memory, (uint32_t)address,
                             bytes_per_pixel))
        return false;
    uint32_t packed = psp_memory_read8(ge->memory, (uint32_t)address) |
        ((uint32_t)psp_memory_read8(ge->memory,
                                    (uint32_t)address + 1u) << 8);
    *color = ge->texture_format == 3u ?
        psp_memory_read32(ge->memory, (uint32_t)address) :
        ge_decode_texture_color(ge->texture_format, packed);
    return true;
}

static bool ge_texture_sample_level(PSP_GE *ge, unsigned int level,
                                    float u, float v, bool linear,
                                    uint32_t *color)
{
    uint32_t width = ge->texture_level_width[level];
    uint32_t height = ge->texture_level_height[level];
    if (width == 0u || height == 0u ||
        ge->texture_level_buffer_width[level] < width)
        return false;
    float scale_u = (float)width / (float)ge->texture_width;
    float scale_v = (float)height / (float)ge->texture_height;
    u *= scale_u;
    v *= scale_v;
    if (!linear) {
        if (ge->texture_wrap_u == 1u) {
            if (u < 0.0f) u = 0.0f;
            if (u > (float)(width - 1u)) u = (float)(width - 1u);
        }
        if (ge->texture_wrap_v == 1u) {
            if (v < 0.0f) v = 0.0f;
            if (v > (float)(height - 1u)) v = (float)(height - 1u);
        }
        if (!isfinite(u) || !isfinite(v) ||
            u < (float)INT32_MIN || u > (float)INT32_MAX ||
            v < (float)INT32_MIN || v > (float)INT32_MAX)
            return false;
        return ge_texture_sample_nearest(ge, level, (int)floorf(u),
                                         (int)floorf(v), color);
    }
    float fx = u - 0.5f, fy = v - 0.5f;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float tx = fx - (float)x0, ty = fy - (float)y0;
    uint32_t c00, c10, c01, c11;
    if (!ge_texture_sample_nearest(ge, level, x0, y0, &c00) ||
        !ge_texture_sample_nearest(ge, level, x0 + 1, y0, &c10) ||
        !ge_texture_sample_nearest(ge, level, x0, y0 + 1, &c01) ||
        !ge_texture_sample_nearest(ge, level, x0 + 1, y0 + 1, &c11))
        return false;
    uint32_t top = ge_interpolate_color(c00, c10, c10, 1.0f - tx, tx, 0.0f);
    uint32_t bottom = ge_interpolate_color(c01, c11, c11, 1.0f - tx, tx, 0.0f);
    *color = ge_interpolate_color(top, bottom, bottom,
                                  1.0f - ty, ty, 0.0f);
    return true;
}

static bool ge_texture_sample(PSP_GE *ge, float u, float v, float lod,
                              uint32_t *color)
{
    if (!ge->texture_enabled || color == NULL ||
        ge->texture_width == 0u || ge->texture_height == 0u ||
        ge->texture_format > 5u ||
        ((ge->texture_format == 4u || ge->texture_format == 5u) &&
         ge->clut_entries_loaded == 0u))
        return false;

    if (!isfinite(lod) || lod < 0.0f) lod = 0.0f;
    bool minifying = lod > 0.0f;
    if (!minifying)
        return ge_texture_sample_level(ge, 0u, u, v,
                                       ge->texture_mag_filter != 0u, color);

    uint32_t filter = ge->texture_min_filter;
    if (filter <= 1u)
        return ge_texture_sample_level(ge, 0u, u, v, filter == 1u, color);

    float max_lod = 0.0f;
    for (unsigned int i = 1u; i < 8u; i++) {
        if (ge->texture_level_width[i] == 0u ||
            ge->texture_level_height[i] == 0u ||
            ge->texture_level_buffer_width[i] <
                ge->texture_level_width[i])
            break;
        max_lod = (float)i;
    }
    if (lod > max_lod) lod = max_lod;
    bool linear_texel = (filter == 3u || filter == 5u);
    bool linear_level = (filter == 4u || filter == 5u);
    if (!linear_level) {
        unsigned int level = (unsigned int)floorf(lod + 0.5f);
        if (level > (unsigned int)max_lod) level = (unsigned int)max_lod;
        return ge_texture_sample_level(ge, level, u, v,
                                       linear_texel, color);
    }

    unsigned int low = (unsigned int)floorf(lod);
    unsigned int high = low < (unsigned int)max_lod ? low + 1u : low;
    float blend = lod - (float)low;
    uint32_t a, b;
    if (!ge_texture_sample_level(ge, low, u, v, linear_texel, &a))
        return false;
    if (high == low) {
        *color = a;
        return true;
    }
    if (!ge_texture_sample_level(ge, high, u, v, linear_texel, &b))
        return false;
    *color = ge_interpolate_color(a, b, b, 1.0f - blend, blend, 0.0f);
    return true;
}

static uint32_t ge_add_color(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    for (unsigned int shift = 0; shift <= 24; shift += 8) {
        uint32_t value = ((a >> shift) & 0xFFu) +
            ((b >> shift) & 0xFFu);
        if (value > 255u) value = 255u;
        result |= value << shift;
    }
    return result;
}

static uint32_t ge_decal_color(uint32_t primary, uint32_t texel,
                               bool texture_alpha)
{
    if (!texture_alpha)
        return (primary & 0xFF000000u) | (texel & 0x00FFFFFFu);
    uint32_t alpha = (texel >> 24) & 0xFFu;
    uint32_t result = primary & 0xFF000000u;
    for (unsigned int shift = 0; shift <= 16; shift += 8) {
        uint32_t src = (texel >> shift) & 0xFFu;
        uint32_t dst = (primary >> shift) & 0xFFu;
        uint32_t value = (src * alpha + dst * (255u - alpha) + 127u) /
            255u;
        result |= value << shift;
    }
    return result;
}

static uint32_t ge_blend_texture_color(const PSP_GE *ge, uint32_t primary,
                                       uint32_t texel)
{
    uint32_t result = primary & 0xFF000000u;
    for (unsigned int shift = 0; shift <= 16; shift += 8) {
        uint32_t factor = (texel >> shift) & 0xFFu;
        uint32_t src = (primary >> shift) & 0xFFu;
        uint32_t env = (ge->texture_environment_color >> shift) & 0xFFu;
        uint32_t value = (src * factor + env * (255u - factor) + 127u) /
            255u;
        result |= value << shift;
    }
    if (ge->texture_color_component) {
        uint32_t alpha = ((primary >> 24) & 0xFFu) *
            ((texel >> 24) & 0xFFu) / 255u;
        result = (result & 0x00FFFFFFu) | (alpha << 24);
    }
    return result;
}

static uint32_t ge_shade_fragment(PSP_GE *ge, uint32_t color,
                                  float u, float v, float lod)
{
    uint32_t texel;
    if (ge->clear_mode || !ge->texture_enabled)
        return color;
    if (!ge_texture_sample(ge, u, v, lod, &texel))
        return 0;

    switch (ge->texture_function) {
        case 0:
            return ge_modulate_color(color, texel);
        case 1:
            return ge_decal_color(color, texel,
                                  ge->texture_color_component);
        case 2:
            return ge_blend_texture_color(ge, color, texel);
        case 3:
            if (ge->texture_color_component)
                return texel;
            return (color & 0xFF000000u) | (texel & 0x00FFFFFFu);
        case 4: {
            uint32_t added = ge_add_color(color, texel);
            if (!ge->texture_color_component)
                added = (added & 0x00FFFFFFu) | (color & 0xFF000000u);
            return added;
        }
        default:
            return ge_modulate_color(color, texel);
    }
}

static GE_Vertex ge_lerp_vertex(const GE_Vertex *a,
                                  const GE_Vertex *b, float t)
{
    GE_Vertex out = *a;
#define GE_LERP_FIELD(field) out.field = a->field + (b->field - a->field) * t
    GE_LERP_FIELD(x); GE_LERP_FIELD(y); GE_LERP_FIELD(z);
    GE_LERP_FIELD(nx); GE_LERP_FIELD(ny); GE_LERP_FIELD(nz);
    GE_LERP_FIELD(view_x); GE_LERP_FIELD(view_y); GE_LERP_FIELD(view_z);
    GE_LERP_FIELD(reciprocal_w);
    GE_LERP_FIELD(clip_x); GE_LERP_FIELD(clip_y);
    GE_LERP_FIELD(clip_z); GE_LERP_FIELD(clip_w);
#undef GE_LERP_FIELD
    float fog = (float)a->fog_factor +
        ((float)b->fog_factor - (float)a->fog_factor) * t;
    if (fog < 0.0f) fog = 0.0f;
    if (fog > 255.0f) fog = 255.0f;
    out.fog_factor = (uint8_t)(fog + 0.5f);
    out.clip_valid = a->clip_valid && b->clip_valid;
    /* Preserve perspective-correct texture attributes across generated
       clip vertices by interpolating u/w and v/w, then restoring u and v. */
    float rw = out.reciprocal_w;
    float uow = a->u * a->reciprocal_w +
        (b->u * b->reciprocal_w - a->u * a->reciprocal_w) * t;
    float vow = a->v * a->reciprocal_w +
        (b->v * b->reciprocal_w - a->v * a->reciprocal_w) * t;
    if (fabsf(rw) > 0.000001f) {
        out.u = uow / rw;
        out.v = vow / rw;
    } else {
        out.u = a->u + (b->u - a->u) * t;
        out.v = a->v + (b->v - a->v) * t;
    }
    uint32_t color = 0;
    for (unsigned int shift = 0; shift <= 24; shift += 8) {
        float av = (float)((a->color >> shift) & 0xFFu);
        float bv = (float)((b->color >> shift) & 0xFFu);
        float value = av + (bv - av) * t;
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        color |= (uint32_t)(value + 0.5f) << shift;
    }
    out.color = color;
    return out;
}

static float ge_clip_distance(const GE_Vertex *v, unsigned int plane,
                              float left, float top, float right, float bottom)
{
    switch (plane) {
        case 0: return v->x - left;
        case 1: return right - v->x;
        case 2: return v->y - top;
        default: return bottom - v->y;
    }
}

static unsigned int ge_clip_polygon(const GE_Vertex *input,
                                    unsigned int input_count,
                                    GE_Vertex *output,
                                    unsigned int plane,
                                    float left, float top,
                                    float right, float bottom)
{
    if (input_count == 0) return 0;
    unsigned int output_count = 0;
    const GE_Vertex *previous = &input[input_count - 1u];
    float previous_distance = ge_clip_distance(
        previous, plane, left, top, right, bottom);
    bool previous_inside = previous_distance >= 0.0f;
    for (unsigned int i = 0; i < input_count; i++) {
        const GE_Vertex *current = &input[i];
        float current_distance = ge_clip_distance(
            current, plane, left, top, right, bottom);
        bool current_inside = current_distance >= 0.0f;
        if (current_inside != previous_inside) {
            float denominator = previous_distance - current_distance;
            if (fabsf(denominator) > 0.000001f && output_count < 12u) {
                float t = previous_distance / denominator;
                output[output_count++] = ge_lerp_vertex(previous, current, t);
            }
        }
        if (current_inside && output_count < 12u)
            output[output_count++] = *current;
        previous = current;
        previous_distance = current_distance;
        previous_inside = current_inside;
    }
    return output_count;
}

static float ge_homogeneous_clip_distance(const GE_Vertex *vertex,
                                             unsigned int plane)
{
    switch (plane) {
        case 0: return vertex->clip_x + vertex->clip_w;
        case 1: return vertex->clip_w - vertex->clip_x;
        case 2: return vertex->clip_y + vertex->clip_w;
        case 3: return vertex->clip_w - vertex->clip_y;
        case 4: return vertex->clip_z + vertex->clip_w;
        default: return vertex->clip_w - vertex->clip_z;
    }
}

static GE_Vertex ge_lerp_clip_vertex(const GE_Vertex *a,
                                     const GE_Vertex *b, float t)
{
    GE_Vertex out = ge_lerp_vertex(a, b, t);
    /* Homogeneous clipping uses the edge parameter before perspective
       division, so ordinary vertex attributes remain linearly interpolated. */
    out.u = a->u + (b->u - a->u) * t;
    out.v = a->v + (b->v - a->v) * t;
    out.clip_x = a->clip_x + (b->clip_x - a->clip_x) * t;
    out.clip_y = a->clip_y + (b->clip_y - a->clip_y) * t;
    out.clip_z = a->clip_z + (b->clip_z - a->clip_z) * t;
    out.clip_w = a->clip_w + (b->clip_w - a->clip_w) * t;
    out.clip_valid = true;
    return out;
}

static unsigned int ge_clip_homogeneous_polygon(
    const GE_Vertex *input, unsigned int input_count,
    GE_Vertex *output, unsigned int plane)
{
    if (input_count == 0u) return 0u;
    unsigned int output_count = 0u;
    const GE_Vertex *previous = &input[input_count - 1u];
    float previous_distance = ge_homogeneous_clip_distance(previous, plane);
    bool previous_inside = previous_distance >= 0.0f;
    for (unsigned int i = 0u; i < input_count; i++) {
        const GE_Vertex *current = &input[i];
        float current_distance = ge_homogeneous_clip_distance(current, plane);
        bool current_inside = current_distance >= 0.0f;
        if (current_inside != previous_inside) {
            float denominator = previous_distance - current_distance;
            if (fabsf(denominator) > 0.000001f && output_count < 12u) {
                float t = previous_distance / denominator;
                output[output_count++] = ge_lerp_clip_vertex(
                    previous, current, t);
            }
        }
        if (current_inside && output_count < 12u)
            output[output_count++] = *current;
        previous = current;
        previous_distance = current_distance;
        previous_inside = current_inside;
    }
    return output_count;
}

static bool ge_project_clipped_vertex(const PSP_GE *ge, GE_Vertex *vertex)
{
    if (!vertex->clip_valid || !isfinite(vertex->clip_w) ||
        vertex->clip_w <= 0.000001f)
        return false;
    float inv_w = 1.0f / vertex->clip_w;
    float nx = vertex->clip_x * inv_w;
    float ny = vertex->clip_y * inv_w;
    float nz = vertex->clip_z * inv_w;
    if (!isfinite(nx) || !isfinite(ny) || !isfinite(nz))
        return false;
    vertex->reciprocal_w = inv_w;
    vertex->x = ge->viewport_center[0] +
        nx * ge->viewport_scale[0] - ge->viewport_offset[0];
    vertex->y = ge->viewport_center[1] +
        ny * ge->viewport_scale[1] - ge->viewport_offset[1];
    vertex->z = ge->viewport_center[2] + nz * ge->viewport_scale[2];
    if (!isfinite(vertex->x) || !isfinite(vertex->y) ||
        !isfinite(vertex->z))
        return false;
    if (vertex->z < 0.0f) vertex->z = 0.0f;
    if (vertex->z > 65535.0f) vertex->z = 65535.0f;
    return true;
}

static void ge_rasterize_triangle(PSP_GE *ge, const GE_Vertex *a,
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
    /* Estimate texture-space derivatives in screen pixels. This gives the
       mip selector a stable per-triangle LOD; perspective-correct UVs are
       still used for the actual fragment lookup. */
    float determinant = (b->x - a->x) * (c->y - a->y) -
        (c->x - a->x) * (b->y - a->y);
    float lod = 0.0f;
    if (fabsf(determinant) > 0.0001f) {
        float du1 = b->u - a->u, du2 = c->u - a->u;
        float dv1 = b->v - a->v, dv2 = c->v - a->v;
        float dudx = (du1 * (c->y - a->y) -
            du2 * (b->y - a->y)) / determinant;
        float dudy = ((b->x - a->x) * du2 -
            (c->x - a->x) * du1) / determinant;
        float dvdx = (dv1 * (c->y - a->y) -
            dv2 * (b->y - a->y)) / determinant;
        float dvdy = ((b->x - a->x) * dv2 -
            (c->x - a->x) * dv1) / determinant;
        float rho_x = sqrtf(dudx * dudx + dvdx * dvdx);
        float rho_y = sqrtf(dudy * dudy + dvdy * dvdy);
        float rho = fmaxf(rho_x, rho_y);
        if (isfinite(rho) && rho > 1.0f)
            lod = log2f(rho);
    }
    if (ge->cull_enabled &&
        ((ge->cull_mode == 0u && area > 0.0f) ||
         (ge->cull_mode == 1u && area < 0.0f)))
        return;
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = ge_edge(b->x, b->y, c->x, c->y, px, py);
            float w1 = ge_edge(c->x, c->y, a->x, a->y, px, py);
            float w2 = ge_edge(a->x, a->y, b->x, b->y, px, py);
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                float wa = w0 / area, wb = w1 / area, wc = w2 / area;
                uint32_t color = ge_interpolate_color(
                    a->color, b->color, c->color, wa, wb, wc);
                float depth = a->z * wa + b->z * wb + c->z * wc;
                float perspective_weight = a->reciprocal_w * wa +
                    b->reciprocal_w * wb + c->reciprocal_w * wc;
                if (!isfinite(perspective_weight) ||
                    fabsf(perspective_weight) < 0.000001f)
                    continue;
                float u = (a->u * a->reciprocal_w * wa +
                    b->u * b->reciprocal_w * wb +
                    c->u * c->reciprocal_w * wc) / perspective_weight;
                float v = (a->v * a->reciprocal_w * wa +
                    b->v * b->reciprocal_w * wb +
                    c->v * c->reciprocal_w * wc) / perspective_weight;
                uint32_t fog = (uint32_t)((float)a->fog_factor * wa +
                    (float)b->fog_factor * wb +
                    (float)c->fog_factor * wc + 0.5f);
                if (fog > 255u) fog = 255u;
                uint32_t shaded = ge_shade_fragment(ge, color, u, v, lod);
                ge_put_pixel(ge, x, y,
                    ge_apply_fog(ge, shaded, (uint8_t)fog), depth);
            }
        }
    }
}

static void ge_draw_triangle(PSP_GE *ge, const GE_Vertex *a,
                             const GE_Vertex *b, const GE_Vertex *c)
{
    GE_Vertex first[12], second[12];
    first[0] = *a; first[1] = *b; first[2] = *c;
    unsigned int count = 3u;
    GE_Vertex *input = first, *output = second;
    if (a->clip_valid && b->clip_valid && c->clip_valid) {
        for (unsigned int plane = 0u; plane < 6u && count; plane++) {
            count = ge_clip_homogeneous_polygon(input, count, output, plane);
            GE_Vertex *swap = input; input = output; output = swap;
        }
        if (count < 3u) return;
        for (unsigned int i = 0u; i < count; i++) {
            if (!ge_project_clipped_vertex(ge, &input[i]))
                return;
            input[i].clip_valid = false;
        }
    }
    float left = (float)(ge->scissor_left > 0 ? ge->scissor_left : 0);
    float top = (float)(ge->scissor_top > 0 ? ge->scissor_top : 0);
    int clip_right = ge->scissor_right < PSP_VIDEO_WIDTH - 1 ?
        ge->scissor_right : PSP_VIDEO_WIDTH - 1;
    int clip_bottom = ge->scissor_bottom < PSP_VIDEO_HEIGHT - 1 ?
        ge->scissor_bottom : PSP_VIDEO_HEIGHT - 1;
    float right = (float)clip_right + 1.0f;
    float bottom = (float)clip_bottom + 1.0f;
    if (clip_right < left || clip_bottom < top) return;
    for (unsigned int plane = 0; plane < 4u && count; plane++) {
        count = ge_clip_polygon(input, count, output, plane,
                                left, top, right, bottom);
        GE_Vertex *swap = input; input = output; output = swap;
    }
    if (count < 3u) return;
    for (unsigned int i = 1u; i + 1u < count; i++)
        ge_rasterize_triangle(ge, &input[0], &input[i], &input[i + 1u]);
}

static bool ge_draw_primitive(PSP_GE *ge, uint32_t primitive,
                              uint32_t count)
{
    if (count == 0 || count > 4096)
        return count == 0;
    if (ge->texture_enabled &&
        (ge->texture_width == 0 || ge->texture_height == 0 ||
         ge->texture_buffer_width < ge->texture_width ||
         ge->texture_format > 5u ||
         ((ge->texture_format == 4u || ge->texture_format == 5u) &&
          ge->clut_entries_loaded == 0u)))
        return false;
    GE_Vertex vertices[3];
    uint32_t drawn = 0;
    if (primitive == 0) {
        for (uint32_t i = 0; i < count; i++) {
            if (!ge_read_primitive_vertex(ge, i, &vertices[0])) return false;
            ge_put_pixel(ge, (int)lroundf(vertices[0].x),
                         (int)lroundf(vertices[0].y),
                         ge_apply_fog(ge,
                             ge_shade_fragment(ge, vertices[0].color,
                                 vertices[0].u, vertices[0].v, 0.0f),
                             vertices[0].fog_factor), vertices[0].z);
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
                        float mix = (tx + ty) * 0.5f;
                        float fog = (float)vertices[0].fog_factor +
                            ((float)vertices[1].fog_factor -
                             (float)vertices[0].fog_factor) * mix;
                        if (fog < 0.0f) fog = 0.0f;
                        if (fog > 255.0f) fog = 255.0f;
                        uint32_t shaded = ge_shade_fragment(
                            ge, vertices[0].color, u, v, 0.0f);
                        ge_put_pixel(ge, x, y,
                            ge_apply_fog(ge, shaded, (uint8_t)(fog + 0.5f)),
                            vertices[0].z + (vertices[1].z - vertices[0].z) * mix);
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


static bool ge_execute_transfer(PSP_GE *ge, bool transfer_32bit)
{
    uint32_t bytes_per_pixel = transfer_32bit ? 4u : 2u;
    uint32_t width = ge->transfer_width;
    uint32_t height = ge->transfer_height;
    /* Upper buffer-width register bits carry the framebuffer address. */
    uint32_t src_width = ge->transfer_source_buffer_width & 0xFFFFu;
    uint32_t dst_width = ge->transfer_destination_buffer_width & 0xFFFFu;

    if (width == 0u || height == 0u || width > 1024u || height > 1024u ||
        src_width == 0u || dst_width == 0u ||
        ge->transfer_source_x > src_width ||
        width > src_width - ge->transfer_source_x ||
        ge->transfer_destination_x > dst_width ||
        width > dst_width - ge->transfer_destination_x)
        return false;

    uint64_t row_bytes64 = (uint64_t)width * bytes_per_pixel;
    if (row_bytes64 > SIZE_MAX)
        return false;
    size_t row_bytes = (size_t)row_bytes64;
    uint8_t *row = malloc(row_bytes);
    if (row == NULL)
        return false;

    bool success = true;
    for (uint32_t y = 0u; y < height; y++) {
        uint64_t src64 = (uint64_t)ge->transfer_source_address +
            ((uint64_t)(ge->transfer_source_y + y) * src_width +
             ge->transfer_source_x) * bytes_per_pixel;
        uint64_t dst64 = (uint64_t)ge->transfer_destination_address +
            ((uint64_t)(ge->transfer_destination_y + y) * dst_width +
             ge->transfer_destination_x) * bytes_per_pixel;
        if (src64 > UINT32_MAX || dst64 > UINT32_MAX ||
            !psp_memory_is_valid(ge->memory, (uint32_t)src64,
                (uint32_t)row_bytes) ||
            !psp_memory_is_valid(ge->memory, (uint32_t)dst64,
                (uint32_t)row_bytes)) {
            success = false;
            break;
        }
        for (size_t i = 0u; i < row_bytes; i++)
            row[i] = psp_memory_read8(ge->memory,
                (uint32_t)src64 + (uint32_t)i);
        for (size_t i = 0u; i < row_bytes; i++)
            psp_memory_write8(ge->memory,
                (uint32_t)dst64 + (uint32_t)i, row[i]);
    }
    free(row);
    return success;
}

bool psp_ge_init(PSP_GE *ge, PSP_Memory *memory, PSP_Video *video)
{
    if (ge == NULL || memory == NULL || video == NULL ||
        memory->ram == NULL || !video->initialized)
        return false;
    memset(ge, 0, sizeof(*ge));
    ge->depth_buffer = calloc(PSP_VIDEO_WIDTH * PSP_VIDEO_HEIGHT,
                              sizeof(*ge->depth_buffer));
    if (ge->depth_buffer == NULL)
        return false;
    ge->memory = memory;
    ge->video = video;
    ge->initialized = true;
    psp_ge_reset(ge);
    return true;
}

void psp_ge_destroy(PSP_GE *ge)
{
    if (ge == NULL)
        return;
    free(ge->depth_buffer);
    memset(ge, 0, sizeof(*ge));
}

void psp_ge_reset(PSP_GE *ge)
{
    if (ge == NULL || !ge->initialized) return;
    ge->base_address = 0;
    ge->framebuffer_address = PSP_VRAM_BASE;
    ge->framebuffer_stride = PSP_VIDEO_STRIDE;
    ge->framebuffer_format = 3u;
    ge->display_address = PSP_VRAM_BASE;
    ge->display_stride = PSP_VIDEO_STRIDE;
    ge->display_format = 3u;
    ge->vertex_address = 0;
    ge->index_address = 0;
    ge->vertex_type = (2u << 7) | (1u << 23);
    ge->draw_color = 0xFFFFFFFFu;
    ge->lighting_enabled = false;
    ge->light0_enabled = false;
    ge->light0_type = 0;
    ge->light0_direction[0] = 0.0f;
    ge->light0_direction[1] = 0.0f;
    ge->light0_direction[2] = -1.0f;
    ge->light0_diffuse = 0x00FFFFFFu;
    ge->ambient_color = 0;
    ge->fog_enabled = false;
    ge->fog_end = 0.0f;
    ge->fog_slope = 0.0f;
    ge->fog_color = 0;
    ge->transfer_source_address = 0u;
    ge->transfer_source_buffer_width = 0u;
    ge->transfer_destination_address = 0u;
    ge->transfer_destination_buffer_width = 0u;
    ge->transfer_source_x = 0u;
    ge->transfer_source_y = 0u;
    ge->transfer_destination_x = 0u;
    ge->transfer_destination_y = 0u;
    ge->transfer_width = 0u;
    ge->transfer_height = 0u;
    memset(ge->light_enabled, 0, sizeof(ge->light_enabled));
    memset(ge->light_type, 0, sizeof(ge->light_type));
    memset(ge->light_direction, 0, sizeof(ge->light_direction));
    memset(ge->light_position, 0, sizeof(ge->light_position));
    memset(ge->light_diffuse, 0, sizeof(ge->light_diffuse));
    memset(ge->light_spot_exponent, 0, sizeof(ge->light_spot_exponent));
    memset(ge->light_spot_cutoff, 0, sizeof(ge->light_spot_cutoff));
    for (unsigned int light = 0; light < 4u; light++) {
        ge->light_attenuation[light][0] = 1.0f;
        ge->light_attenuation[light][1] = 0.0f;
        ge->light_attenuation[light][2] = 0.0f;
    }
    ge->material_specular = 0u;
    ge->material_specular_coefficient = 1.0f;
    for (unsigned int light = 0; light < 4u; light++) {
        ge->light_specular[light] = 0x00FFFFFFu;
        ge->light_direction[light][2] = -1.0f;
        ge->light_diffuse[light] = 0x00FFFFFFu;
    }
    ge->texture_address = 0;
    memset(ge->texture_level_address, 0, sizeof(ge->texture_level_address));
    memset(ge->texture_level_buffer_width, 0, sizeof(ge->texture_level_buffer_width));
    memset(ge->texture_level_width, 0, sizeof(ge->texture_level_width));
    memset(ge->texture_level_height, 0, sizeof(ge->texture_level_height));
    ge->texture_address_low = 0;
    ge->texture_buffer_width_register = 0;
    ge->texture_buffer_width = 0;
    ge->texture_width = 0;
    ge->texture_height = 0;
    ge->texture_format = 3;
    ge->clut_address = 0;
    ge->clut_format = 3;
    ge->clut_shift = 0;
    ge->clut_mask = 0xFFu;
    ge->clut_start = 0;
    ge->clut_entries_loaded = 0;
    memset(ge->clut, 0, sizeof(ge->clut));
    ge->texture_function = 0;
    ge->texture_environment_color = 0xFFFFFFFFu;
    ge->texture_color_component = true;
    ge->texture_wrap_u = 1;
    ge->texture_wrap_v = 1;
    ge->texture_min_filter = 0;
    ge->texture_mag_filter = 0;
    ge->texture_enabled = false;
    ge->alpha_blend_enabled = false;
    ge->clear_mode = false;
    ge->clear_mode_mask = 0;
    ge->alpha_test_enabled = false;
    ge->color_test_enabled = false;
    ge->color_test_function = 0;
    ge->color_test_reference = 0;
    ge->color_test_mask = 0x00FFFFFFu;
    ge->alpha_test_function = 1u;
    ge->alpha_test_reference = 0;
    ge->alpha_test_mask = 0xFFu;
    ge->stencil_test_enabled = false;
    ge->stencil_test_function = 1u;
    ge->stencil_reference = 0u;
    ge->stencil_mask = 0xFFu;
    ge->stencil_fail_operation = 0u;
    ge->stencil_depth_fail_operation = 0u;
    ge->stencil_pass_operation = 0u;
    ge->alpha_write_mask = 0u;
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
    memset(ge->depth_buffer, 0xFF,
           PSP_VIDEO_WIDTH * PSP_VIDEO_HEIGHT * sizeof(*ge->depth_buffer));
    const float identity3x4[12] = {
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0
    };
    const float identity4x4[16] = {
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 1
    };
    memcpy(ge->world_matrix, identity3x4, sizeof(identity3x4));
    memcpy(ge->view_matrix, identity3x4, sizeof(identity3x4));
    memcpy(ge->projection_matrix, identity4x4, sizeof(identity4x4));
    ge->world_matrix_index = 0;
    ge->view_matrix_index = 0;
    ge->projection_matrix_index = 0;
    ge->viewport_scale[0] = 240.0f;
    ge->viewport_scale[1] = -136.0f;
    ge->viewport_scale[2] = 32767.0f;
    ge->viewport_center[0] = 240.0f;
    ge->viewport_center[1] = 136.0f;
    ge->viewport_center[2] = 32767.0f;
    ge->viewport_offset[0] = 0.0f;
    ge->viewport_offset[1] = 0.0f;
    ge->depth_test_enabled = false;
    ge->cull_enabled = false;
    ge->cull_mode = 0;
    ge->depth_write_disabled = false;
    ge->depth_function = 1u;
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
            case GE_CMD_FRAMEBUFPTR:
                ge->framebuffer_address = ge_resolve_address(ge, value);
                break;
            case GE_CMD_FRAMEBUFWIDTH:
                ge->framebuffer_stride = value & 0xFFFFu;
                break;
            case GE_CMD_FRAMEBUFPIXFORMAT:
                ge->framebuffer_format = value & 3u;
                break;
            case GE_CMD_VTYPE:
                ge->vertex_type = value;
                break;
            case GE_CMD_WORLDMATRIXNUMBER:
                ge->world_matrix_index = value & 0xFu;
                break;
            case GE_CMD_WORLDMATRIXDATA:
                if (ge->world_matrix_index < 12u)
                    ge->world_matrix[ge->world_matrix_index++] =
                        ge_decode_float24(value);
                break;
            case GE_CMD_VIEWMATRIXNUMBER:
                ge->view_matrix_index = value & 0xFu;
                break;
            case GE_CMD_VIEWMATRIXDATA:
                if (ge->view_matrix_index < 12u)
                    ge->view_matrix[ge->view_matrix_index++] =
                        ge_decode_float24(value);
                break;
            case GE_CMD_PROJMATRIXNUMBER:
                ge->projection_matrix_index = value & 0xFu;
                break;
            case GE_CMD_PROJMATRIXDATA:
                if (ge->projection_matrix_index < 16u)
                    ge->projection_matrix[ge->projection_matrix_index++] =
                        ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTXSCALE:
                ge->viewport_scale[0] = ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTYSCALE:
                ge->viewport_scale[1] = ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTZSCALE:
                ge->viewport_scale[2] = ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTXCENTER:
                ge->viewport_center[0] = ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTYCENTER:
                ge->viewport_center[1] = ge_decode_float24(value);
                break;
            case GE_CMD_VIEWPORTZCENTER:
                ge->viewport_center[2] = ge_decode_float24(value);
                break;
            case GE_CMD_OFFSETX:
                ge->viewport_offset[0] = (float)(int16_t)(value & 0xFFFFu) /
                    16.0f;
                break;
            case GE_CMD_OFFSETY:
                ge->viewport_offset[1] = (float)(int16_t)(value & 0xFFFFu) /
                    16.0f;
                break;
            case GE_CMD_ZTESTENABLE:
                ge->depth_test_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_STENCILTESTENABLE:
                ge->stencil_test_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_LIGHTINGENABLE:
                ge->lighting_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_FOGENABLE:
                ge->fog_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_FOG1:
                ge->fog_end = ge_decode_float24(value);
                break;
            case GE_CMD_FOG2:
                ge->fog_slope = ge_decode_float24(value);
                break;
            case GE_CMD_FOGCOLOR:
                ge->fog_color = PSP_VIDEO_COLOR(
                    (value >> 16) & 0xFFu, (value >> 8) & 0xFFu,
                    value & 0xFFu, 255u);
                break;
            case GE_CMD_LIGHTENABLE0:
                ge->light0_enabled = (value & 1u) != 0;
                ge->light_enabled[0] = ge->light0_enabled;
                break;
            case GE_CMD_LIGHTENABLE1:
                ge->light_enabled[1] = (value & 1u) != 0;
                break;
            case GE_CMD_LIGHTENABLE2:
                ge->light_enabled[2] = (value & 1u) != 0;
                break;
            case GE_CMD_LIGHTENABLE3:
                ge->light_enabled[3] = (value & 1u) != 0;
                break;
            case GE_CMD_LIGHTTYPE0:
                ge->light0_type = value & 3u;
                ge->light_type[0] = ge->light0_type;
                break;
            case GE_CMD_LIGHTTYPE1:
                ge->light_type[1] = value & 3u;
                break;
            case GE_CMD_LIGHTTYPE2:
                ge->light_type[2] = value & 3u;
                break;
            case GE_CMD_LIGHTTYPE3:
                ge->light_type[3] = value & 3u;
                break;
            case GE_CMD_LX0:
            case GE_CMD_LY0:
            case GE_CMD_LZ0:
            case GE_CMD_LX1:
            case GE_CMD_LY1:
            case GE_CMD_LZ1:
            case GE_CMD_LX2:
            case GE_CMD_LY2:
            case GE_CMD_LZ2:
            case GE_CMD_LX3:
            case GE_CMD_LY3:
            case GE_CMD_LZ3: {
                unsigned int light = (command - GE_CMD_LX0) / 3u;
                unsigned int axis = (command - GE_CMD_LX0) % 3u;
                ge->light_position[light][axis] = ge_decode_float24(value);
                break;
            }
            case GE_CMD_LDX0:
                ge->light0_direction[0] = ge_decode_float24(value);
                break;
            case GE_CMD_LDY0:
                ge->light0_direction[1] = ge_decode_float24(value);
                break;
            case GE_CMD_LDZ0:
                ge->light0_direction[2] = ge_decode_float24(value);
                ge->light_direction[0][2] = ge->light0_direction[2];
                break;
            case GE_CMD_LDX1:
                ge->light_direction[1][0] = ge_decode_float24(value);
                break;
            case GE_CMD_LDY1:
                ge->light_direction[1][1] = ge_decode_float24(value);
                break;
            case GE_CMD_LDZ1:
                ge->light_direction[1][2] = ge_decode_float24(value);
                break;
            case GE_CMD_LDX2:
                ge->light_direction[2][0] = ge_decode_float24(value);
                break;
            case GE_CMD_LDY2:
                ge->light_direction[2][1] = ge_decode_float24(value);
                break;
            case GE_CMD_LDZ2:
                ge->light_direction[2][2] = ge_decode_float24(value);
                break;
            case GE_CMD_LDX3:
                ge->light_direction[3][0] = ge_decode_float24(value);
                break;
            case GE_CMD_LDY3:
                ge->light_direction[3][1] = ge_decode_float24(value);
                break;
            case GE_CMD_LDZ3:
                ge->light_direction[3][2] = ge_decode_float24(value);
                break;
            case GE_CMD_LKA0: case GE_CMD_LKA1:
            case GE_CMD_LKA2: case GE_CMD_LKA3:
            case GE_CMD_LKB0: case GE_CMD_LKB1:
            case GE_CMD_LKB2: case GE_CMD_LKB3:
            case GE_CMD_LKC0: case GE_CMD_LKC1:
            case GE_CMD_LKC2: case GE_CMD_LKC3: {
                unsigned int attenuation_index = command - GE_CMD_LKA0;
                unsigned int light = attenuation_index / 3u;
                unsigned int component = attenuation_index % 3u;
                ge->light_attenuation[light][component] =
                    ge_decode_float24(value);
                break;
            }
            case GE_CMD_LKS0: case GE_CMD_LKS1:
            case GE_CMD_LKS2: case GE_CMD_LKS3:
                ge->light_spot_exponent[command - GE_CMD_LKS0] =
                    ge_decode_float24(value);
                break;
            case GE_CMD_LKO0: case GE_CMD_LKO1:
            case GE_CMD_LKO2: case GE_CMD_LKO3:
                ge->light_spot_cutoff[command - GE_CMD_LKO0] =
                    ge_decode_float24(value);
                break;
            case GE_CMD_LDC0:
                ge->light0_diffuse = value & 0x00FFFFFFu;
                ge->light_diffuse[0] = ge->light0_diffuse;
                break;
            case GE_CMD_LDC1:
                ge->light_diffuse[1] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LDC2:
                ge->light_diffuse[2] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LDC3:
                ge->light_diffuse[3] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LSC0:
                ge->light_specular[0] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LSC1:
                ge->light_specular[1] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LSC2:
                ge->light_specular[2] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_LSC3:
                ge->light_specular[3] = value & 0x00FFFFFFu;
                break;
            case GE_CMD_MATERIALSPECULAR:
                ge->material_specular = value & 0x00FFFFFFu;
                break;
            case GE_CMD_MATERIALSPECULARCOEF:
                ge->material_specular_coefficient = ge_decode_float24(value);
                if (!isfinite(ge->material_specular_coefficient) ||
                    ge->material_specular_coefficient < 0.0f)
                    ge->material_specular_coefficient = 0.0f;
                if (ge->material_specular_coefficient > 128.0f)
                    ge->material_specular_coefficient = 128.0f;
                break;
            case GE_CMD_AMBIENTCOLOR:
                ge->ambient_color = value & 0x00FFFFFFu;
                break;
            case GE_CMD_CULLFACEENABLE:
                ge->cull_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_CULL:
                ge->cull_mode = value & 1u;
                break;
            case GE_CMD_ZTEST:
                ge->depth_function = value & 7u;
                break;
            case GE_CMD_STENCILTEST:
                ge->stencil_test_function = value & 7u;
                ge->stencil_reference = (value >> 8) & 0xFFu;
                ge->stencil_mask = (value >> 16) & 0xFFu;
                break;
            case GE_CMD_STENCILOP:
                ge->stencil_depth_fail_operation = (value >> 16) & 7u;
                ge->stencil_fail_operation = (value >> 8) & 7u;
                ge->stencil_pass_operation = value & 7u;
                break;
            case GE_CMD_PIXELMASKALPHA:
                ge->alpha_write_mask = value & 0xFFu;
                break;
            case GE_CMD_ZWRITEDISABLE:
                ge->depth_write_disabled = (value & 1u) != 0;
                break;
            case GE_CMD_TEXTUREMAPENABLE:
                ge->texture_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_ALPHABLENDENABLE:
                ge->alpha_blend_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_CLEARMODE:
                ge->clear_mode = (value & 1u) != 0;
                ge->clear_mode_mask = value & 0x700u;
                break;
            case GE_CMD_ALPHATESTENABLE:
                ge->alpha_test_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_COLORTESTENABLE:
                ge->color_test_enabled = (value & 1u) != 0;
                break;
            case GE_CMD_COLORTEST:
                ge->color_test_function = value & 3u;
                if (ge->color_test_function > 1u)
                    return false;
                break;
            case GE_CMD_COLORREF:
                ge->color_test_reference = value & 0x00FFFFFFu;
                break;
            case GE_CMD_COLORTESTMASK:
                ge->color_test_mask = value & 0x00FFFFFFu;
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
            case GE_CMD_TRXSBP:
                ge->transfer_source_address =
                    ((ge->transfer_source_buffer_width & 0x000F0000u) << 8) |
                    (value & 0x00FFFFFFu);
                break;
            case GE_CMD_TRXSBW:
                ge->transfer_source_buffer_width = value;
                ge->transfer_source_address =
                    ((value & 0x000F0000u) << 8) |
                    (ge->transfer_source_address & 0x00FFFFFFu);
                break;
            case GE_CMD_TRXDBP:
                ge->transfer_destination_address =
                    ((ge->transfer_destination_buffer_width & 0x000F0000u) << 8) |
                    (value & 0x00FFFFFFu);
                break;
            case GE_CMD_TRXDBW:
                ge->transfer_destination_buffer_width = value;
                ge->transfer_destination_address =
                    ((value & 0x000F0000u) << 8) |
                    (ge->transfer_destination_address & 0x00FFFFFFu);
                break;
            case GE_CMD_TRXSPOS:
                ge->transfer_source_x = value & 0x3FFu;
                ge->transfer_source_y = (value >> 10) & 0x3FFu;
                break;
            case GE_CMD_TRXDPOS:
                ge->transfer_destination_x = value & 0x3FFu;
                ge->transfer_destination_y = (value >> 10) & 0x3FFu;
                break;
            case GE_CMD_TRXSIZE:
                ge->transfer_width = (value & 0x3FFu) + 1u;
                ge->transfer_height = ((value >> 10) & 0x3FFu) + 1u;
                break;
            case GE_CMD_TRXKICK:
                if (!ge_execute_transfer(ge, (value & 1u) != 0u))
                    return false;
                break;
            case GE_CMD_CLUTADDR:
                ge->clut_address = ge_resolve_address(ge, value);
                break;
            case GE_CMD_CLUTADDRUPPER:
                ge->clut_address =
                    ((value & 0x000F0000u) << 8) |
                    (ge->clut_address & 0x00FFFFFFu);
                break;
            case GE_CMD_CLUTFORMAT:
                ge->clut_format = value & 3u;
                ge->clut_shift = (value >> 2) & 0x1Fu;
                ge->clut_mask = (value >> 8) & 0xFFu;
                ge->clut_start = (value >> 16) & 0x1Fu;
                break;
            case GE_CMD_CLUTLOAD: {
                uint32_t entries = (value & 0x3Fu) * 8u;
                uint32_t bytes_per_entry = ge->clut_format == 3u ? 4u : 2u;
                if (entries && (ge->clut_address > UINT32_MAX -
                    entries * bytes_per_entry ||
                    !psp_memory_is_valid(ge->memory, ge->clut_address,
                        entries * bytes_per_entry))) return false;
                for (uint32_t i = 0; i < entries; i++) {
                    uint32_t address = ge->clut_address + i * bytes_per_entry;
                    uint32_t packed = bytes_per_entry == 4u ?
                        psp_memory_read32(ge->memory, address) :
                        (uint32_t)psp_memory_read8(ge->memory, address) |
                        ((uint32_t)psp_memory_read8(ge->memory, address + 1u) << 8);
                    ge->clut[i] = ge_decode_texture_color(ge->clut_format, packed);
                }
                ge->clut_entries_loaded = entries;
                break;
            }
            case GE_CMD_TEXADDR0:
                ge->texture_address_low = value & 0x00FFFFFFu;
                ge->texture_address =
                    ((ge->texture_buffer_width_register & 0x000F0000u) << 8) |
                    ge->texture_address_low;
                ge->texture_level_address[0] = ge->texture_address;
                break;
            case GE_CMD_TEXBUFWIDTH0:
                ge->texture_buffer_width_register = value;
                ge->texture_buffer_width = value & 0x3FFu;
                ge->texture_address =
                    ((value & 0x000F0000u) << 8) |
                    ge->texture_address_low;
                ge->texture_level_buffer_width[0] = ge->texture_buffer_width;
                ge->texture_level_address[0] = ge->texture_address;
                break;
            case GE_CMD_TEXSIZE0: {
                uint32_t width_exp = value & 0xFu;
                uint32_t height_exp = (value >> 8) & 0xFu;
                if (width_exp > 9u || height_exp > 9u)
                    return false;
                ge->texture_width = 1u << width_exp;
                ge->texture_height = 1u << height_exp;
                ge->texture_level_width[0] = ge->texture_width;
                ge->texture_level_height[0] = ge->texture_height;
                break;
            }
            case GE_CMD_TEXFORMAT:
                ge->texture_format = value & 0xFu;
                break;
            case GE_CMD_TEXFUNC:
                ge->texture_function = value & 7u;
                ge->texture_color_component = ((value >> 8) & 1u) != 0;
                if (ge->texture_function > 4u)
                    return false;
                break;
            case GE_CMD_TEXENV:
                ge->texture_environment_color = PSP_VIDEO_COLOR(
                    (value >> 16) & 0xFFu,
                    (value >> 8) & 0xFFu,
                    value & 0xFFu, 255u);
                break;
            case GE_CMD_TEXFILTER:
                ge->texture_min_filter = value & 0xFu;
                ge->texture_mag_filter = (value >> 8) & 0xFu;
                if (ge->texture_min_filter > 5u ||
                    ge->texture_mag_filter > 1u)
                    return false;
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
                if (command >= 0xA0u && command <= 0xA7u) {
                    uint32_t level = command - 0xA0u;
                    ge->texture_level_address[level] =
                        ((ge->texture_level_buffer_width[level] & 0x000F0000u) << 8) |
                        (value & 0x00FFFFFFu);
                    if (level == 0u) {
                        ge->texture_address_low = value & 0x00FFFFFFu;
                        ge->texture_address = ge->texture_level_address[0];
                    }
                } else if (command >= 0xA8u && command <= 0xAFu) {
                    uint32_t level = command - 0xA8u;
                    ge->texture_level_buffer_width[level] = value & 0x3FFu;
                    ge->texture_level_address[level] =
                        ((value & 0x000F0000u) << 8) |
                        (ge->texture_level_address[level] & 0x00FFFFFFu);
                    if (level == 0u) {
                        ge->texture_buffer_width_register = value;
                        ge->texture_buffer_width = value & 0x3FFu;
                        ge->texture_address = ge->texture_level_address[0];
                    }
                } else if (command >= 0xB8u && command <= 0xBFu) {
                    uint32_t level = command - 0xB8u;
                    uint32_t width_exp = value & 0xFu;
                    uint32_t height_exp = (value >> 8) & 0xFu;
                    if (width_exp > 9u || height_exp > 9u) return false;
                    ge->texture_level_width[level] = 1u << width_exp;
                    ge->texture_level_height[level] = 1u << height_exp;
                    if (level == 0u) {
                        ge->texture_width = ge->texture_level_width[0];
                        ge->texture_height = ge->texture_level_height[0];
                    }
                }
                break;
        }
    }
    return false;
}
