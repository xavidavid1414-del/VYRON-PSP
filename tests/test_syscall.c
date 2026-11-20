#include "psp_syscall.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define TEST_SYSCALL 0x12345u

static bool handler_a(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
)
{
    if (cpu == 0 || memory == 0 || syscall_code != TEST_SYSCALL)
        return false;

    cpu->gpr[2] = 0xA11Au;
    return true;
}

static bool handler_b(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
)
{
    if (cpu == 0 || memory == 0 || syscall_code != TEST_SYSCALL)
        return false;

    cpu->gpr[2] = 0xB22Bu;
    return true;
}

int main(void)
{
    uint8_t ram[16] = {0};
    PSP_Memory memory = { .ram = ram, .size = sizeof(ram) };
    PSP_CPU cpu;
    memset(&cpu, 0, sizeof(cpu));

    psp_syscall_reset();

    assert(!psp_syscall_dispatch(&cpu, &memory, TEST_SYSCALL));
    assert(!psp_syscall_register(TEST_SYSCALL, 0));
    assert(!psp_syscall_dispatch(0, &memory, TEST_SYSCALL));
    assert(!psp_syscall_dispatch(&cpu, 0, TEST_SYSCALL));

    assert(psp_syscall_register(TEST_SYSCALL, handler_a));
    assert(psp_syscall_dispatch(&cpu, &memory, TEST_SYSCALL));
    assert(cpu.gpr[2] == 0xA11Au);

    /* Registering an existing code replaces its handler, not its slot. */
    assert(psp_syscall_register(TEST_SYSCALL, handler_b));
    cpu.gpr[2] = 0;
    assert(psp_syscall_dispatch(&cpu, &memory, TEST_SYSCALL));
    assert(cpu.gpr[2] == 0xB22Bu);

    assert(!psp_syscall_unregister(TEST_SYSCALL + 1u));
    assert(psp_syscall_unregister(TEST_SYSCALL));
    assert(!psp_syscall_dispatch(&cpu, &memory, TEST_SYSCALL));

    /* Reset clears every registered entry. */
    assert(psp_syscall_register(TEST_SYSCALL, handler_a));
    psp_syscall_reset();
    assert(!psp_syscall_dispatch(&cpu, &memory, TEST_SYSCALL));

    /* The registry has a fixed capacity and rejects overflow cleanly. */
    for (uint32_t i = 0; i < 64u; i++)
        assert(psp_syscall_register(0x1000u + i, handler_a));

    assert(!psp_syscall_register(0x2000u, handler_a));
    assert(psp_syscall_unregister(0x1000u));
    assert(psp_syscall_register(0x2000u, handler_b));

    psp_syscall_reset();
    return 0;
}
