/* 
 *  Raster graphics library
 * 
 *  Copyright (c) 1997-2000 Alfredo K. Kojima
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


/*
 *----------------------------------------------------------------------
 * RBlurImage--
 * 	Apply 3x3 1 1 1 low pass, convolution mask to image.
 *                1 2 1
 *                1 1 1 /10
 *----------------------------------------------------------------------
 */
int
RBlurImage(RImage *image)
{
    register int x, y;
    register int tmp;
    unsigned char *ptr, *nptr;
    unsigned char *pptr=NULL, *tmpp;
    int ch = image->format == RRGBAFormat ? 4 : 3;

    pptr = malloc(image->width * ch);
    if (!pptr) {
	RErrorCode = RERR_NOMEMORY;
	return False;
    }

#define MASK(prev, cur, next, ch)\
	(*(prev-ch) + *prev + *(prev+ch)\
	+*(cur-ch) + 2 * *cur + *(cur+ch)\
	+*(next-ch) + *next + *(next+ch)) / 10

    memcpy(pptr, image->data, image->width * ch);
    
    ptr = image->data;
    nptr = ptr + image->width*ch;
    tmpp = pptr;
    
    if (ch == 3) {
	ptr+=3;
	nptr+=3;
	pptr+=3;

	for (y = 1; y < image->height-1; y++) {
	    	    
	    for (x = 1; x < image->width-1; x++) {
		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 3);
		*pptr = tmp;
		ptr++; nptr++; pptr++;

		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 3);
		*pptr = tmp;
		ptr++; nptr++; pptr++;

		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 3);
		*pptr = tmp;
		ptr++; nptr++; pptr++;
	    }
	    pptr = tmpp;
	    ptr+=6;
	    nptr+=6;
	    pptr+=6;
	}
    } else {
	ptr+=4;
	nptr+=4;
	pptr+=4;

	for (y = 1; y < image->height-1; y++) {
	    for (x = 1; x < image->width-1; x++) {
		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 4);
		*pptr = tmp;
		ptr++; nptr++; pptr++;

		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 4);
		*pptr = tmp;
		ptr++; nptr++; pptr++;

		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 4);
		*pptr = tmp;
		ptr++; nptr++; pptr++;
		
		tmp = *ptr;
		*ptr = MASK(pptr, ptr, nptr, 4);
		*pptr = tmp;
		ptr++; nptr++; pptr++;
	    }
	    pptr = tmpp;
	    ptr+=8;
	    nptr+=8;
	    pptr+=8;
	}
    }
    

    return True;
}


#if 0
int
REdgeDetectImage(RImage *image)
{
    register int x, y, d1, d2, d3, d4, rsum;
    int w;
    unsigned char *r, *g, *b, *a;
    unsigned char *dr, *dg, *db, *da;
    unsigned char *pr=NULL, *pg=NULL, *pb=NULL, *pa=NULL;
    RImage *image2;


    image2 = RCloneImage(image);
    
    pr = alloca(image->width*sizeof(char));
    if (!pr)
      goto outofmem;

    pg = alloca(image->width*sizeof(char));
    if (!pg)
      goto outofmem;

    pb = alloca(image->width*sizeof(char));
    if (!pb)
      goto outofmem;

    pa = alloca(image->width*sizeof(char));
    if (!pa)
      goto outofmem;

    
    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];

    dr = image2->data[0];
    dg = image2->data[1];
    db = image2->data[2];
    da = image2->data[3];

    
    for (x=0; x<image->width; x++) {
	*(dr++) = *(r++);
	*(dg++) = *(g++);
	*(db++) = *(b++);
    }

    w = image->width;
    
    for (y=1; y<image->height-1; y++) {
	dr[w-1] = r[w-1];
	dg[w-1] = g[w-1];
	db[w-1] = b[w-1];

	*(dr++) = *(r++);
	*(dg++) = *(g++);
	*(db++) = *(b++);
	
	for (x=1; x<image->width-1; x++) {
	    d1 = r[w+1]  - r[-w-1];
	    d2 = r[1]    - r[-1];  
	    d3 = r[-w+1] - r[w-1]; 
	    d4 = r[-w]   - r[w];

	    rsum = d1 + d2 + d3;
	    if (rsum < 0) rsum = -rsum;
	    d1 = d1 - d2 - d4;         /* vertical gradient */
	    if (d1 < 0) d1 = -d1;
	    if (d1 > rsum) rsum = d1;
	    rsum /= 3;
	    
	    *(dr++) = rsum;
	    
	    d1 = g[w+1]  - g[-w-1];
	    d2 = g[1]    - g[-1];  
	    d3 = g[-w+1] - g[w-1]; 
	    d4 = g[-w]   - g[w];

	    rsum = d1 + d2 + d3;
	    if (rsum < 0) rsum = -rsum;
	    d1 = d1 - d2 - d4;         /* vertical gradient */
	    if (d1 < 0) d1 = -d1;
	    if (d1 > rsum) rsum = d1;
	    rsum /= 3;
	    
	    *(dg++) = rsum;

	    d1 = b[w+1]  - b[-w-1];
	    d2 = b[1]    - b[-1];  
	    d3 = b[-w+1] - b[w-1]; 
	    d4 = b[-w]   - b[w];

	    rsum = d1 + d2 + d3;
	    if (rsum < 0) rsum = -rsum;
	    d1 = d1 - d2 - d4;         /* vertical gradient */
	    if (d1 < 0) d1 = -d1;
	    if (d1 > rsum) rsum = d1;
	    rsum /= 3;
	    
	    *(db++) = rsum;
	    
	    r++;
	    g++;
	    b++;
	}
	    r++;
	    g++;
	    b++;

	dr++;
	dg++;
	db++;
    }
    {
	r = image->data[0];
	image2->data[0] = r;
	g = image->data[1];
	image2->data[1] = g;
	b = image->data[2];
	image2->data[2] = b;
	RDestroyImage(image2);
    }

#undef MASK
    
    return True;
}


int
RSmoothImage(RImage *image)
{
    register int x, y;
    register int v, w;
    unsigned char *ptr;
    int ch = image->format == RRGBAFormat;

    ptr = image->data;


    w = image->width*ch;
    for (y=0; y<image->height - 1; y++) {
	for (x=0; x<image->width - 1; x++) {
	    v = *ptr + 2 * *(ptr + ch) + 2 * *(ptr + w) + *(ptr + w + ch);
	    *ptr = v/6;
	    v = *(ptr+1) + 2 * *(ptr+1 + ch) + 2 * *(ptr+1 + w) + *(ptr+1 + w + ch);
	    *(ptr+1) = v/6;
            v = *(ptr+2) + 2 * *(ptr+2 + ch) + 2 * *(ptr+2 + w) + *(ptr+2 + w + ch);
	    *(ptr+2) = v/6;
	    
	    ptr+= ch;
	}
	/* last column */
	v = 3 * *ptr + 3 * *(ptr + w);
        *ptr = v/6;
	v = 3 * *(ptr+1) + 3 * *(ptr+1 + w);
        *(ptr+1) = v/6;
	v = 3 * *(ptr+2) + 3 * *(ptr+2 + w);
	*(ptr+2) = v/6;
	
	ptr+= ch;
    }
    
    /* last line */
    for (x=0; x<image->width - 1; x++) {
	v = 3 * *ptr + 3 * *(ptr + ch);
	*ptr = v/6;
	v = 3 * *(ptr+1) + 3 * *(ptr+1 + ch);
	*(ptr+1) = v/6;
	v = 3 * *(ptr+2) + 3 * *(ptr+2 + ch);
	*(ptr+2) = v/6;
    
	ptr+= ch;
    }

    return True;
}
#endif
