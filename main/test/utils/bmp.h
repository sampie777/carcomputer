//
// Created by samuel on 6/6/25.
//

#ifndef BMP_H
#define BMP_H
#include <stddef.h>
#include <stdint.h>

#define BMP_WIDTH (128)
#define BMP_HEIGHT (64)

void initgraph3();
void setcolor(int vp, int r, int g, int b);
void clrscr(int vp);
void getpixel(int vp, int x, int y, char* r, char* g, char* b);
void putpixel(int vp, int x, int y);
int writebmp(char* fname, int vp);

#endif //BMP_H
