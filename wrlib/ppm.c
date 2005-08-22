/* ppm.c - load PPM image from file
 *
 * Raster graphics library
 *
 * Copyright (c) 1997-2003 Alfredo K. Kojima
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>


#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wraster.h"


static RImage*
load_graymap(char *file_name, FILE *file, int w, int h, int max, int raw)
{
    RImage *image;

    image = RCreateImage(w, h, 0);
    if (!image) {
        return NULL;
    }
    if (!raw) {

    } else {
        if (max<256) {
            unsigned char *ptr;
            char *buf;
            int x, y;

            buf = malloc(w+1);
            if (!buf) {
                return NULL;
            }

            ptr = image->data;
            for (y = 0; y < h; y++) {
                if (!fread(buf, w, 1, file)) {
                    free(buf);
                    goto short_file;
                }
                for (x = 0; x < w; x++) {
                    *(ptr++) = buf[x];
                    *(ptr++) = buf[x];
                    *(ptr++) = buf[x];
                }
            }
            free(buf);
        } else {

        }
    }

    return image;

short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    return NULL;
}


static RImage*
load_pixmap(char *file_name, FILE *file, int w, int h, int max, int raw)
{
    RImage *image;
    int i;
    char buf[3];
    unsigned char *ptr;

    image = RCreateImage(w, h, 0);
    if (!image) {
        return NULL;
    }
    ptr = image->data;
    if (!raw) {

    } else {
        if (max<256) {
            i = 0;
            while (i < w*h) {
                if (fread(buf, 1, 3, file)!=3)
                    goto short_file;
                *(ptr++) = buf[0];
                *(ptr++) = buf[1];
                *(ptr++) = buf[2];
                i++;
            }
        } else {

        }
    }

    return image;

short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    return NULL;
}


RImage*
RLoadPPM(RContext *context, char *file_name, int index)
{
    FILE *file;
    RImage *image = NULL;
    char buffer[256];
    int w, h, m;
    int type;

#define GETL() if (!fgets(buffer, 255, file)) goto short_file

    file = fopen(file_name, "rb");
    if (!file) {
        RErrorCode = RERR_OPEN;
        return NULL;
    }

    /* get signature */
    GETL();

    /* only accept raw pixmaps or graymaps */
    if (buffer[0] != 'P' || (buffer[1] != '5' && buffer[1] != '6')) {
        RErrorCode = RERR_BADFORMAT;
        fclose(file);
        return NULL;
    }

    type = buffer[1];

    /* skip comments */
    while (1) {
        GETL();

        if (buffer[0]!='#')
            break;
    }

    /* get size */
    if (sscanf(buffer, "%i %i", &w, &h)!=2 || w < 1 || h < 1)
        goto bad_file;


    GETL();
    if (sscanf(buffer, "%i", &m)!=1 || m < 1)
        goto bad_file;

    if (type=='5')
        image = load_graymap(file_name, file, w, h, m, type=='5');
    else if (type=='6')
        image = load_pixmap(file_name, file, w, h, m, type=='6');

    fclose(file);
    return image;

bad_file:
short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    fclose(file);
    return NULL;
}

