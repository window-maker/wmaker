/* rotate.c - image rotation
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 2000 Alfredo K. Kojima
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "wraster.h"

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif


static RImage *rotateImage(RImage *image, float angle);


RImage*
RRotateImage(RImage *image, float angle)
{
    RImage *img;
    int nwidth, nheight;
    int x, y;
    int bpp = image->format == RRGBAFormat ? 4 : 3;

    angle = ((int)angle % 360) + (angle - (int)angle);
    
    if (angle == 0.0) {
	return RCloneImage(image);
	
    } else if (angle == 90.0) {
	nwidth = image->height;
	nheight = image->width;

	img = RCreateImage(nwidth, nheight, True);
	if (!img) {
	    return NULL;
	}
	
	if (bpp == 3) {
	    unsigned char *optr, *nptr;
	    unsigned offs;


	    offs = nwidth * 4;
	    
	    optr = image->data;
	    nptr = img->data;

	    for (x = 0; x < nwidth; x++) {
		nptr = img->data + x*4;
		for (y = nheight; y; y--) {
		    nptr[0] = *optr++;
		    nptr[1] = *optr++;
		    nptr[2] = *optr++;
		    nptr[3] = 255;

		    nptr += offs;
		}
	    }
	} else {
	    unsigned *optr, *nptr;
	    unsigned *p;

	    optr = (unsigned*)image->data;
	    p = (unsigned*)img->data;
	    for (x = 0; x < nwidth; x++) {
		nptr = p++;
		for (y = nheight; y; y--) {
		    *nptr = *optr++;
		    nptr += nwidth;
		}
	    }
	}
    } else if (angle == 180.0) {
	
	nwidth = image->width;
	nheight = image->height;
	img = RCreateImage(nwidth, nheight, True);
	if (!img) {
	    return NULL;
	}
	
	if (bpp == 3) {
	    unsigned char *optr, *nptr;

	    optr = image->data;
	    nptr = img->data + nwidth * nheight * 4 - 4;

	    for (y = 0; y < nheight; y++) {
		for (x = 0; x < nwidth; x++) {
		    nptr[0] = optr[0];
		    nptr[1] = optr[1];
		    nptr[2] = optr[2];
		    nptr[3] = 255;
		
		    optr += 3;
		    nptr -= 4;
		}
	    }
	} else {
	    unsigned *optr, *nptr;
	    
	    optr = (unsigned*)image->data;
	    nptr = (unsigned*)img->data + nwidth * nheight - 1;
	    
	    for (y = nheight*nwidth-1; y >= 0; y--) {
		*nptr = *optr;
		optr++;
		nptr--;
	    }
	}
    } else if (angle == 270.0) {
	nwidth = image->height;
	nheight = image->width;

	img = RCreateImage(nwidth, nheight, True);
	if (!img) {
	    return NULL;
	}
	
	if (bpp == 3) {
	    unsigned char *optr, *nptr;
	    unsigned offs;


	    offs = nwidth * 4;
	    
	    optr = image->data;
	    nptr = img->data;

	    for (x = 0; x < nwidth; x++) {
		nptr = img->data + x*4;
		for (y = nheight; y; y--) {
		    nptr[0] = *optr++;
		    nptr[1] = *optr++;
		    nptr[2] = *optr++;
		    nptr[3] = 255;

		    nptr += offs;
		}
	    }
	} else {
	    unsigned *optr, *nptr;
	    unsigned *p;

	    optr = (unsigned*)image->data;
	    p = (unsigned*)img->data + nwidth*nheight;
	    for (x = 0; x < nwidth; x++) {
		nptr = p--;
		for (y = nheight; y; y--) {
		    *nptr = *optr++;
		    nptr -= nwidth;
		}
	    }
	}
    } else {
	img = rotateImage(image, angle);
    }
    
    return img;
}




/*
 * Image rotation through Bresenham's line algorithm:
 * 
 * If a square must be rotate by angle a, like in:
 *  _______
 * |    B  |
 * |   /4\ |     
 * |  /3 8\|
 * | /2 7 /|
 * |A1 6 / |      A_______B
 * | \5 / a| <--- |1 2 3 4|
 * |__C/_)_|      |5 6 7 8|
 *                C-------
 *
 * for each point P1 in the line from C to A
 *	for each point P2 in the perpendicular line starting at P1
 *		get pixel from the source and plot at P2
 * 		increment pixel location from source
 * 
 */


static void
copyLine(int x1, int y1, int x2, int y2, int nwidth, int format,
         unsigned char *dst, unsigned char **src)
{
    unsigned char *s = *src;
    int dx, dy;
    int xi, yi;
    int offset;
    int dpr, dpru, p;
    
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    
    if (x1 > x2) xi = -1; else xi = 1;
    if (y1 > y2) yi = -1; else yi = 1;
    
    if (dx >= dy) {
	
	dpr = dy << 1;
	dpru = dpr - (dx << 1);
	p = dpr - dx;
	
	while (dx-- >= 0) {
	    /* fetch and draw the pixel */
	    offset = (x1 + y1 * nwidth) << 2;
	    dst[offset++] = *s++;
	    dst[offset++] = *s++;
	    dst[offset++] = *s++;
	    if (format == RRGBAFormat)
		dst[offset++] = *s++;
	    else
		dst[offset++] = 255;

	    /* calc next step */
	    if (p > 0) {
		x1 += xi;
		y1 += yi;
		p += dpru;
	    } else {
		x1 += xi;
		p += dpr;
	    }
	}
    } else {
	
	dpr = dx << 1;
	dpru = dpr - (dy << 1);
	p = dpr - dy;
	
	while (dy-- >= 0) {	    
	    /* fetch and draw the pixel */
	    offset = (x1 + y1 * nwidth) << 2;
	    dst[offset++] = *s++;
	    dst[offset++] = *s++;
	    dst[offset++] = *s++;
	    if (format == RRGBAFormat)
		dst[offset++] = *s++;
	    else
		dst[offset++] = 255;

	    /* calc next step */
	    if (p > 0) {
		x1 += xi;
		y1 += yi;
		p += dpru;
	    } else {
		y1 += yi;
		p += dpr;
	    }
	}
    }

    
    *src = s;
}


static RImage*
rotateImage(RImage *image, float angle)
{
    RImage *img;
    int nwidth, nheight;
    int x1, y1;
    int x2, y2;
    int dx, dy;
    int xi, yi;
    int xx, yy;
    unsigned char *src, *dst;
    int dpr, dpru, p;
    
    /* only 180o for now */
    if (angle > 180.0)
	angle -= 180.0;
    
    
    angle = (angle * PI) / 180.0;
    
    nwidth = ceil(abs(cos(angle) * image->width))
	+ ceil(abs(cos(PI/2 - angle) * image->width));

    nheight = ceil(abs(sin(angle) * image->height)) 
	+ ceil(abs(cos(PI/2 - angle) * image->height));
    
    img = RCreateImage(nwidth, nheight, True);
    if (!img)
	return NULL;
    
    src = image->data;
    dst = img->data;
    
    x1 = floor(abs(cos(PI/2 - angle)*image->width));
    y1 = 0;
    
    x2 = 0;
    y2 = floor(abs(sin(PI/2 - angle)*image->width));

    xx = floor(abs(cos(angle)*image->height)) - 1;
    yy = nheight - 1;

    printf("%ix%i, %i %i     %i %i %i\n",
	   nwidth, nheight, x1, y1, x2, y2, (int)((angle*180.0)/PI));
    
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    
    if (x1 > x2) xi = -1; else xi = 1;
    if (y1 > y2) yi = -1; else yi = 1;
    
    if (dx >= dy) {
	dpr = dy << 1;
	dpru = dpr - (dx << 1);
	p = dpr - dx;

	while (dx-- >= 0) {

	    copyLine(x1, y1, xx, yy, nwidth, image->format, dst, &src);

	    /* calc next step */

	    if (p > 0) {
		x1 += xi;
		y1 += yi;
		xx += xi;
		yy += yi;
		p += dpru;
	    } else {
		x1 += xi;
		xx += xi;
		p += dpr;
	    }
	}
    } else {
	puts("NOT IMPLEMTENED");
	return img;
	dpr = dx << 1;
	dpru = dpr - (dy << 1);
	p = dpr - dy;

	while (dy-- >= 0) {
	    xx = abs(x1*sin(angle*PI/180.0));
	    yy = abs(y1*cos(angle*PI/180.0));

	    copyLine(x1, y1, xx, yy, nwidth, image->format, dst, &src);

	    /* calc next step*/
	    if (p > 0) {
		x1 += xi;
		y1 += yi;
		p += dpru;
	    } else {
		y1 += yi;
		p += dpr;
	    }
	}
    }
    
    return img;
}
