/* xpm.c - load XPM image from file
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


#ifdef USE_XPM

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/xpm.h>

#include "wraster.h"

RImage*
RGetImageFromXPMData(RContext *context, char **xpmData)
{
    Display *dpy = context->dpy;
    Colormap cmap = context->cmap;
    RImage *image;
    XpmImage xpm;
    unsigned char *color_table[4];
    unsigned char *data;
    int *p;
    int i;

    i = XpmCreateXpmImageFromData(xpmData, &xpm, (XpmInfo *)NULL);
    if (i!=XpmSuccess) {
        switch (i) {
        case XpmOpenFailed:
            RErrorCode = RERR_OPEN;
            break;
        case XpmFileInvalid:
            RErrorCode = RERR_BADIMAGEFILE;
            break;
        case XpmNoMemory:
            RErrorCode = RERR_NOMEMORY;
            break;
        default:
            RErrorCode = RERR_BADIMAGEFILE;
            break;
        }
        return NULL;
    }
    if (xpm.height<1 || xpm.width < 1) {
        RErrorCode = RERR_BADIMAGEFILE;
        XpmFreeXpmImage(&xpm);
        return NULL;
    }

    if (xpm.colorTable==NULL) {
        RErrorCode = RERR_BADIMAGEFILE;
        XpmFreeXpmImage(&xpm);
        return NULL;
    }
    image = RCreateImage(xpm.width, xpm.height, True);
    if (!image) {
        XpmFreeXpmImage(&xpm);
        return NULL;
    }

    /* make color table */
    for (i=0; i<4; i++) {
        color_table[i] = malloc(xpm.ncolors*sizeof(char));
        if (!color_table[i]) {
            for (i=i-1;i>=0; i--) {
                if (color_table[i])
                    free(color_table[i]);
            }
            RReleaseImage(image);
            RErrorCode = RERR_NOMEMORY;
            XpmFreeXpmImage(&xpm);
            return NULL;
        }
    }

    for (i=0; i<xpm.ncolors; i++) {
        XColor xcolor;
        char * color = NULL;

        if (xpm.colorTable[i].c_color)
            color = xpm.colorTable[i].c_color;
        else if (xpm.colorTable[i].g_color)
            color = xpm.colorTable[i].g_color;
        else if (xpm.colorTable[i].g4_color)
            color = xpm.colorTable[i].g4_color;
        else if (xpm.colorTable[i].m_color)
            color = xpm.colorTable[i].m_color;
        else if (xpm.colorTable[i].symbolic)
            color = xpm.colorTable[i].symbolic;

        if (!color) {
            color_table[0][i] = 0xbe;
            color_table[1][i] = 0xbe;
            color_table[2][i] = 0xbe;
            color_table[3][i] = 0xff;
            continue;
        }

        if (strncmp(color,"None",4)==0) {
            color_table[0][i]=0;
            color_table[1][i]=0;
            color_table[2][i]=0;
            color_table[3][i]=0;
            continue;
        }
        if (XParseColor(dpy, cmap, color, &xcolor)) {
            color_table[0][i] = xcolor.red>>8;
            color_table[1][i] = xcolor.green>>8;
            color_table[2][i] = xcolor.blue>>8;
            color_table[3][i] = 0xff;
        } else {
            color_table[0][i] = 0xbe;
            color_table[1][i] = 0xbe;
            color_table[2][i] = 0xbe;
            color_table[3][i] = 0xff;
        }
    }
    /* convert pixmap to RImage */
    p = (int*)xpm.data;
    data = image->data;
    for (i=0; i<xpm.width*xpm.height; i++) {
        *(data++)=color_table[0][*p];
        *(data++)=color_table[1][*p];
        *(data++)=color_table[2][*p];
        *(data++)=color_table[3][*p];
        p++;
    }
    for(i=0; i<4; i++) {
        free(color_table[i]);
    }
    XpmFreeXpmImage(&xpm);
    return image;
}



RImage*
RLoadXPM(RContext *context, char *file, int index)
{
    Display *dpy = context->dpy;
    Colormap cmap = context->cmap;
    RImage *image;
    XpmImage xpm;
    unsigned char *color_table[4];
    unsigned char *data;
    int *p;
    int i;

    i = XpmReadFileToXpmImage(file, &xpm, (XpmInfo *)NULL);
    if (i!=XpmSuccess) {
        switch (i) {
        case XpmOpenFailed:
            RErrorCode = RERR_OPEN;
            break;
        case XpmFileInvalid:
            RErrorCode = RERR_BADIMAGEFILE;
            break;
        case XpmNoMemory:
            RErrorCode = RERR_NOMEMORY;
            break;
        default:
            RErrorCode = RERR_BADIMAGEFILE;
            break;
        }
        return NULL;
    }
    if (xpm.height<1 || xpm.width < 1) {
        RErrorCode = RERR_BADIMAGEFILE;
        XpmFreeXpmImage(&xpm);
        return NULL;
    }

    if (xpm.colorTable==NULL) {
        RErrorCode = RERR_BADIMAGEFILE;
        XpmFreeXpmImage(&xpm);
        return NULL;
    }
    image = RCreateImage(xpm.width, xpm.height, True);
    if (!image) {
        XpmFreeXpmImage(&xpm);
        return NULL;
    }

    /* make color table */
    for (i=0; i<4; i++) {
        color_table[i] = malloc(xpm.ncolors*sizeof(char));
        if (!color_table[i]) {
            for (i=i-1;i>=0; i--) {
                if (color_table[i])
                    free(color_table[i]);
            }
            RReleaseImage(image);
            RErrorCode = RERR_NOMEMORY;
            XpmFreeXpmImage(&xpm);
            return NULL;
        }
    }

    for (i=0; i<xpm.ncolors; i++) {
        XColor xcolor;
        char * color = NULL;

        if (xpm.colorTable[i].c_color)
            color = xpm.colorTable[i].c_color;
        else if (xpm.colorTable[i].g_color)
            color = xpm.colorTable[i].g_color;
        else if (xpm.colorTable[i].g4_color)
            color = xpm.colorTable[i].g4_color;
        else if (xpm.colorTable[i].m_color)
            color = xpm.colorTable[i].m_color;
        else if (xpm.colorTable[i].symbolic)
            color = xpm.colorTable[i].symbolic;

        if (!color) {
            color_table[0][i] = 0xbe;
            color_table[1][i] = 0xbe;
            color_table[2][i] = 0xbe;
            color_table[3][i] = 0xff;
            continue;
        }

        if (strncmp(color,"None",4)==0) {
            color_table[0][i]=0;
            color_table[1][i]=0;
            color_table[2][i]=0;
            color_table[3][i]=0;
            continue;
        }
        if (XParseColor(dpy, cmap, color, &xcolor)) {
            color_table[0][i] = xcolor.red>>8;
            color_table[1][i] = xcolor.green>>8;
            color_table[2][i] = xcolor.blue>>8;
            color_table[3][i] = 0xff;
        } else {
            color_table[0][i] = 0xbe;
            color_table[1][i] = 0xbe;
            color_table[2][i] = 0xbe;
            color_table[3][i] = 0xff;
        }
    }
    /* convert pixmap to RImage */
    p = (int*)xpm.data;
    data = image->data;
    for (i=0; i<xpm.width*xpm.height; i++, p++) {
        *(data++)=color_table[0][*p];
        *(data++)=color_table[1][*p];
        *(data++)=color_table[2][*p];
        *(data++)=color_table[3][*p];
    }
    for(i=0; i<4; i++) {
        free(color_table[i]);
    }
    XpmFreeXpmImage(&xpm);
    return image;
}

#endif /* USE_XPM */

