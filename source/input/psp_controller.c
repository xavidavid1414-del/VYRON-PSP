#include "psp_controller.h"

void psp_controller_reset(PSP_Controller *controller)
{
    if (controller == 0)
        return;

    controller->buttons = 0;
    controller->previous_buttons = 0;
    controller->pressed = 0;
    controller->released = 0;
    controller->analog_x = 128u;
    controller->analog_y = 128u;
}

uint8_t psp_controller_normalize_axis(int16_t value)
{
    const int16_t deadzone = 8;
    const int16_t maximum = 156;

    if (value > maximum)
        value = maximum;
    if (value < -maximum)
        value = -maximum;

    if (value >= -deadzone && value <= deadzone)
        return 128u;

    if (value > 0) {
        int16_t adjusted = value - deadzone;
        int16_t range = maximum - deadzone;
        return (uint8_t)(128 + ((int32_t)adjusted * 127) / range);
    }

    int16_t adjusted = -value - deadzone;
    int16_t range = maximum - deadzone;
    return (uint8_t)(128 - ((int32_t)adjusted * 128) / range);
}

void psp_controller_set_analog(
    PSP_Controller *controller,
    uint8_t x,
    uint8_t y
)
{
    if (controller == 0)
        return;

    controller->analog_x = x;
    controller->analog_y = y;
}


void psp_controller_update_mapped(
    PSP_Controller *controller, uint32_t host_buttons,
    const uint8_t face_mapping[4])
{
    static const uint32_t face_buttons[4] = {
        PSP_CTRL_CROSS, PSP_CTRL_CIRCLE, PSP_CTRL_TRIANGLE, PSP_CTRL_SQUARE
    };
    static const uint32_t face_host[4] = {
        PSP_HOST_A, PSP_HOST_B, PSP_HOST_X, PSP_HOST_Y
    };
    static const uint32_t direction_host[4] = {
        PSP_HOST_UP, PSP_HOST_DOWN, PSP_HOST_LEFT, PSP_HOST_RIGHT
    };
    static const uint32_t direction_psp[4] = {
        PSP_CTRL_UP, PSP_CTRL_DOWN, PSP_CTRL_LEFT, PSP_CTRL_RIGHT
    };
    if (controller == 0 || face_mapping == 0) return;
    uint32_t mapped = 0;
    for (unsigned int i = 0; i < 4; i++) {
        if (host_buttons & direction_host[i]) mapped |= direction_psp[i];
        if ((host_buttons & face_host[i]) && face_mapping[i] < 4u)
            mapped |= face_buttons[face_mapping[i]];
    }
    if (host_buttons & PSP_HOST_L) mapped |= PSP_CTRL_LTRIGGER;
    if (host_buttons & PSP_HOST_R) mapped |= PSP_CTRL_RTRIGGER;
    if (host_buttons & PSP_HOST_START) mapped |= PSP_CTRL_START;
    if (host_buttons & PSP_HOST_SELECT) mapped |= PSP_CTRL_SELECT;
    controller->previous_buttons = controller->buttons;
    controller->buttons = mapped;
    controller->pressed = mapped & ~controller->previous_buttons;
    controller->released = controller->previous_buttons & ~mapped;
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
