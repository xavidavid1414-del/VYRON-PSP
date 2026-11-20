#include "psp_armv6k.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    uint32_t code[16] = {0};
    PSP_ARMv6KEmitter out;
    psp_armv6k_emitter_init(&out, code, 16);

    assert(psp_armv6k_emit_mov_imm(&out, 0, 1));
    assert(code[0] == 0xE3A00001u);
    assert(psp_armv6k_emit_dp_reg(&out, 4, false, 1, 1, 2));
    assert(code[1] == 0xE0811002u);
    assert(psp_armv6k_emit_ldr_str(&out, true, 3, 4, 12));
    assert(code[2] == 0xE594300Cu);
    assert(psp_armv6k_emit_ldr_str(&out, false, 3, 4, -8));
    assert(code[3] == 0xE5043008u);
    assert(psp_armv6k_emit_branch(&out, 16, true));
    assert(code[4] == 0xEB000004u);
    assert(psp_armv6k_emit_dp_imm(&out, 4, false, 3, 1, 7));
    assert(code[5] == 0xE2813007u);

    /* ADDU r3, r1, r2: load guest registers, add, store guest result. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addu = (1u << 21) | (2u << 16) | (3u << 11) | 0x21u;
    assert(psp_armv6k_translate_allegrex(&out, addu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 4);
    assert(code[0] == 0xE5901004u); /* LDR r1, [r0, #4] */
    assert(code[1] == 0xE5902008u); /* LDR r2, [r0, #8] */
    assert(code[2] == 0xE0813002u); /* ADD r3, r1, r2 */
    assert(code[3] == 0xE580300Cu); /* STR r3, [r0, #12] */

    /* ADDIU r2, r1, -4. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addiu = (9u << 26) | (1u << 21) | (2u << 16) | 0xFFFCu;
    assert(psp_armv6k_translate_allegrex(&out, addiu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE2413004u); /* SUB r3, r1, #4 */

    /* Unsupported branches must be handed to the interpreter. */
    assert(psp_armv6k_translate_allegrex(&out, 0x10000001u) ==
           PSP_ARMV6K_TRANSLATE_UNSUPPORTED);

    psp_armv6k_emitter_init(&out, code, 1);
    assert(psp_armv6k_translate_allegrex(&out, addu) ==
           PSP_ARMV6K_TRANSLATE_ERROR);
    assert(out.failed);
    return 0;
}
