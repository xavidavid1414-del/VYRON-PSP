#include "psp_armv6k.h"

#include <limits.h>

#define ARM_COND_AL (0xEu << 28)

static bool emit_word(PSP_ARMv6KEmitter *emitter, uint32_t word)
{
    if (!emitter || !emitter->words || emitter->failed ||
        emitter->count >= emitter->capacity) {
        if (emitter) emitter->failed = true;
        return false;
    }
    emitter->words[emitter->count++] = word;
    return true;
}

void psp_armv6k_emitter_init(PSP_ARMv6KEmitter *emitter,
                             uint32_t *storage, size_t capacity)
{
    if (!emitter) return;
    emitter->words = storage;
    emitter->capacity = capacity;
    emitter->count = 0;
    emitter->failed = storage == NULL && capacity != 0;
}

static uint32_t ror32(uint32_t value, unsigned amount)
{
    amount &= 31u;
    return amount ? (value >> amount) | (value << (32u - amount)) : value;
}

bool psp_armv6k_emit_mov_imm(PSP_ARMv6KEmitter *emitter,
                             uint8_t rd, uint32_t immediate)
{
    if (rd > 15u) return false;
    for (unsigned rotate = 0; rotate < 16u; ++rotate) {
        uint32_t candidate = ror32(immediate, 32u - (rotate * 2u));
        if (candidate <= 0xFFu) {
            uint32_t word = ARM_COND_AL | (1u << 25) | (13u << 21) |
                            ((uint32_t)rd << 12) | (rotate << 8) | candidate;
            return emit_word(emitter, word);
        }
    }
    return false;
}

bool psp_armv6k_emit_dp_reg(PSP_ARMv6KEmitter *emitter, uint8_t opcode,
                            bool set_flags, uint8_t rd, uint8_t rn,
                            uint8_t rm)
{
    if (opcode > 15u || rd > 15u || rn > 15u || rm > 15u)
        return false;
    uint32_t word = ARM_COND_AL | ((uint32_t)opcode << 21) |
                    (set_flags ? (1u << 20) : 0u) |
                    ((uint32_t)rn << 16) | ((uint32_t)rd << 12) | rm;
    return emit_word(emitter, word);
}

bool psp_armv6k_emit_ldr_str(PSP_ARMv6KEmitter *emitter, bool load,
                             uint8_t rd, uint8_t rn, int32_t offset)
{
    if (rd > 15u || rn > 15u || offset < -4095 || offset > 4095)
        return false;
    uint32_t magnitude = (uint32_t)(offset < 0 ? -offset : offset);
    uint32_t word = ARM_COND_AL | (1u << 26) | (1u << 24) |
                    (offset >= 0 ? (1u << 23) : 0u) |
                    (load ? (1u << 20) : 0u) |
                    ((uint32_t)rn << 16) | ((uint32_t)rd << 12) |
                    magnitude;
    return emit_word(emitter, word);
}

bool psp_armv6k_emit_branch(PSP_ARMv6KEmitter *emitter,
                            int32_t displacement_bytes, bool link)
{
    if ((displacement_bytes & 3) != 0 ||
        displacement_bytes < -33554432 || displacement_bytes > 33554428)
        return false;
    uint32_t imm24 = ((uint32_t)(displacement_bytes >> 2)) & 0x00FFFFFFu;
    return emit_word(emitter, ARM_COND_AL | (5u << 25) |
                     (link ? (1u << 24) : 0u) | imm24);
}
