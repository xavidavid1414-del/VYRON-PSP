
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
#include <sys/stat.h>

#define ISO_DIRECTORY "sdmc:/PSP/GAMES"
#define PSP_ROOT_DIRECTORY "sdmc:/PSP"
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

typedef struct {
    unsigned int show_fps;
    unsigned int stylus_menu;
    unsigned int cstick_enabled;
    unsigned int video_fit;
    unsigned int swap_ab;
    unsigned int swap_xy;
} VYRON_Settings;

static VYRON_Settings settings = {1, 1, 1, 0, 0, 0};
static bool settings_open;
static unsigned int settings_selected;

static u64 fps_window_start;
static unsigned int fps_frame_count;
static unsigned int displayed_fps;

static void update_fps_display(void)
{
    if (!settings.show_fps) {
        printf("\x1b[28;1H                 ");
        return;
    }

    u64 now = osGetTime();

    if (fps_window_start == 0)
        fps_window_start = now;

    fps_frame_count++;

    u64 elapsed = now - fps_window_start;

    if (elapsed >= 1000) {
        displayed_fps = (unsigned int)
            (((u64)fps_frame_count * 1000u) / elapsed);
        fps_frame_count = 0;
        fps_window_start = now;
    }

    printf("\x1b[28;1HFPS: %u   ", displayed_fps);
}

#define SETTINGS_FILE PSP_ROOT_DIRECTORY "/CONFIG/settings.ini"
#define SETTINGS_COUNT 6

static bool ensure_storage_layout(void)
{
    static const char *directories[] = {
        PSP_ROOT_DIRECTORY,
        ISO_DIRECTORY,
        PSP_ROOT_DIRECTORY "/SAVES",
        PSP_ROOT_DIRECTORY "/CONFIG",
        PSP_ROOT_DIRECTORY "/SYSTEM"
    };

    bool ready = true;

    for (size_t i = 0;
         i < sizeof(directories) / sizeof(directories[0]);
         i++) {
        if (mkdir(directories[i], 0777) != 0) {
            struct stat info;

            if (stat(directories[i], &info) != 0 ||
                (info.st_mode & S_IFDIR) == 0) {
                ready = false;
            }
        }
    }

    return ready;
}

static void load_settings(void)
{
    FILE *file = fopen(SETTINGS_FILE, "r");

    if (file == NULL)
        return;

    unsigned int values[SETTINGS_COUNT];
    int count = fscanf(file, "%u %u %u %u %u %u",
                       &values[0], &values[1], &values[2],
                       &values[3], &values[4], &values[5]);
    fclose(file);

    if (count != SETTINGS_COUNT)
        return;

    for (unsigned int i = 0; i < SETTINGS_COUNT; i++) {
        if (values[i] > 1)
            return;
    }

    settings.show_fps = values[0];
    settings.stylus_menu = values[1];
    settings.cstick_enabled = values[2];
    settings.video_fit = values[3];
    settings.swap_ab = values[4];
    settings.swap_xy = values[5];
}

static void save_settings(void)
{
    FILE *file = fopen(SETTINGS_FILE, "w");

    if (file == NULL) {
        snprintf(status_message, sizeof(status_message),
                 "Cannot save settings.ini");
        return;
    }

    fprintf(file, "%u %u %u %u %u %u\n",
            settings.show_fps,
            settings.stylus_menu,
            settings.cstick_enabled,
            settings.video_fit,
            settings.swap_ab,
            settings.swap_xy);
    fclose(file);
}

static void print_settings_menu(void)
{
    consoleClear();
    printf("\x1b[1;1HVYRON PSP | SETTINGS");
    printf("\x1b[3;1HUP/DOWN: Select  A: Change");
    printf("\x1b[4;1HB: Back  START: Exit");
    printf("\x1b[6;1H%s FPS counter: %s",
           settings_selected == 0 ? ">" : " ",
           settings.show_fps ? "ON" : "OFF");
    printf("\x1b[8;1H%s Stylus menu: %s",
           settings_selected == 1 ? ">" : " ",
           settings.stylus_menu ? "ON" : "OFF");
    printf("\x1b[10;1H%s C-Stick: %s",
           settings_selected == 2 ? ">" : " ",
           settings.cstick_enabled ? "ON" : "OFF");
    printf("\x1b[12;1H%s Video fit: %s",
           settings_selected == 3 ? ">" : " ",
           settings.video_fit ? "STRETCH" : "FIT");
    printf("\x1b[14;1H%s Swap A/B: %s",
           settings_selected == 4 ? ">" : " ",
           settings.swap_ab ? "ON" : "OFF");
    printf("\x1b[16;1H%s Swap X/Y: %s",
           settings_selected == 5 ? ">" : " ",
           settings.swap_xy ? "ON" : "OFF");
    printf("\x1b[19;1HSaved in /PSP/CONFIG/settings.ini");
    printf("\x1b[21;1HNote: game input/video options");
    printf("\x1b[22;1Hare stored; runtime support is");
    printf("\x1b[23;1Hbeing implemented.");
}

static void change_selected_setting(void)
{
    switch (settings_selected) {
        case 0: settings.show_fps = !settings.show_fps; break;
        case 1: settings.stylus_menu = !settings.stylus_menu; break;
        case 2: settings.cstick_enabled = !settings.cstick_enabled; break;
        case 3: settings.video_fit = !settings.video_fit; break;
        case 4: settings.swap_ab = !settings.swap_ab; break;
        case 5: settings.swap_xy = !settings.swap_xy; break;
        default: return;
    }

    core.video.stretch = settings.video_fit != 0;
    save_settings();
    print_settings_menu();
}

static bool has_iso_extension(const char *name)
{
    const char *dot = strrchr(name, '.');

    return dot && strcasecmp(dot, ".iso") == 0;
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
                 "Cannot open ISO file");
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
    if (eboot_entry.size >= 4 &&
        memcmp(buffer, "~PSP", 4) == 0) {
        snprintf(status_message, sizeof(status_message),
                 "Encrypted PRX: decryption not supported");
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
        printf("Folder not found. Check /PSP/GAMES.\n");
    } else if (iso_count == 0) {
        printf("No ISO files found.\n");
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

    if (!ensure_storage_layout()) {
        snprintf(status_message, sizeof(status_message),
                 "SD folder setup incomplete");
    }

    load_settings();
    core.video.stretch = settings.video_fit != 0;
    scan_iso_directory();
    print_file_list();

    while (aptMainLoop()) {
        hidScanInput();

        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if (down & KEY_Y) {
            settings_open = true;
            settings_selected = 0;
            print_settings_menu();
        }

        if (settings_open) {
            if (down & KEY_B) {
                settings_open = false;
                print_file_list();
            }

            if (down & KEY_UP) {
                if (settings_selected > 0)
                    settings_selected--;
                print_settings_menu();
            }

            if (down & KEY_DOWN) {
                if (settings_selected + 1 < SETTINGS_COUNT)
                    settings_selected++;
                print_settings_menu();
            }

            if (down & KEY_A)
                change_selected_setting();

            if (down & KEY_TOUCH) {
                touchPosition touch;
                hidTouchRead(&touch);
                if (touch.py >= 40 && touch.py < 136) {
                    unsigned int index = (touch.py - 40) / 16;
                    if (index < SETTINGS_COUNT) {
                        settings_selected = index;
                        change_selected_setting();
                    }
                }
            }

            execute_cpu_frame();
            draw_interface(&core.video);
            psp_video_present(&core.video);
            update_fps_display();
            continue;
        }

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
        update_fps_display();
    }

    execution_active = false;

    if (audio_ready)
        psp_audio_shutdown();

    psp_core_destroy(&core);
    gfxExit();

    return 0;
}
