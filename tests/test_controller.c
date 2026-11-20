#include "psp_controller.h"

#include <assert.h>

int main(void)
{
    assert(psp_controller_normalize_axis(0) == 128u);
    assert(psp_controller_normalize_axis(8) == 128u);
    assert(psp_controller_normalize_axis(-8) == 128u);
    assert(psp_controller_normalize_axis(156) == 255u);
    assert(psp_controller_normalize_axis(-156) == 0u);
    assert(psp_controller_normalize_axis(200) == 255u);
    assert(psp_controller_normalize_axis(-200) == 0u);

    PSP_Controller controller;
    psp_controller_reset(&controller);

    psp_controller_update(
        &controller,
        PSP_HOST_A | PSP_HOST_X | PSP_HOST_UP | PSP_HOST_L,
        false,
        false
    );

    assert(controller.buttons ==
        (PSP_CTRL_CROSS | PSP_CTRL_TRIANGLE |
         PSP_CTRL_UP | PSP_CTRL_LTRIGGER));
    assert(controller.pressed == controller.buttons);
    assert(controller.released == 0);
    assert(controller.analog_x == 128u);
    assert(controller.analog_y == 128u);

    psp_controller_set_analog(&controller, 255u, 0u);
    assert(controller.analog_x == 255u);
    assert(controller.analog_y == 0u);

    psp_controller_update(
        &controller,
        PSP_HOST_B | PSP_HOST_Y | PSP_HOST_DOWN,
        true,
        true
    );

    assert(controller.buttons ==
        (PSP_CTRL_CROSS | PSP_CTRL_TRIANGLE | PSP_CTRL_DOWN));
    assert(controller.pressed ==
        (PSP_CTRL_DOWN));
    assert(controller.released ==
        (PSP_CTRL_UP | PSP_CTRL_LTRIGGER));

    psp_controller_update(&controller, 0, false, false);
    assert(controller.buttons == 0);
    assert(controller.pressed == 0);
    assert(controller.released ==
        (PSP_CTRL_CROSS | PSP_CTRL_TRIANGLE | PSP_CTRL_DOWN));

    psp_controller_reset(&controller);
    assert(controller.buttons == 0);
    assert(controller.previous_buttons == 0);
    assert(controller.pressed == 0);
    assert(controller.released == 0);

    return 0;
}
