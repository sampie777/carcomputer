//
// Created by Samuel-Anton Jansen on 2025/12/08.
//

#ifndef CARCOMPUTER_VIDEO_H
#define CARCOMPUTER_VIDEO_H

#include "../../state.h"

void video_init(State *state);
void video_display_update();
void video_render(int framerate);

#endif //CARCOMPUTER_VIDEO_H