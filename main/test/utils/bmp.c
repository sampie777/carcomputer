//
// Created by samuel on 6/6/25.
//

#include "bmp.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Gotten from: https://forums.raspberrypi.com/viewtopic.php?t=175498

// *******************************************
//
// GRAPH3.C - Basic Graphics Support for CLI's
//
// *******************************************

//statically allocated arrays for graphics output
//main historgram display. 10 viewports
#define BMP_WIDTH (128)
#define BMP_HEIGHT (64)
char red[BMP_WIDTH][BMP_HEIGHT][10];
char green[BMP_WIDTH][BMP_HEIGHT][10];
char blue[BMP_WIDTH][BMP_HEIGHT][10];

long sx = BMP_WIDTH; //dimensions of viewport
long sy = BMP_HEIGHT;

//CURRENT COLOR SUPPORT
long cur_red[10];
long cur_green[10];
long cur_blue[10];

//note if using a refreshing browser page for output remember to include cache defeating

// FUNCTION PROTOTYPES

// Limited graphics support
//void writebmp(char *fname); //writes out the graphics arrays to a BMP file
//void bar2(int x, int y, int w, int h, char i);
//void clrscr();

// END OF FUNCTION PROTOTYPES

//Graphics Support Routines

void initgraph3() {
    int a;

    //set all current colors to white
    //for all viewports
    //(this way it will draw right out of the package)
    for (a = 0; a < 10; a++) {
        cur_red[a] = 255;
        cur_green[a] = 255;
        cur_blue[a] = 255;
    }
}

void setcolor(int vp, int r, int g, int b) {
    cur_red[vp] = r;
    cur_green[vp] = g;
    cur_blue[vp] = b;
}

void clrscr(int vp) {
    int x, y;

    //clearscreen to white
    for (x = 0; x < sx; x++)
        for (y = 0; y < sy; y++) {
            //			red[x][y][vp]=255; //to white
            //			green[x][y][vp]=255;
            //			blue[x][y][vp]=255;

            red[x][y][vp] = 0; //to black
            green[x][y][vp] = 0;
            blue[x][y][vp] = 0;
        }
}

void getpixel(int vp, int x, int y, char* r, char* g, char* b) {
    if ((x > 0) && (x < sx) && (y > 0) && (y < sy)) {
        *r = red[x][y][vp];
        *g = green[x][y][vp];
        *b = blue[x][y][vp];
    }
    else {
        *r = 0; //returns black on a clip
        *g = 0;
        *b = 0;
    }
}

void putpixel(int vp, int x, int y) {
    //wrap the putpixel to reduce segmentation faults
    //"clipping"

    if ((x >= 0) && (x < sx) && (y >= 0) && (y < sy)) {
        //uses current color
        red[x][y][vp] = cur_red[vp];
        green[x][y][vp] = cur_green[vp];
        blue[x][y][vp] = cur_blue[vp];
    }
}

int writebmp(char* fname, int vp) {
    FILE* fptr;
    long fs, is, a, b; //filesize, image size, counters
    int yr;
    char s[255];
    char n, o, p; //color save values

    //save the current color and restore at the end of the function
    //usually this is the last call to a viewport, but sometimes not.
    n = cur_red[vp];
    o = cur_green[vp];
    p = cur_blue[vp];


    //minimalist graphics support for CLI's
    char bmp_hdr[54] = {
            0x42, 0x4D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
            0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
            0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
            0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

    fs = 54 + (sx * sy * 3); //filesize is the dimentions of the array multiplied by 3 bytes (R,G and B bytes)
    is = (sx * sy * 3);

    //put RED registration square in top-left hand corner at (20,20), height =20, width=20
    //for (a=0;a<20;a++)
    //	for (b=0;b<20;b++)
    //		red[a+20][b+20][vp]=255;

    setcolor(vp, 255, 255, 255);
    //textout(vp,20,20,"Reg",1);

    //Stanford Systems copyright notice
    yr = 2016;
    //sprintf(s,"%d (c) Copyright Stanford Systems",yr);
    //textout(vp,sx/2-(8*strlen(s))/2,sy-10,s,1);

    memcpy((void*)&bmp_hdr[2], (void*)&fs, 4);
    memcpy((void*)&bmp_hdr[18], (void*)&sx, 4);
    memcpy((void*)&bmp_hdr[22], (void*)&sy, 4);
    memcpy((void*)&bmp_hdr[34], (void*)&is, 4);

    if ((fptr = fopen(fname, "wb")) != NULL) {
        fwrite(bmp_hdr, 54, 1, fptr);

        for (b = sy; b > 0; b--)
            for (a = 0; a < sx; a++) {
                fwrite(&blue[a][b][vp], 1, 1, fptr);
                fwrite(&green[a][b][vp], 1, 1, fptr);
                fwrite(&red[a][b][vp], 1, 1, fptr);
            }
    }
    else {
        printf("failed to open BMP file.\n");
        return -1; //error code indicating file could not be opened
    }

    fclose(fptr);

    //restore any viewport vars we changed here
    cur_red[vp] = n;
    cur_green[vp] = o;
    cur_blue[vp] = o;

    return 1;
}
