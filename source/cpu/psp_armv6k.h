#ifndef PSP_ARMV6K_H
#define PSP_ARMV6K_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *words;
    size_t capacity;
    size_t count;
    bool failed;
} PSP_ARMv6KEmitter;

typedef enum {
    PSP_ARMV6K_TRANSLATE_OK = 0,
    PSP_ARMV6K_TRANSLATE_UNSUPPORTED,
    PSP_ARMV6K_TRANSLATE_ERROR
} PSP_ARMv6KTranslateResult;

void psp_armv6k_emitter_init(PSP_ARMv6KEmitter *emitter,
                             uint32_t *storage, size_t capacity);
/* Wrap a translated block as an AAPCS-callable function taking PSP_CPU* in r0. */
bool psp_armv6k_emit_function_prologue(PSP_ARMv6KEmitter *emitter);
bool psp_armv6k_emit_function_epilogue(PSP_ARMv6KEmitter *emitter);
bool psp_armv6k_emit_mov_imm(PSP_ARMv6KEmitter *emitter,
                             uint8_t rd, uint32_t immediate);
bool psp_armv6k_emit_dp_reg(PSP_ARMv6KEmitter *emitter, uint8_t opcode,
                            bool set_flags, uint8_t rd, uint8_t rn,
                            uint8_t rm);
bool psp_armv6k_emit_dp_imm(PSP_ARMv6KEmitter *emitter, uint8_t opcode,
                            bool set_flags, uint8_t rd, uint8_t rn,
                            uint32_t immediate);
bool psp_armv6k_emit_ldr_str(PSP_ARMv6KEmitter *emitter, bool load,
                             uint8_t rd, uint8_t rn, int32_t offset);
bool psp_armv6k_emit_branch(PSP_ARMv6KEmitter *emitter,
                            int32_t displacement_bytes, bool link);

/*
 * Emit ARM-state code for a limited, side-effect-safe Allegrex subset.
 * R0 is the pointer to PSP_CPU. The caller must provide an executable-memory
 * strategy and preserve the platform ABI before executing emitted words.
 * Branches, exceptions, memory instructions and unsupported opcodes are
 * deliberately rejected so the caller can use the reference interpreter.
 */
PSP_ARMv6KTranslateResult psp_armv6k_translate_allegrex(
    PSP_ARMv6KEmitter *emitter, uint32_t instruction);

#endif
