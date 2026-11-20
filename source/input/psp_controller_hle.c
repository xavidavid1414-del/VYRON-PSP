#include "psp_controller_hle.h"

#include <stddef.h>

static uint32_t controller_timestamp;

static int write_samples(
    const PSP_Controller *controller,
    PSP_Memory *memory,
    uint32_t output_address,
    uint32_t sample_count
)
{
    if (controller == NULL || memory == NULL || sample_count == 0)
        return 0;

    if (sample_count > PSP_CTRL_MAX_SAMPLES)
        sample_count = PSP_CTRL_MAX_SAMPLES;

    if (sample_count > UINT32_MAX / PSP_CTRL_DATA_SIZE)
        return -1;

    uint32_t total_size = sample_count * PSP_CTRL_DATA_SIZE;
    if (!psp_memory_is_valid(memory, output_address, total_size))
        return -1;

    for (uint32_t i = 0; i < sample_count; i++) {
        uint32_t address = output_address + i * PSP_CTRL_DATA_SIZE;
        controller_timestamp++;

        psp_memory_write32(memory, address, controller_timestamp);
        psp_memory_write32(memory, address + 4u, controller->buttons);
        psp_memory_write8(memory, address + 8u, 128u);
        psp_memory_write8(memory, address + 9u, 128u);

        for (uint32_t reserved = 10u; reserved < PSP_CTRL_DATA_SIZE;
             reserved++) {
            psp_memory_write8(memory, address + reserved, 0u);
        }
    }

    return (int)sample_count;
}

int psp_controller_hle_read_buffer_positive(
    const PSP_Controller *controller,
    PSP_Memory *memory,
    uint32_t output_address,
    uint32_t sample_count
)
{
    return write_samples(controller, memory, output_address, sample_count);
}

int psp_controller_hle_peek_buffer_positive(
    const PSP_Controller *controller,
    PSP_Memory *memory,
    uint32_t output_address,
    uint32_t sample_count
)
{
    return write_samples(controller, memory, output_address, sample_count);
}
