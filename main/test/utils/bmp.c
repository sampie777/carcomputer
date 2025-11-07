//
// Created by samuel on 6/6/25.
//

#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>


void read_bmp(const char* file_name, BMP_File* image) {
    if (!image) {
        printf("ERROR: Output BMP_File pointer is NULL\n");
        exit(1);
    }

    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        printf("ERROR: Could not open file %s\n", file_name);
        exit(1);
    }

    fread(image->header, sizeof(char), 14, file);
    printf("Reading header: %lu\n", sizeof(BmpInfoHeader));
    fread(image->header + 14, sizeof(BmpInfoHeader), 1, file);
    BmpInfoHeader header = *(BmpInfoHeader*)&image->header[14];
    printf("Read header\n");
    image->width = header.width;
    image->height = header.height;
    image->bitDepth = header.colorDepth;

    printf(""
           "sizeOfThisHeader: %u\n"
           "width: %d\n"
           "height: %d\n"
           "numberOfColorPlanes: %hu\n"
           "colorDepth: %hu\n"
           "compressionMethod: %u\n"
           "rawBitmapDataSize: %u\n"
           "horizontalResolution: %d\n"
           "verticalResolution: %d\n"
           "colorTableEntries: %u\n"
           "importantColors: %u\n"
           "\n",
           header.sizeOfThisHeader,
           header.width,
           header.height,
           header.numberOfColorPlanes,
           header.colorDepth,
           header.compressionMethod,
           header.rawBitmapDataSize,
           header.horizontalResolution,
           header.verticalResolution,
           header.colorTableEntries,
           header.importantColors);


    if (image->bitDepth != 8) {
        printf("ERROR: Unsupported BMP bit depth: %d\n", image->bitDepth);
        fclose(file);
        exit(1);
    }

    // Read color table
    // if (bitDepth < 8) {
    // fread(NULL, sizeof(char), 1024, file);
    // }

    image->size = image->width * image->height * (image->bitDepth / 8);
    image->data = (unsigned char*)malloc(image->size * sizeof(char));
    if (!image->data) {
        printf("ERROR: Memory allocation failed\n");
        fclose(file);
        exit(1);
    }

    fread(image->data, sizeof(char), image->size, file);

    fclose(file);
}

void write_bmp(const char* file_name, const BMP_File* image) {
    if (!image) {
        printf("ERROR: Input BMP_File pointer is NULL\n");
        exit(1);
    }

    FILE* file = fopen(file_name, "wb");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing\n", file_name);
        exit(1);
    }

    fwrite(image->header, sizeof(char), sizeof(image->header), file);
    fwrite(image->data, sizeof(char), image->size, file);
    fwrite(image->data, sizeof(char), image->size, file);
    fclose(file);
}
