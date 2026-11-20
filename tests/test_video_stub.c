#include "psp_video.h"

#include <stdlib.h>
#include <string.h>

bool psp_video_init(PSP_Video *video)
{
    if (video == NULL)
        return false;
    memset(video, 0, sizeof(*video));
    video->initialized = true;
    return true;
}

void psp_video_destroy(PSP_Video *video)
{
    if (video != NULL)
        memset(video, 0, sizeof(*video));
}

void psp_video_clear(PSP_Video *video, uint32_t color)
{
    (void)video;
    (void)color;
}

bool psp_video_put_pixel(PSP_Video *video, int x, int y, uint32_t color)
{
    (void)video;
    (void)x;
    (void)y;
    (void)color;
    return false;
}

void psp_video_fill_rect(PSP_Video *video, int x, int y,
                         int width, int height, uint32_t color)
{
    (void)video;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

uint32_t psp_video_get_pixel(const PSP_Video *video, int x, int y)
{
    (void)video;
    (void)x;
    (void)y;
    return 0;
}

void psp_video_present(PSP_Video *video)
{
    (void)video;
}
