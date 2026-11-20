#ifndef PSP_ARMV6K_H
#define PSP_ARMV6K_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Portable ARMv6K instruction encoder.
 * It writes ARM-state words into caller-owned storage; it does not allocate
 * executable memory or execute generated code.
 */
typedef struct {
    uint32_t *words;
    size_t capacity;
    size_t count;
    bool failed;
} PSP_ARMv6KEmitter;

void psp_armv6k_emitter_init(PSP_ARMv6KEmitter *emitter,
                             uint32_t *storage, size_t capacity);
bool psp_armv6k_emit_mov_imm(PSP_ARMv6KEmitter *emitter,
                             uint8_t rd, uint32_t immediate);
bool psp_armv6k_emit_dp_reg(PSP_ARMv6KEmitter *emitter, uint8_t opcode,
                            bool set_flags, uint8_t rd, uint8_t rn,
                            uint8_t rm);
bool psp_armv6k_emit_ldr_str(PSP_ARMv6KEmitter *emitter, bool load,
                             uint8_t rd, uint8_t rn, int32_t offset);
bool psp_armv6k_emit_branch(PSP_ARMv6KEmitter *emitter,
                            int32_t displacement_bytes, bool link);

#endif
