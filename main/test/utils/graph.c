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
    initgraph3();
    setcolor(0, 255, 255, 255);

    for (int x = 0; x < BMP_WIDTH; x++) {
        int closest_index = (x * graph->size) / BMP_WIDTH;
        if (closest_index >= graph->size) continue;
        long value = graph->data[closest_index];

        int y = value - graph->min;
        y = (y * BMP_HEIGHT) / (graph->max - graph->min);
        if (y < 0 || y >= BMP_HEIGHT) continue;

        putpixel(0, x, BMP_HEIGHT - y);
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "../../../test_output/graph.bmp");
    writebmp(filename, 0);
}


