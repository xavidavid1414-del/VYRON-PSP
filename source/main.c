#include <3ds.h>
#include "psp_core.h"

static void draw_video_test(PSP_Video *video)
{
    psp_video_clear(video, PSP_VIDEO_COLOR(12, 16, 28, 255));

    psp_video_fill_rect(video, 24, 24, 432, 224,
        PSP_VIDEO_COLOR(28, 38, 58, 255));

    psp_video_fill_rect(video, 40, 40, 400, 12,
        PSP_VIDEO_COLOR(40, 160, 240, 255));

    psp_video_fill_rect(video, 40, 68, 120, 150,
        PSP_VIDEO_COLOR(220, 70, 80, 255));

    psp_video_fill_rect(video, 180, 68, 120, 150,
        PSP_VIDEO_COLOR(70, 190, 120, 255));

    psp_video_fill_rect(video, 320, 68, 120, 150,
        PSP_VIDEO_COLOR(230, 180, 60, 255));
}

int main(void)
{
    gfxInitDefault();

    PSP_Core core;

    if (!psp_core_init(&core))
    {
        gfxExit();
        return 1;
    }

    draw_video_test(&core.video);

    while (aptMainLoop())
    {
        hidScanInput();

        if (hidKeysDown() & KEY_START)
            break;

        psp_video_present(&core.video);
    }

    psp_core_destroy(&core);
    gfxExit();

    return 0;
}
