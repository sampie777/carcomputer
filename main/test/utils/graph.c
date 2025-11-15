//
// Created by Samuel-Anton Jansen on 2025/11/15.
//

#include "graph.h"

#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

void graph_add(Graph *graph, long value) {
    graph->data = realloc(graph->data, (graph->size + 1) * sizeof(long));
    graph->data[graph->size] = value;
    graph->size++;
}

void graph_render(Graph *graph) {
    BmpImage bmp = {
        .width = 64 * 8,
        .height = 64 * 6,
    };
    bmp_init(&bmp);

    for (int x = 0; x < bmp.width; x++) {
        int closest_index = (x * graph->size) / bmp.width;
        if (closest_index >= graph->size) continue;
        long value = graph->data[closest_index];

        int y = value - graph->min;
        y = (y * bmp.height) / (graph->max - graph->min);
        if (y < 0 || y >= bmp.height) continue;

        bmp_draw_pixel(&bmp, x, bmp.height - y);
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "../../../test_output/graph.bmp");
    bmp_save(&bmp, filename);
}


