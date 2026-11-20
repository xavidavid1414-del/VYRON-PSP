#include "psp_controller.h"
#include "psp_controller_hle.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    PSP_Controller controller;
    PSP_Memory memory = {0};

    psp_controller_reset(&controller);
    assert(psp_memory_init(&memory));

    psp_controller_update(
        &controller,
        PSP_HOST_A | PSP_HOST_UP | PSP_HOST_L,
        false,
        false
    );

    uint32_t output = PSP_RAM_BASE + 0x100u;
    assert(psp_controller_hle_read_buffer_positive(
        &controller, &memory, output, 1u) == 1);

    assert(psp_memory_read32(&memory, output + 4u) ==
        (PSP_CTRL_CROSS | PSP_CTRL_UP | PSP_CTRL_LTRIGGER));
    assert(psp_memory_read8(&memory, output + 8u) == 128u);
    assert(psp_memory_read8(&memory, output + 9u) == 128u);

    psp_controller_set_analog(&controller, 200u, 64u);
    assert(psp_controller_hle_peek_buffer_positive(
        &controller, &memory, output + PSP_CTRL_DATA_SIZE, 1u) == 1);
    assert(psp_memory_read8(&memory,
        output + PSP_CTRL_DATA_SIZE + 8u) == 200u);
    assert(psp_memory_read8(&memory,
        output + PSP_CTRL_DATA_SIZE + 9u) == 64u);

    assert(psp_controller_hle_peek_buffer_positive(
        &controller, &memory, output + PSP_CTRL_DATA_SIZE, 2u) == 2);

    assert(psp_controller_hle_read_buffer_positive(
        &controller, &memory, PSP_RAM_BASE + PSP_RAM_SIZE - 8u, 1u) == -1);
    assert(psp_controller_hle_read_buffer_positive(
        &controller, &memory, output, 0u) == 0);

    psp_memory_destroy(&memory);
    return 0;
}
