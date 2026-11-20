#include "psp_controller.h"

void psp_controller_reset(PSP_Controller *controller)
{
    if (controller == 0)
        return;

    controller->buttons = 0;
    controller->previous_buttons = 0;
    controller->pressed = 0;
    controller->released = 0;
}

void psp_controller_update(
    PSP_Controller *controller,
    uint32_t host_buttons,
    bool swap_ab,
    bool swap_xy
)
{
    if (controller == 0)
        return;

    uint32_t mapped = 0;

    if (host_buttons & PSP_HOST_UP) mapped |= PSP_CTRL_UP;
    if (host_buttons & PSP_HOST_DOWN) mapped |= PSP_CTRL_DOWN;
    if (host_buttons & PSP_HOST_LEFT) mapped |= PSP_CTRL_LEFT;
    if (host_buttons & PSP_HOST_RIGHT) mapped |= PSP_CTRL_RIGHT;
    if (host_buttons & PSP_HOST_L) mapped |= PSP_CTRL_LTRIGGER;
    if (host_buttons & PSP_HOST_R) mapped |= PSP_CTRL_RTRIGGER;
    if (host_buttons & PSP_HOST_START) mapped |= PSP_CTRL_START;
    if (host_buttons & PSP_HOST_SELECT) mapped |= PSP_CTRL_SELECT;

    bool host_a = (host_buttons & PSP_HOST_A) != 0;
    bool host_b = (host_buttons & PSP_HOST_B) != 0;
    bool host_x = (host_buttons & PSP_HOST_X) != 0;
    bool host_y = (host_buttons & PSP_HOST_Y) != 0;

    if (swap_ab) {
        bool temporary = host_a;
        host_a = host_b;
        host_b = temporary;
    }

    if (swap_xy) {
        bool temporary = host_x;
        host_x = host_y;
        host_y = temporary;
    }

    if (host_a) mapped |= PSP_CTRL_CROSS;
    if (host_b) mapped |= PSP_CTRL_CIRCLE;
    if (host_x) mapped |= PSP_CTRL_TRIANGLE;
    if (host_y) mapped |= PSP_CTRL_SQUARE;

    controller->previous_buttons = controller->buttons;
    controller->buttons = mapped;
    controller->pressed = mapped & ~controller->previous_buttons;
    controller->released = controller->previous_buttons & ~mapped;
}
