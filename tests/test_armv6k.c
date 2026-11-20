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

    /* SLL r3, r2, 4; SRL and SRA share ARM shift encoding. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sll = (2u << 16) | (3u << 11) | (4u << 6);
    assert(psp_armv6k_translate_allegrex(&out, sll) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE1A03202u); /* MOV r3, r2, LSL #4 */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t srl = (2u << 16) | (3u << 11) | (2u << 6) | 0x02u;
    assert(psp_armv6k_translate_allegrex(&out, srl) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE1A03122u); /* MOV r3, r2, LSR #2 */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sra = (2u << 16) | (3u << 11) | (3u << 6) | 0x03u;
    assert(psp_armv6k_translate_allegrex(&out, sra) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE1A031C2u); /* MOV r3, r2, ASR #3 */

    /* SLT r3, r1, r2: signed comparison. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t slt = (1u << 21) | (2u << 16) | (3u << 11) | 0x2Au;
    assert(psp_armv6k_translate_allegrex(&out, slt) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 6);
    assert(code[2] == 0xE1510002u); /* CMP r1, r2 */
    assert(code[3] == 0xB3A03001u); /* MOVLT r3, #1 */
    assert(code[4] == 0xA3A03000u); /* MOVGE r3, #0 */

    /* SLTU r3, r1, r2: unsigned comparison. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sltu = (1u << 21) | (2u << 16) | (3u << 11) | 0x2Bu;
    assert(psp_armv6k_translate_allegrex(&out, sltu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[3] == 0x33A03001u); /* MOVLO r3, #1 */
    assert(code[4] == 0x23A03000u); /* MOVHS r3, #0 */

    /* ADDIU r2, r1, -4. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addiu = (9u << 26) | (1u << 21) | (2u << 16) | 0xFFFCu;
    assert(psp_armv6k_translate_allegrex(&out, addiu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE2413004u); /* SUB r3, r1, #4 */

    /* ANDI r4, r3, 0x0F. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t andi = (12u << 26) | (3u << 21) | (4u << 16) | 0x000Fu;
    assert(psp_armv6k_translate_allegrex(&out, andi) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE201300Fu); /* AND r3, r1, #15 */

    /* ORI r4, r3, 0x80. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t ori = (13u << 26) | (3u << 21) | (4u << 16) | 0x0080u;
    assert(psp_armv6k_translate_allegrex(&out, ori) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE3813080u); /* ORR r3, r1, #128 */

    /* An immediate not representable in ARM's rotated imm8 falls back. */
    uint32_t andi_wide = (12u << 26) | (3u << 21) | (4u << 16) | 0x1234u;
    assert(psp_armv6k_translate_allegrex(&out, andi_wide) ==
           PSP_ARMV6K_TRANSLATE_UNSUPPORTED);

    /* Unsupported branches must be handed to the interpreter. */
    assert(psp_armv6k_translate_allegrex(&out, 0x10000001u) ==
           PSP_ARMV6K_TRANSLATE_UNSUPPORTED);

    psp_armv6k_emitter_init(&out, code, 1);
    assert(psp_armv6k_translate_allegrex(&out, addu) ==
           PSP_ARMV6K_TRANSLATE_ERROR);
    assert(out.failed);
    return 0;
}
