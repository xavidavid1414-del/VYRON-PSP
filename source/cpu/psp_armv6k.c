#include "psp_armv6k.h"
#include "psp_cpu.h"

#include <stddef.h>

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

static bool encode_imm8(uint32_t value, uint32_t *operand)
{
    for (unsigned rotate = 0; rotate < 16u; ++rotate) {
        uint32_t candidate = ror32(value, 32u - rotate * 2u);
        if (candidate <= 0xFFu) {
            *operand = (rotate << 8) | candidate;
            return true;
        }
    }
    return false;
}

bool psp_armv6k_emit_mov_imm(PSP_ARMv6KEmitter *emitter,
                             uint8_t rd, uint32_t immediate)
{
    return psp_armv6k_emit_dp_imm(emitter, 13, false, rd, 0, immediate);
}

bool psp_armv6k_emit_dp_imm(PSP_ARMv6KEmitter *emitter, uint8_t opcode,
                            bool set_flags, uint8_t rd, uint8_t rn,
                            uint32_t immediate)
{
    uint32_t operand;
    if (opcode > 15u || rd > 15u || rn > 15u ||
        !encode_imm8(immediate, &operand))
        return false;
    uint32_t word = ARM_COND_AL | (1u << 25) |
                    ((uint32_t)opcode << 21) |
                    (set_flags ? (1u << 20) : 0u) |
                    ((uint32_t)rn << 16) | ((uint32_t)rd << 12) | operand;
    return emit_word(emitter, word);
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

static bool emit_load_gpr(PSP_ARMv6KEmitter *emitter, uint8_t arm_reg,
                          uint8_t mips_reg)
{
    if (mips_reg == 0u)
        return psp_armv6k_emit_mov_imm(emitter, arm_reg, 0);
    return psp_armv6k_emit_ldr_str(
        emitter, true, arm_reg, 0,
        (int32_t)(offsetof(PSP_CPU, gpr) + (size_t)mips_reg * sizeof(uint32_t)));
}

static bool emit_store_gpr(PSP_ARMv6KEmitter *emitter, uint8_t mips_reg,
                           uint8_t arm_reg)
{
    if (mips_reg == 0u) return true;
    return psp_armv6k_emit_ldr_str(
        emitter, false, arm_reg, 0,
        (int32_t)(offsetof(PSP_CPU, gpr) + (size_t)mips_reg * sizeof(uint32_t)));
}

PSP_ARMv6KTranslateResult psp_armv6k_translate_allegrex(
    PSP_ARMv6KEmitter *emitter, uint32_t instruction)
{
    if (!emitter || !emitter->words || emitter->failed)
        return PSP_ARMV6K_TRANSLATE_ERROR;

    uint8_t rs = (uint8_t)((instruction >> 21) & 31u);
    uint8_t rt = (uint8_t)((instruction >> 16) & 31u);
    uint8_t rd = (uint8_t)((instruction >> 11) & 31u);
    uint8_t opcode = (uint8_t)(instruction >> 26);
    uint8_t function = (uint8_t)(instruction & 63u);
    uint8_t arm_opcode;

    if (opcode == 0u) {
        if (function == 0x00u || function == 0x02u || function == 0x03u) {
            uint8_t sa = (uint8_t)((instruction >> 6) & 31u);
            uint8_t shift_type = function == 0x00u ? 0u :
                                 (function == 0x02u ? 1u : 2u);
            if (!emit_load_gpr(emitter, 1, rt) ||
                !emit_shift_imm(emitter, 3, 1, shift_type, sa) ||
                !emit_store_gpr(emitter, rd, 3))
                return PSP_ARMV6K_TRANSLATE_ERROR;
            return PSP_ARMV6K_TRANSLATE_OK;
        }
        switch (function) {
        case 0x21: arm_opcode = 4; break;  /* ADDU */
        case 0x23: arm_opcode = 2; break;  /* SUBU */
        case 0x24: arm_opcode = 0; break;  /* AND */
        case 0x25: arm_opcode = 12; break; /* OR */
        case 0x26: arm_opcode = 1; break;  /* XOR */
        case 0x27: arm_opcode = 15; break; /* NOR */
        default: return PSP_ARMV6K_TRANSLATE_UNSUPPORTED;
        }
        if (!emit_load_gpr(emitter, 1, rs) ||
            !emit_load_gpr(emitter, 2, rt) ||
            !psp_armv6k_emit_dp_reg(emitter, arm_opcode, false, 3, 1, 2) ||
            !emit_store_gpr(emitter, rd, 3))
            return PSP_ARMV6K_TRANSLATE_ERROR;
        return PSP_ARMV6K_TRANSLATE_OK;
    }

    if (opcode == 12u || opcode == 13u || opcode == 14u) {
        /* ANDI/ORI/XORI use a zero-extended 16-bit immediate. */
        uint32_t immediate = instruction & 0xFFFFu;
        arm_opcode = opcode == 12u ? 0u : (opcode == 13u ? 12u : 1u);
        if (!encode_imm8(immediate, &(uint32_t){0}))
            return PSP_ARMV6K_TRANSLATE_UNSUPPORTED;
        if (!emit_load_gpr(emitter, 1, rs) ||
            !psp_armv6k_emit_dp_imm(emitter, arm_opcode, false, 3, 1, immediate) ||
            !emit_store_gpr(emitter, rt, 3))
            return PSP_ARMV6K_TRANSLATE_ERROR;
        return PSP_ARMV6K_TRANSLATE_OK;
    }

    if (opcode == 9u) { /* ADDIU: immediate must fit ARM's rotated imm8. */
        int32_t immediate = (int16_t)(instruction & 0xFFFFu);
        uint8_t arm_op = immediate < 0 ? 2u : 4u;
        uint32_t magnitude = (uint32_t)(immediate < 0 ? -immediate : immediate);
        if (!encode_imm8(magnitude, &(uint32_t){0}))
            return PSP_ARMV6K_TRANSLATE_UNSUPPORTED;
        if (!emit_load_gpr(emitter, 1, rs) ||
            !psp_armv6k_emit_dp_imm(emitter, arm_op, false, 3, 1, magnitude) ||
            !emit_store_gpr(emitter, rt, 3))
            return PSP_ARMV6K_TRANSLATE_ERROR;
        return PSP_ARMV6K_TRANSLATE_OK;
    }

    return PSP_ARMV6K_TRANSLATE_UNSUPPORTED;
}
