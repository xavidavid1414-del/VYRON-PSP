#include "psp_controller_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
static PSP_Controller *active_controller;
static bool handle_ctrl_read(PSP_CPU *cpu,PSP_Memory *memory,uint32_t code) {
    if(!cpu || !memory || !active_controller || code!=PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE) return false;
    int result=psp_controller_hle_read_buffer_positive(active_controller,memory,cpu->gpr[4],cpu->gpr[5]);
    cpu->gpr[2]=(uint32_t)result; return true;
}
static bool handle_ctrl_peek(PSP_CPU *cpu,PSP_Memory *memory,uint32_t code) {
    if(!cpu || !memory || !active_controller || code!=PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE) return false;
    int result=psp_controller_hle_peek_buffer_positive(active_controller,memory,cpu->gpr[4],cpu->gpr[5]);
    cpu->gpr[2]=(uint32_t)result; return true;
}
bool psp_controller_hle_register(PSP_Controller *controller) {
    if(!controller) return false;
    active_controller=controller;
    if(!psp_syscall_register(PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE,handle_ctrl_read) ||
       !psp_syscall_register(PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE,handle_ctrl_peek) ||
       !psp_nid_register(PSP_NID_CTRL_READ_BUFFER_POSITIVE,PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE) ||
       !psp_nid_register(PSP_NID_CTRL_PEEK_BUFFER_POSITIVE,PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE)) {
        psp_controller_hle_unregister(); return false;
    }
    return true;
}
void psp_controller_hle_unregister(void) {
    psp_syscall_unregister(PSP_SYSCALL_CTRL_READ_BUFFER_POSITIVE);
    psp_syscall_unregister(PSP_SYSCALL_CTRL_PEEK_BUFFER_POSITIVE);
    psp_nid_unregister(PSP_NID_CTRL_READ_BUFFER_POSITIVE);
    psp_nid_unregister(PSP_NID_CTRL_PEEK_BUFFER_POSITIVE);
    active_controller=0;
}
