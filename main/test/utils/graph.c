//
// Created by Samuel-Anton Jansen on 2025/11/15.
//

#include "graph.h"

#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

void graph_add(Graph *graph, double value) {
    graph->data = realloc(graph->data, (graph->size + 1) * sizeof(double));
    graph->data[graph->size] = value;
    graph->size++;
}

double graph_get_calculated_y(Graph *graph, BmpImage *bmp, double y) {
    y -= graph->min;
    y = y * (double) bmp->height / (graph->max - graph->min);
    return bmp->height - y;
}

void graph_render(Graph *graph, const char *file_path) {
    BmpImage bmp = {
        .width = 64 * 8,
        .height = 64 * 6,
    };
    bmp_init(&bmp);

    // Draw axis
    bmp_set_color(0, 255, 255);
    for (int i = graph->min; i <= graph->max; i++) {
        for (int x = 0; x < (i % 10 ? 5 : 10); x++) {
            bmp_draw_pixel(&bmp, x, graph_get_calculated_y(graph, &bmp, i));
        }

        if (i % 10) continue;
        // Draw horizontal axis
        for (int x = 0; x < bmp.width; x += 10) {
            bmp_draw_pixel(&bmp, x, graph_get_calculated_y(graph, &bmp, i));
            // bmp_draw_pixel(&bmp, x+1, graph_get_calculated_y(graph, &bmp, graph->highlight_y));
        }
    }
    for (int i = 0; i <= graph->size; i += 10) {
        int x = i * (double) bmp.width / graph->size;
        for (int y = 0; y < (i % 100 ? 5 : 10); y++) {
            bmp_draw_pixel(&bmp, x, bmp.height - y);
        }
    }
    bmp_set_color(0, 200, 200);
    for (int x = 0; x < bmp.width; x += 5) {
        bmp_draw_pixel(&bmp, x, graph_get_calculated_y(graph, &bmp, graph->highlight_y));
        bmp_draw_pixel(&bmp, x + 1, graph_get_calculated_y(graph, &bmp, graph->highlight_y));
    }

    // Draw data
    bmp_set_color(255, 255, 255);
    for (int x = 0; x < bmp.width; x++) {
        int closest_index = x * (double) graph->size / bmp.width;
        if (closest_index >= graph->size) continue;
        double value = graph->data[closest_index];

        bmp_draw_pixel(&bmp, x, graph_get_calculated_y(graph, &bmp, value));
    }

    bmp_save(&bmp, file_path);
}
