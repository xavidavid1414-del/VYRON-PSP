
#include <3ds.h>

#include "psp_core.h"
#include "loader/psp_iso_file.h"
#include "loader/psp_iso_extract.h"
#include "audio/psp_audio.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define ISO_DIRECTORY "sdmc:/PSP/ISO"
#define MAX_ISO_FILES 32
#define ISO_NAME_LENGTH 128
#define CPU_CYCLES_PER_FRAME 250

typedef struct {
    char name[ISO_NAME_LENGTH];
} ISO_File;

static PSP_Core core;

static ISO_File iso_files[MAX_ISO_FILES];
static unsigned int iso_count;
static unsigned int selected;

static bool scan_ok;
static bool audio_ready;
static bool execution_active;

static char status_message[96];

static bool has_iso_extension(const char *name)
{
    const char *dot = strrchr(name, '.');

    return dot &&
        (strcasecmp(dot, ".iso") == 0 ||
         strcasecmp(dot, ".cso") == 0);
}

static void scan_iso_directory(void)
{
    DIR *directory = opendir(ISO_DIRECTORY);

    iso_count = 0;
    selected = 0;
    scan_ok = directory != NULL;

    if (!scan_ok) {
        snprintf(status_message, sizeof(status_message),
                 "Folder not found: %s", ISO_DIRECTORY);
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(directory)) != NULL &&
           iso_count < MAX_ISO_FILES) {

        if (entry->d_name[0] == '.' ||
            !has_iso_extension(entry->d_name))
            continue;

        snprintf(iso_files[iso_count].name,
                 sizeof(iso_files[iso_count].name),
                 "%s", entry->d_name);

        iso_count++;
    }

    closedir(directory);

    snprintf(status_message, sizeof(status_message),
             "%u PSP image(s) detected", iso_count);
}

static void probe_selected_iso(void)
{
    char path[256];
    PSP_ISO_File iso;
    PSP_ISO_FileEntry game_entry;
    PSP_ISO_FileEntry eboot_entry;

    if (selected >= iso_count)
        return;

    execution_active = false;

    snprintf(path, sizeof(path), "%s/%s",
             ISO_DIRECTORY, iso_files[selected].name);

    if (!psp_iso_file_open(&iso, path)) {
        snprintf(status_message, sizeof(status_message),
                 "Cannot open ISO (CSO unsupported)");
        return;
    }

    if (!psp_iso_file_find_path(
            &iso, "PSP_GAME", &game_entry) ||
        !game_entry.directory) {
        snprintf(status_message, sizeof(status_message),
                 "Invalid ISO: PSP_GAME not found");
        psp_iso_file_close(&iso);
        return;
    }

    if (!psp_iso_file_find_path(
            &iso, "PSP_GAME/SYSDIR/EBOOT.BIN", &eboot_entry) ||
        eboot_entry.directory || eboot_entry.size == 0) {
        snprintf(status_message, sizeof(status_message),
                 "EBOOT.BIN missing or invalid");
        psp_iso_file_close(&iso);
        return;
    }

    uint8_t *buffer = malloc(eboot_entry.size);

    if (buffer == NULL) {
        snprintf(status_message, sizeof(status_message),
                 "Not enough memory for EBOOT.BIN");
        psp_iso_file_close(&iso);
        return;
    }

    bool extracted = psp_iso_extract_entry(
        &iso,
        &eboot_entry,
        buffer,
        eboot_entry.size
    );

    psp_iso_file_close(&iso);

    if (!extracted) {
        snprintf(status_message, sizeof(status_message),
                 "EBOOT extraction failed");
        free(buffer);
        return;
    }

    psp_core_reset(&core);

    bool loaded = psp_core_load_executable_data(
        &core,
        buffer,
        eboot_entry.size
    );

    free(buffer);

    if (loaded) {
        execution_active = true;

        snprintf(status_message, sizeof(status_message),
                 "ELF loaded: CPU running");
    } else {
        execution_active = false;

        snprintf(status_message, sizeof(status_message),
                 "Encrypted/unsupported EBOOT");
    }
}

static void execute_cpu_frame(void)
{
    if (!execution_active || !core.image_loaded)
        return;

    if (!psp_core_run(&core, CPU_CYCLES_PER_FRAME)) {
        execution_active = false;

        snprintf(status_message, sizeof(status_message),
                 "CPU stopped: execution error");
    }
}

static void draw_interface(PSP_Video *video)
{
    psp_video_clear(video, PSP_VIDEO_COLOR(8, 12, 28, 255));

    psp_video_fill_rect(video, 18, 18, 444, 42,
                        PSP_VIDEO_COLOR(20, 30, 58, 255));

    psp_video_fill_rect(video, 18, 18, 5, 42,
                        PSP_VIDEO_COLOR(45, 170, 245, 255));

    psp_video_fill_rect(video, 18, 72, 444, 176,
                        PSP_VIDEO_COLOR(15, 23, 44, 255));

    if (iso_count > 0) {
        int row_y = 88 + (int)(selected % 6) * 25;

        psp_video_fill_rect(video, 30, row_y, 420, 21,
                            PSP_VIDEO_COLOR(30, 70, 112, 255));

        psp_video_fill_rect(video, 30, row_y, 4, 21,
                            PSP_VIDEO_COLOR(55, 190, 255, 255));
    }

    psp_video_fill_rect(video, 18, 256, 444, 3,
                        PSP_VIDEO_COLOR(45, 170, 245, 255));
}

static void print_file_list(void)
{
    consoleClear();

    printf("\x1b[1;1HVYRON PSP | CPU EXECUTION");
    printf("\x1b[3;1HFolder: %s", ISO_DIRECTORY);
    printf("\x1b[5;1H");

    if (!scan_ok) {
        printf("Folder not found. Create /PSP/ISO.\n");
    } else if (iso_count == 0) {
        printf("No ISO or CSO files found.\n");
        printf("Copy your PSP images into this folder.\n");
    } else {
        unsigned int first = selected > 5 ? selected - 5 : 0;
        unsigned int last = first + 6;

        if (last > iso_count)
            last = iso_count;

        for (unsigned int i = first; i < last; i++) {
            printf("%c %s\n",
                   i == selected ? '>' : ' ',
                   iso_files[i].name);
        }
    }

    printf("\n%s\n", status_message);

    printf("\nUP/DOWN: Select  A: Load");
    printf("\nB: Pause/Resume  X: Rescan");
    printf("\nSTART: Exit");
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    if (!psp_core_init(&core)) {
        printf("VYRON PSP: initialization failed.\n");

        while (aptMainLoop()) {
            hidScanInput();

            if (hidKeysDown() & KEY_START)
                break;
        }

        gfxExit();
        return 1;
    }

    audio_ready = psp_audio_init();

    scan_iso_directory();
    print_file_list();

    while (aptMainLoop()) {
        hidScanInput();

        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if (down & KEY_UP) {
            if (iso_count > 0 && selected > 0)
                selected--;

            print_file_list();
        }

        if (down & KEY_DOWN) {
            if (iso_count > 0 && selected + 1 < iso_count)
                selected++;

            print_file_list();
        }

        if (down & KEY_X) {
            scan_iso_directory();
            print_file_list();
        }

        if ((down & KEY_A) && iso_count > 0) {
            probe_selected_iso();
            print_file_list();
        }

        if (down & KEY_B) {
            if (core.image_loaded) {
                execution_active = !execution_active;

                snprintf(status_message, sizeof(status_message),
                         execution_active ?
                         "CPU resumed" : "CPU paused");

                print_file_list();
            }
        }

        execute_cpu_frame();

        draw_interface(&core.video);
        psp_video_present(&core.video);
    }

    execution_active = false;

    if (audio_ready)
        psp_audio_shutdown();

    psp_core_destroy(&core);
    gfxExit();

    return 0;
}
