/* xpm.c - load XPM image from file
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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
RGetImageFromXPMData(RContext *context, char **data)
{
    Display *dpy = context->dpy;
    Colormap cmap = context->cmap;
    RImage *image;
    XpmImage xpm;
    unsigned char *color_table[3];
    unsigned char *r, *g, *b, *a;
    int *p;
    int i;
    int transp=-1;
    
    i = XpmCreateXpmImageFromData(data, &xpm, (XpmInfo *)NULL);
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
    for (i=0; i<3; i++) {
	color_table[i] = malloc(xpm.ncolors*sizeof(char));
	if (!color_table[i]) {
	    for (i=i-1;i>=0; i--) {
		if (color_table[i])
		  free(color_table[i]);
	    }
	    RDestroyImage(image);
	    RErrorCode = RERR_NOMEMORY;
	    XpmFreeXpmImage(&xpm);
	    return NULL;
	}
    }

    for (i=0; i<xpm.ncolors; i++) {
	XColor xcolor;
	
	if (strncmp(xpm.colorTable[i].c_color,"None",4)==0) {
	    color_table[0][i]=0;
	    color_table[1][i]=0;
	    color_table[2][i]=0;
	    transp = i;
	    continue;
	}
	if (XParseColor(dpy, cmap, xpm.colorTable[i].c_color, &xcolor)) {
	    color_table[0][i] = xcolor.red>>8;
	    color_table[1][i] = xcolor.green>>8;
	    color_table[2][i] = xcolor.blue>>8;
	} else {
	    color_table[0][i]=0xbe;
	    color_table[1][i]=0xbe;
	    color_table[2][i]=0xbe;
	}
    }
    memset(image->data[3], 255, xpm.width*xpm.height);
    /* convert pixmap to RImage */
    p = (int*)xpm.data;
    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];
    for (i=0; i<xpm.width*xpm.height; i++) {
	if (*p==transp)
	  *a=0;
	a++;
	*(r++)=color_table[0][*p];
	*(g++)=color_table[1][*p];
	*(b++)=color_table[2][*p];
	p++;
    }
    for(i=0; i<3; i++) {
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
    unsigned char *color_table[3];
    unsigned char *r, *g, *b, *a;
    int *p;
    int i;
    int transp=-1;
    
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
    for (i=0; i<3; i++) {
	color_table[i] = malloc(xpm.ncolors*sizeof(char));
	if (!color_table[i]) {
	    for (i=i-1;i>=0; i--) {
		if (color_table[i])
		  free(color_table[i]);
	    }
	    RDestroyImage(image);
	    RErrorCode = RERR_NOMEMORY;
	    XpmFreeXpmImage(&xpm);
	    return NULL;
	}
    }

    for (i=0; i<xpm.ncolors; i++) {
	XColor xcolor;
	
	if (strncmp(xpm.colorTable[i].c_color,"None",4)==0) {
	    color_table[0][i]=0;
	    color_table[1][i]=0;
	    color_table[2][i]=0;
	    transp = i;
	    continue;
	}
	if (XParseColor(dpy, cmap, xpm.colorTable[i].c_color, &xcolor)) {
	    color_table[0][i] = xcolor.red>>8;
	    color_table[1][i] = xcolor.green>>8;
	    color_table[2][i] = xcolor.blue>>8;
	} else {
	    color_table[0][i]=0xbe;
	    color_table[1][i]=0xbe;
	    color_table[2][i]=0xbe;
	}
    }
    memset(image->data[3], 255, xpm.width*xpm.height);
    /* convert pixmap to RImage */
    p = (int*)xpm.data;
    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];
    for (i=0; i<xpm.width*xpm.height; i++) {
	if (*p==transp)
	  *a=0;
	a++;
	*(r++)=color_table[0][*p];
	*(g++)=color_table[1][*p];
	*(b++)=color_table[2][*p];
	p++;
    }
    for(i=0; i<3; i++) {
	free(color_table[i]);
    }
    XpmFreeXpmImage(&xpm);
    return image;
}

#endif /* USE_XPM */
