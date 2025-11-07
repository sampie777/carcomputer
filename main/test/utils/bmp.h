//
// Created by samuel on 6/6/25.
//

#ifndef BMP_H
#define BMP_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t sizeOfThisHeader;// = 40;
    int32_t width;// = 512; // in pixels
    int32_t height;// = 512; // in pixels
    uint16_t numberOfColorPlanes;// = 1; // must be 1
    uint16_t colorDepth;// = 24;
    uint32_t compressionMethod;// = 0;
    uint32_t rawBitmapDataSize;// = 0; // generally ignored
    int32_t horizontalResolution;// = 3780; // in pixel per meter
    int32_t verticalResolution;// = 3780; // in pixel per meter
    uint32_t colorTableEntries;// = 0;
    uint32_t importantColors;// = 0;
} BmpInfoHeader;

typedef struct {
    int width;
    int height;
    int bitDepth;
    size_t size;
    unsigned char* data;
    unsigned char header[54];
} BMP_File;

void read_bmp(const char* file_name, BMP_File *image);
void write_bmp(const char* file_name, const BMP_File *image);

#endif //BMP_H
