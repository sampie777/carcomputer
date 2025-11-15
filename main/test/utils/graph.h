//
// Created by Samuel-Anton Jansen on 2025/11/15.
//

#ifndef CARCOMPUTER_GRAPH_H
#define CARCOMPUTER_GRAPH_H
#include <stddef.h>

typedef struct {
    long max;
    long min;
    long *data;
    size_t size;
} Graph;

void graph_add(Graph *graph, long value);
void graph_render(Graph *graph);

#endif //CARCOMPUTER_GRAPH_H