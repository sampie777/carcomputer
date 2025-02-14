//
// Created by samuel on 17-7-22.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../config.h"
#include "display.h"
#include "../../utils.h"
#include "sh1106.h"
#include "icons.h"
#include "../../version.h"
#include "../../error_codes.h"
#include "display_screens.h"
#include "../../return_codes.h"
#include "special_chars.h"
#include "font.h"
#include "../sd_card.h"

SH1106Config sh1106_config = {
        .address = DISPLAY_I2C_ADDRESS,
        .mirror_vertical = DISPLAY_UPSIDE_DOWN,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
};

void display_init() {
    printf("[Display] Initializing display...\n");
    if (sh1106_init(&sh1106_config) != RESULT_OK) {
        printf("[Display] Init failed\n");
        return;
    }
    printf("[Display] Init done\n");
}


void show_video(SH1106Config *display, const char *_content, long file_size) {
    static long frame = 0;
    int lines_per_screen = display->height >> 3;
    int chunk_size = lines_per_screen * display->width;
    char *content = NULL;

    long chars_read = sd_card_read_file_part("matrix_output.bin", frame == 0 ? 0 : -1, chunk_size, &content);

    if (chars_read > 0) {
        for (int y = 0; y < lines_per_screen; y++) {
            for (int x = 0; x < display->width; x++) {
                long content_index = y * display->width + x;
                if (content_index >= file_size) break;

                sh1106_draw_byte(display, x, y * 8, content[content_index], FONT_WHITE);
            }
        }
    }

    frame++;
    if (frame * chunk_size >= file_size) {
        frame = 0; // Reset index if we reach the end of the content
    }
    free(content);
}

void display_update(State *state, const char *content, long size) {
    static int64_t last_update_time = 0;
    if (esp_timer_get_time_ms() < last_update_time + DISPLAY_UPDATE_MIN_INTERVAL) return;
    last_update_time = esp_timer_get_time_ms();

    sh1106_clear(&sh1106_config);

    show_video(&sh1106_config, content, size);

    sh1106_display(&sh1106_config);
}
