//
// Created by samuel on 6/6/25.
//

#ifndef BMP_H
#define BMP_H
#include <stdint.h>


typedef struct {
    int32_t width;
    int32_t height;
    uint8_t **red;
    uint8_t **blue;
    uint8_t **green;
} BmpImage;

void bmp_init(BmpImage *bmp);
void bmp_set_color(uint8_t r, uint8_t g, uint8_t b);
void bmp_clear(BmpImage *bmp);
void bmp_read_pixel(const BmpImage *bmp, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);
void bmp_draw_pixel(BmpImage *bmp, int x, int y);
void bmp_draw_line(BmpImage *bmp, int x0, int y0, int x1, int y1);
int bmp_save(const BmpImage *bmp, const char *file_path);

#endif //BMP_H
