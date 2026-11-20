#ifndef PSP_CONTROLLER_H
#define PSP_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

/* Platform-neutral input flags populated by the host frontend. */
#define PSP_HOST_A       (1u << 0)
#define PSP_HOST_B       (1u << 1)
#define PSP_HOST_X       (1u << 2)
#define PSP_HOST_Y       (1u << 3)
#define PSP_HOST_UP      (1u << 4)
#define PSP_HOST_DOWN    (1u << 5)
#define PSP_HOST_LEFT    (1u << 6)
#define PSP_HOST_RIGHT   (1u << 7)
#define PSP_HOST_L       (1u << 8)
#define PSP_HOST_R       (1u << 9)
#define PSP_HOST_START   (1u << 10)
#define PSP_HOST_SELECT  (1u << 11)

/* PSP sceCtrl button bit values. */
#define PSP_CTRL_SELECT   0x000001u
#define PSP_CTRL_START    0x000008u
#define PSP_CTRL_UP       0x000010u
#define PSP_CTRL_RIGHT    0x000020u
#define PSP_CTRL_DOWN     0x000040u
#define PSP_CTRL_LEFT     0x000080u
#define PSP_CTRL_LTRIGGER 0x000100u
#define PSP_CTRL_RTRIGGER 0x000200u
#define PSP_CTRL_TRIANGLE 0x001000u
#define PSP_CTRL_CIRCLE   0x002000u
#define PSP_CTRL_CROSS    0x004000u
#define PSP_CTRL_SQUARE   0x008000u

typedef struct {
    uint32_t buttons;
    uint32_t previous_buttons;
    uint32_t pressed;
    uint32_t released;
} PSP_Controller;

void psp_controller_reset(PSP_Controller *controller);
void psp_controller_update(
    PSP_Controller *controller,
    uint32_t host_buttons,
    bool swap_ab,
    bool swap_xy
);

#endif
