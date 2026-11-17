#include "psp_video.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

bool psp_video_init(PSP_Video *video)
{
    if (video == NULL)
        return false;

    memset(video, 0, sizeof(*video));

    size_t pixels =
        (size_t)PSP_VIDEO_STRIDE * PSP_VIDEO_HEIGHT;

    video->framebuffer = calloc(
        pixels,
        sizeof(uint32_t)
    );

    if (video->framebuffer == NULL)
        return false;

    video->frame_count = 0;
    video->initialized = true;

    return true;
}

void psp_video_destroy(PSP_Video *video)
{
    if (video == NULL)
        return;

    free(video->framebuffer);

    memset(video, 0, sizeof(*video));
}

void psp_video_clear(PSP_Video *video, uint32_t color)
{
    if (video == NULL ||
        !video->initialized ||
        video->framebuffer == NULL)
        return;

    size_t pixels =
        (size_t)PSP_VIDEO_STRIDE * PSP_VIDEO_HEIGHT;

    for (size_t i = 0; i < pixels; i++)
        video->framebuffer[i] = color;
}

bool psp_video_put_pixel(
    PSP_Video *video,
    int x,
    int y,
    uint32_t color
)
{
    if (video == NULL ||
        !video->initialized ||
        video->framebuffer == NULL)
        return false;

    if (x < 0 || x >= PSP_VIDEO_WIDTH ||
        y < 0 || y >= PSP_VIDEO_HEIGHT)
        return false;

    size_t index =
        (size_t)y * PSP_VIDEO_STRIDE + (size_t)x;

    video->framebuffer[index] = color;

    return true;
}

void psp_video_fill_rect(
    PSP_Video *video,
    int x,
    int y,
    int width,
    int height,
    uint32_t color
)
{
    if (video == NULL ||
        !video->initialized ||
        video->framebuffer == NULL ||
        width <= 0 || height <= 0)
        return;

    int64_t left = x;
    int64_t top = y;
    int64_t right = (int64_t)x + width;
    int64_t bottom = (int64_t)y + height;

    if (left < 0)
        left = 0;

    if (top < 0)
        top = 0;

    if (right > PSP_VIDEO_WIDTH)
        right = PSP_VIDEO_WIDTH;

    if (bottom > PSP_VIDEO_HEIGHT)
        bottom = PSP_VIDEO_HEIGHT;

    if (left >= right || top >= bottom)
        return;

    for (int64_t row = top; row < bottom; row++)
    {
        size_t index =
            (size_t)row * PSP_VIDEO_STRIDE +
            (size_t)left;

        for (int64_t column = left;
             column < right;
             column++)
        {
            video->framebuffer[index++] = color;
        }
    }
}

uint32_t psp_video_get_pixel(
    const PSP_Video *video,
    int x,
    int y
)
{
    if (video == NULL ||
        !video->initialized ||
        video->framebuffer == NULL)
        return 0;

    if (x < 0 || x >= PSP_VIDEO_WIDTH ||
        y < 0 || y >= PSP_VIDEO_HEIGHT)
        return 0;

    size_t index =
        (size_t)y * PSP_VIDEO_STRIDE + (size_t)x;

    return video->framebuffer[index];
}

void psp_video_present(PSP_Video *video)
{
    if (video == NULL ||
        !video->initialized)
        return;

    video->frame_count++;
}
