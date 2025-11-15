//
// Created by samuel on 6/6/25.
//

#include "bmp.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../math.h"

// Gotten from: https://forums.raspberrypi.com/viewtopic.php?t=175498
// Improved and refactored by myself

//CURRENT COLOR SUPPORT
uint8_t cur_red;
uint8_t cur_green;
uint8_t cur_blue;

void allocate_2d_array(uint8_t ***array, int width, int height) {
    *array = malloc(width * sizeof(uint8_t *));
    for (int i = 0; i < width; i++) {
        (*array)[i] = malloc(height * sizeof(uint8_t));
    }
}

void bmp_init(BmpImage *bmp) {
    allocate_2d_array(&bmp->red, bmp->width, bmp->height);
    allocate_2d_array(&bmp->blue, bmp->width, bmp->height);
    allocate_2d_array(&bmp->green, bmp->width, bmp->height);

    //set all current colors to white
    //(this way it will draw right out of the package)
    cur_red = 255;
    cur_green = 255;
    cur_blue = 255;
    bmp_clear(bmp);
}

void bmp_set_color(uint8_t r, uint8_t g, uint8_t b) {
    cur_red = r;
    cur_green = g;
    cur_blue = b;
}

void bmp_clear(BmpImage *bmp) {
    for (int x = 0; x < bmp->width; x++)
        for (int y = 0; y < bmp->height; y++) {
            bmp->red[x][y] = 0; //to black
            bmp->green[x][y] = 0;
            bmp->blue[x][y] = 0;
        }
}

void bmp_read_pixel(const BmpImage *bmp, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (x > 0 && x < bmp->width && y > 0 && y < bmp->height) {
        *r = bmp->red[x][y];
        *g = bmp->green[x][y];
        *b = bmp->blue[x][y];
    } else {
        *r = 0; //returns black on a clip
        *g = 0;
        *b = 0;
    }
}

void bmp_draw_pixel(BmpImage *bmp, int x, int y) {
    if (x < 0 || x >= bmp->width || y < 0 || y >= bmp->height) return;

    bmp->red[x][y] = cur_red;
    bmp->green[x][y] = cur_green;
    bmp->blue[x][y] = cur_blue;
}

void bmp_draw_line(BmpImage *bmp, int x0, int y0, int x1, int y1) {
    if (x0 == x1) {
        // Draw vertical line
        for (int y = min(y0, y1); y <= max(y0, y1); y++) {
            bmp_draw_pixel(bmp, x0, y);
        }
        return;
    }

    // y = ax  + b
    float a = (float) (y1 - y0) / (float) (x1 - x0);
    float b = y0 - a * x0;
    for (int x = min(x0, x1); x <= max(x0, x1); x++) {
        int y = a * x + b;
        bmp_draw_pixel(bmp, x, y);
    }
}

int bmp_save(const BmpImage *bmp, const char *file_path) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        printf("failed to open BMP file.\n");
        return -1;
    }

    //minimalist graphics support for CLI's
    char bmp_header[54] = {
        0x42, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
        0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    //filesize is the dimension of the array multiplied by 3 bytes (R,G and B bytes)
    int32_t file_size = sizeof(bmp_header) + bmp->width * bmp->height * 3;
    int32_t image_size = bmp->width * bmp->height * 3;

    memcpy(&bmp_header[2], &file_size, 4);
    memcpy(&bmp_header[18], &bmp->width, 4);
    memcpy(&bmp_header[22], &bmp->height, 4);
    memcpy(&bmp_header[34], &image_size, 4);

    fwrite(bmp_header, 54, 1, file);

    for (int b = bmp->height; b > 0; b--)
        for (int a = 0; a < bmp->width; a++) {
            fwrite(&bmp->blue[a][b], 1, 1, file);
            fwrite(&bmp->green[a][b], 1, 1, file);
            fwrite(&bmp->red[a][b], 1, 1, file);
        }

    fclose(file);
    return 1;
}
