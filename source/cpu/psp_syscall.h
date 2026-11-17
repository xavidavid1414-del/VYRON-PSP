#ifndef PSP_SYSCALL_H
#define PSP_SYSCALL_H

#include <stdbool.h>
#include <stdint.h>

#include "psp_cpu.h"
#include "../memory/psp_memory.h"

typedef bool (*PSP_SyscallHandler)(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
);

bool psp_syscall_register(
    uint32_t syscall_code,
    PSP_SyscallHandler handler
);

bool psp_syscall_unregister(uint32_t syscall_code);

bool psp_syscall_dispatch(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
);

void psp_syscall_reset(void);

#endif
