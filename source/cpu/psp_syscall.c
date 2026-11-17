#include "psp_syscall.h"

#define PSP_SYSCALL_MAX_HANDLERS 64

typedef struct {
    uint32_t code;
    PSP_SyscallHandler handler;
    bool active;
} PSP_SyscallEntry;

static PSP_SyscallEntry syscall_entries[PSP_SYSCALL_MAX_HANDLERS];

bool psp_syscall_register(
    uint32_t syscall_code,
    PSP_SyscallHandler handler
)
{
    if (handler == 0)
        return false;

    for (unsigned int i = 0; i < PSP_SYSCALL_MAX_HANDLERS; i++) {
        if (syscall_entries[i].active &&
            syscall_entries[i].code == syscall_code) {
            syscall_entries[i].handler = handler;
            return true;
        }
    }

    for (unsigned int i = 0; i < PSP_SYSCALL_MAX_HANDLERS; i++) {
        if (!syscall_entries[i].active) {
            syscall_entries[i].code = syscall_code;
            syscall_entries[i].handler = handler;
            syscall_entries[i].active = true;
            return true;
        }
    }

    return false;
}

bool psp_syscall_unregister(uint32_t syscall_code)
{
    for (unsigned int i = 0; i < PSP_SYSCALL_MAX_HANDLERS; i++) {
        if (syscall_entries[i].active &&
            syscall_entries[i].code == syscall_code) {
            syscall_entries[i].code = 0;
            syscall_entries[i].handler = 0;
            syscall_entries[i].active = false;
            return true;
        }
    }

    return false;
}

bool psp_syscall_dispatch(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
)
{
    if (cpu == 0 || memory == 0)
        return false;

    for (unsigned int i = 0; i < PSP_SYSCALL_MAX_HANDLERS; i++) {
        if (syscall_entries[i].active &&
            syscall_entries[i].code == syscall_code) {
            return syscall_entries[i].handler(
                cpu, memory, syscall_code);
        }
    }

    return false;
}

void psp_syscall_reset(void)
{
    for (unsigned int i = 0; i < PSP_SYSCALL_MAX_HANDLERS; i++) {
        syscall_entries[i].code = 0;
        syscall_entries[i].handler = 0;
        syscall_entries[i].active = false;
    }
}
