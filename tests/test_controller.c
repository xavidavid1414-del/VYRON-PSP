#include "psp_controller.h"

#include <assert.h>

int main(void)
{
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
