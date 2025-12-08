//
// Created by Samuel-Anton Jansen on 2025/12/08.
//

#include "video.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "../../peripherals/display/display.h"
#include "../mocks/idf/freertos/task.h"

State *_state;

void video_init(State *state) {
    _state = state;
    set_update_function(&video_display_update);
    system("rm ../../../../test_output/screen*.bmp");
}

void video_display_update() {
    display_update(_state);
    update_bitmap();
}

void video_render(int framerate) {
    char command[512];
    snprintf(command, sizeof(command),
             "ffmpeg -y -framerate %d -pattern_type glob -i '../../../test_output/screen*.bmp' -c:v libx264 -pix_fmt yuv420p '../../../test_output/out.mp4'",
             framerate);
    system(command);
    system("rm ../../../test_output/screen*.bmp");
}
