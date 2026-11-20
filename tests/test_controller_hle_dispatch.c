#include "psp_controller_hle.h"
#include "../cpu/psp_syscall.h"
#include "../cpu/psp_interpreter.h"
#include "../loader/psp_nid.h"
#include <assert.h>
#include <string.h>
int main(void) {
    PSP_Controller controller; PSP_Memory memory={0}; PSP_CPU cpu;
    memset(&cpu,0,sizeof(cpu)); psp_controller_reset(&controller);
    psp_syscall_reset(); psp_nid_reset(); assert(psp_memory_init(&memory));
    psp_controller_update(&controller,PSP_HOST_A|PSP_HOST_RIGHT,false,false);
    assert(psp_controller_hle_register(&controller));
    uint32_t code=0;
    assert(psp_nid_resolve(PSP_NID_CTRL_READ_BUFFER_POSITIVE,&code));
    assert(code==PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE);
    cpu.gpr[4]=PSP_RAM_BASE+0x100u; cpu.gpr[5]=1;
    cpu.gpr[31]=PSP_RAM_BASE+0x300u; cpu.pc=PSP_RAM_BASE+0x200u;
    psp_memory_write32(&memory,cpu.pc,0x03E00008u); /* jr ra */
    psp_memory_write32(&memory,cpu.pc+4u,(code<<6)|0x0Cu);
    assert(psp_interpreter_step(&cpu,&memory));
    assert(psp_interpreter_step(&cpu,&memory));
    assert(cpu.pc==PSP_RAM_BASE+0x300u);
    assert(cpu.gpr[2]==1u);
    assert(psp_memory_read32(&memory,cpu.gpr[4]+4u)==(PSP_CTRL_CROSS|PSP_CTRL_RIGHT));
    assert(psp_nid_resolve(PSP_NID_CTRL_PEEK_BUFFER_POSITIVE,&code));
    assert(code==PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE);
    cpu.gpr[4]=PSP_RAM_BASE+0x120u; cpu.gpr[5]=1;
    cpu.pc=PSP_RAM_BASE+0x220u;
    psp_memory_write32(&memory,cpu.pc,0x03E00008u);
    psp_memory_write32(&memory,cpu.pc+4u,(code<<6)|0x0Cu);
    assert(psp_interpreter_step(&cpu,&memory));
    assert(psp_interpreter_step(&cpu,&memory));
    assert(cpu.pc==PSP_RAM_BASE+0x300u);
    assert(cpu.gpr[2]==1u);
    psp_controller_hle_unregister();
    assert(!psp_nid_resolve(PSP_NID_CTRL_READ_BUFFER_POSITIVE,&code));
    psp_syscall_reset(); psp_nid_reset(); psp_memory_destroy(&memory);
    return 0;
}
