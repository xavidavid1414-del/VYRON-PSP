#include "psp_armv6k.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    uint32_t code[8] = {0};
    PSP_ARMv6KEmitter out;
    psp_armv6k_emitter_init(&out, code, 8);

    assert(psp_armv6k_emit_mov_imm(&out, 0, 1));
    assert(code[0] == 0xE3A00001u); /* MOV r0, #1 */
    assert(psp_armv6k_emit_dp_reg(&out, 4, false, 1, 1, 2));
    assert(code[1] == 0xE0811002u); /* ADD r1, r1, r2 */
    assert(psp_armv6k_emit_ldr_str(&out, true, 3, 4, 12));
    assert(code[2] == 0xE594300Cu); /* LDR r3, [r4, #12] */
    assert(psp_armv6k_emit_ldr_str(&out, false, 3, 4, -8));
    assert(code[3] == 0xE5043008u); /* STR r3, [r4, #-8] */
    assert(psp_armv6k_emit_branch(&out, 16, true));
    assert(code[4] == 0xEB000004u); /* BL +16 */

    assert(!psp_armv6k_emit_mov_imm(&out, 0, 0x12345678u));
    assert(!psp_armv6k_emit_dp_reg(&out, 16, false, 0, 0, 0));
    assert(!psp_armv6k_emit_ldr_str(&out, true, 0, 0, 4096));
    assert(!psp_armv6k_emit_branch(&out, 2, false));

    /* Capacity exhaustion is sticky and never writes out of bounds. */
    assert(psp_armv6k_emit_mov_imm(&out, 5, 7));
    assert(psp_armv6k_emit_mov_imm(&out, 6, 8));
    assert(psp_armv6k_emit_mov_imm(&out, 7, 9));
    assert(out.count == 8);
    assert(!psp_armv6k_emit_mov_imm(&out, 8, 10));
    assert(out.count == 8);
    assert(out.failed);
    return 0;
}
