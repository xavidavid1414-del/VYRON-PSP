#ifndef PSP_VIDEO_H
#define PSP_VIDEO_H

#include <stdint.h>
#include <stdbool.h>

#define PSP_VIDEO_WIDTH 480
#define PSP_VIDEO_HEIGHT 272
#define PSP_VIDEO_STRIDE 512

#define PSP_VIDEO_COLOR(r, g, b, a) \
    ((((uint32_t)(a) & 0xFFu) << 24) | \
     (((uint32_t)(r) & 0xFFu) << 16) | \
     (((uint32_t)(g) & 0xFFu) << 8) | \
     ((uint32_t)(b) & 0xFFu))

typedef struct {
    uint32_t *framebuffer;
    uint32_t frame_count;
    bool stretch;
    bool initialized;
} PSP_Video;

bool psp_video_init(PSP_Video *video);
void psp_video_destroy(PSP_Video *video);

void psp_video_clear(PSP_Video *video, uint32_t color);

bool psp_video_put_pixel(
    PSP_Video *video,
    int x,
    int y,
    uint32_t color
);

void psp_video_fill_rect(
    PSP_Video *video,
    int x,
    int y,
    int width,
    int height,
    uint32_t color
);

uint32_t psp_video_get_pixel(
    const PSP_Video *video,
    int x,
    int y
);

void psp_video_present(PSP_Video *video);

#endif
