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
    assert(code[1] == 0xE1A03201u); /* MOV r3, r1, LSL #4 */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t srl = (2u << 16) | (3u << 11) | (2u << 6) | 0x02u;
    assert(psp_armv6k_translate_allegrex(&out, srl) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE1A03121u); /* MOV r3, r1, LSR #2 */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sra = (2u << 16) | (3u << 11) | (3u << 6) | 0x03u;
    assert(psp_armv6k_translate_allegrex(&out, sra) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE1A031C1u); /* MOV r3, r1, ASR #3 */

    /* Variable shifts mask the guest shift amount to five bits. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sllv = (1u << 21) | (2u << 16) | (3u << 11) | 0x04u;
    assert(psp_armv6k_translate_allegrex(&out, sllv) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[0] == 0xE5902004u); /* LDR shift amount */
    assert(code[1] == 0xE202201Fu); /* AND shift amount, #31 */
    assert(code[2] == 0xE5901008u); /* LDR value */
    assert(code[3] == 0xE1A03211u); /* MOV r3, r1, LSL r2 */
    assert(code[4] == 0xE580300Cu);

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t srlv = (1u << 21) | (2u << 16) | (3u << 11) | 0x06u;
    assert(psp_armv6k_translate_allegrex(&out, srlv) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[3] == 0xE1A03231u); /* MOV r3, r1, LSR r2 */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t srav = (1u << 21) | (2u << 16) | (3u << 11) | 0x07u;
    assert(psp_armv6k_translate_allegrex(&out, srav) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[3] == 0xE1A03251u); /* MOV r3, r1, ASR r2 */

    /* MULT/MULTU calculate both halves of the product into HI:LO. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t mult = (1u << 21) | (2u << 16) | 0x18u;
    assert(psp_armv6k_translate_allegrex(&out, mult) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[0] == 0xE5901004u); /* LDR rs */
    assert(code[1] == 0xE5902008u); /* LDR rt */
    assert(code[2] == 0xE0C43291u); /* SMULL r4, r3, r2, r1 */
    assert(code[3] == 0xE5803088u); /* STR LO */
    assert(code[4] == 0xE5804084u); /* STR HI */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t multu = (1u << 21) | (2u << 16) | 0x19u;
    assert(psp_armv6k_translate_allegrex(&out, multu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[2] == 0xE0843291u); /* UMULL r4, r3, r2, r1 */

    /* MFHI/MFLO copy the CPU special register into a GPR. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t mfhi = (4u << 11) | 0x10u;
    assert(psp_armv6k_translate_allegrex(&out, mfhi) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 2);
    assert(code[0] == 0xE5903084u); /* LDR r3, [cpu, #hi] */
    assert(code[1] == 0xE5803010u); /* STR r3, [cpu, #gpr[4]] */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t mflo = (4u << 11) | 0x12u;
    assert(psp_armv6k_translate_allegrex(&out, mflo) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[0] == 0xE5903088u); /* LDR r3, [cpu, #lo] */

    /* MTHI/MTLO copy a GPR into the CPU special register. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t mthi = (5u << 21) | 0x11u;
    assert(psp_armv6k_translate_allegrex(&out, mthi) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 2);
    assert(code[0] == 0xE5901014u); /* LDR rs */
    assert(code[1] == 0xE5801084u); /* STR hi */

    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t mtlo = (5u << 21) | 0x13u;
    assert(psp_armv6k_translate_allegrex(&out, mtlo) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE5801088u); /* STR lo */

    /* MOVZ rd, rs, rt: update rd only when rt equals zero. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t movz = (1u << 21) | (2u << 16) | (3u << 11) | 0x0Au;
    assert(psp_armv6k_translate_allegrex(&out, movz) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 6);
    assert(code[0] == 0xE590300Cu); /* LDR old rd */
    assert(code[1] == 0xE5902008u); /* LDR condition register */
    assert(code[2] == 0xE3520000u); /* CMP rt, #0 */
    assert(code[3] == 0xE5901004u); /* LDR source */
    assert(code[4] == 0x01A03001u); /* MOVEQ rd, rs */
    assert(code[5] == 0xE580300Cu); /* STR rd */

    /* MOVN rd, rs, rt: update rd only when rt is nonzero. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t movn = (1u << 21) | (2u << 16) | (3u << 11) | 0x0Bu;
    assert(psp_armv6k_translate_allegrex(&out, movn) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[4] == 0x11A03001u); /* MOVNE rd, rs */

    /* A zero shift amount is a MIPS no-op for SRL/SRA. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t srl_zero = (2u << 16) | (3u << 11) | 0x02u;
    assert(psp_armv6k_translate_allegrex(&out, srl_zero) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[1] == 0xE1A03001u); /* MOV r3, r1 */

    /* SLT r3, r1, r2: signed comparison. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t slt = (1u << 21) | (2u << 16) | (3u << 11) | 0x2Au;
    assert(psp_armv6k_translate_allegrex(&out, slt) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 6);
    assert(code[2] == 0xE1510002u); /* CMP r1, r2 */
    assert(code[3] == 0xB1A03001u); /* MOVLT r3, #1 */
    assert(code[4] == 0xA1A03000u); /* MOVGE r3, #0 */

    /* SLTU r3, r1, r2: unsigned comparison. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sltu = (1u << 21) | (2u << 16) | (3u << 11) | 0x2Bu;
    assert(psp_armv6k_translate_allegrex(&out, sltu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[3] == 0x31A03001u); /* MOVLO r3, #1 */
    assert(code[4] == 0x21A03000u); /* MOVHS r3, #0 */

    /* SLTI r3, r1, 7. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t slti = (0x0Au << 26) | (1u << 21) | (3u << 16) | 7u;
    assert(psp_armv6k_translate_allegrex(&out, slti) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[1] == 0xE3510007u); /* CMP r1, #7 */
    assert(code[2] == 0xB1A03001u); /* MOVLT r3, #1 */
    assert(code[3] == 0xA1A03000u); /* MOVGE r3, #0 */

    /* SLTIU compares against the sign-extended immediate as unsigned. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t sltiu = (0x0Bu << 26) | (1u << 21) | (3u << 16);
    assert(psp_armv6k_translate_allegrex(&out, sltiu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(code[2] == 0x31A03001u); /* MOVLO r3, #1 */
    assert(code[3] == 0x21A03000u); /* MOVHS r3, #0 */

    /* SLTI synthesizes a full-width sign-extended immediate. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t slti_negative = (0x0Au << 26) | (1u << 21) |
                             (3u << 16) | 0xFFFCu;
    assert(psp_armv6k_translate_allegrex(&out, slti_negative) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 9);
    assert(code[5] == 0xE1510002u); /* CMP r1, r2 */

    /* ADDIU r2, r1, -4. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addiu = (9u << 26) | (1u << 21) | (2u << 16) | 0xFFFCu;
    assert(psp_armv6k_translate_allegrex(&out, addiu) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE2413004u); /* SUB r3, r1, #4 */

    /* ADDIU with a wide positive immediate synthesizes the constant. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addiu_wide = (9u << 26) | (1u << 21) | (2u << 16) | 0x1234u;
    assert(psp_armv6k_translate_allegrex(&out, addiu_wide) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[3] == 0xE0813002u); /* ADD r3, r1, r2 */

    /* Negative wide ADDIU uses SUB with the synthesized magnitude. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t addiu_negative_wide =
        (9u << 26) | (1u << 21) | (2u << 16) | 0xEDCCu;
    assert(psp_armv6k_translate_allegrex(&out, addiu_negative_wide) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[3] == 0xE0413002u); /* SUB r3, r1, r2 */

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

    /* A full 16-bit ANDI immediate is synthesized from two ARM immediates. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t andi_wide = (12u << 26) | (3u << 21) | (4u << 16) | 0x1234u;
    assert(psp_armv6k_translate_allegrex(&out, andi_wide) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[3] == 0xE0013002u); /* AND r3, r1, r2 */

    /* LUI r4, 0x1234 constructs 0x12340000. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t lui = (15u << 26) | (4u << 16) | 0x1234u;
    assert(psp_armv6k_translate_allegrex(&out, lui) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[0] == 0xE3A03412u); /* MOV r3, #0x12000000 */
    assert(code[1] == 0xE383370Du); /* ORR r3, r3, #0x00340000 */
    assert(code[2] == 0xE5803010u); /* STR r3, [r0, #16] */

    /* Unsupported branches must be handed to the interpreter. */
    assert(psp_armv6k_translate_allegrex(&out, 0x10000001u) ==
           PSP_ARMV6K_TRANSLATE_UNSUPPORTED);

    psp_armv6k_emitter_init(&out, code, 1);
    assert(psp_armv6k_translate_allegrex(&out, addu) ==
           PSP_ARMV6K_TRANSLATE_ERROR);
    assert(out.failed);
    /* Allegrex ROTR maps to ARM ROR with an immediate amount. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t rotr = (1u << 21) | (2u << 16) | (3u << 11) |
                    (8u << 6) | 0x02u;
    assert(psp_armv6k_translate_allegrex(&out, rotr) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 3);
    assert(code[1] == 0xE1A03461u); /* MOV r3, r1, ROR #8 */

    /* Allegrex ROTRV maps to ARM ROR with a register amount. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t rotrv = (1u << 21) | (2u << 16) | (3u << 11) |
                     (1u << 6) | 0x06u;
    assert(psp_armv6k_translate_allegrex(&out, rotrv) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 5);
    assert(code[3] == 0xE1A03271u); /* MOV r3, r1, ROR r2 */

    /* Allegrex MAX emits a signed conditional select sequence. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t max_inst = (1u << 21) | (2u << 16) |
                        (3u << 11) | 0x2Cu;
    assert(psp_armv6k_translate_allegrex(&out, max_inst) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 7);

    /* Allegrex MIN uses the complementary signed condition. */
    psp_armv6k_emitter_init(&out, code, 16);
    uint32_t min_inst = (1u << 21) | (2u << 16) |
                        (3u << 11) | 0x2Du;
    assert(psp_armv6k_translate_allegrex(&out, min_inst) ==
           PSP_ARMV6K_TRANSLATE_OK);
    assert(out.count == 7);

    return 0;
}
