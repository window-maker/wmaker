/* 
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

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#   pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "wraster.h"


/*
 *----------------------------------------------------------------------
 * RBlurImage--
 * 	Apply 3x3 1 1 1 low pass, convolution mask to image.
 *                1 1 1
 *                1 1 1 /9
 *----------------------------------------------------------------------
 */
int
RBlurImage(RImage *image)
{
    register int x, y;
    register int w, tmp;
    unsigned char *r, *g, *b, *a;
    unsigned char *pr=NULL, *pg=NULL, *pb=NULL, *pa=NULL;

#define MASK(c,pc,p)   ((*c + *(c-1) + *(c+1) + pc[p] + pc[p-1] + pc[p+1] \
			+ *(c+w) + *(c+w-1) + *(c+w+1))/9)
    
    pr = (unsigned char*)alloca(image->width*sizeof(char));
    if (!pr)
      goto outofmem;

    pg = (unsigned char*)alloca(image->width*sizeof(char));
    if (!pg)
      goto outofmem;

    pb = (unsigned char*)alloca(image->width*sizeof(char));
    if (!pb)
      goto outofmem;

    pa = (unsigned char*)alloca(image->width*sizeof(char));
    if (!pa)
      goto outofmem;

    
    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];

    
    for (x=0; x<image->width; x++) {
	pr[x] = *(r++);
	pg[x] = *(g++);
	pb[x] = *(b++);
    }

    w = image->width;
    
    for (y=1; y<image->height-1; y++) {
	pr[w-1] = r[w-1];
	pg[w-1] = g[w-1];
	pb[w-1] = b[w-1];

	pr[0] = *(r++);
	pg[0] = *(g++);
	pb[0] = *(b++);
	
	for (x=1; x<image->width-1; x++) {
	    tmp = *r;
	    *(r++) = MASK(r,pr,x);
	    pr[x] = tmp;
	    
	    tmp = *g;
	    *(g++) = MASK(g,pg,x);
	    pg[x] = tmp;
	    
	    tmp = *b;
	    *(b++) = MASK(b,pb,x);
	    pb[x] = tmp;
	}
	r++;
	g++;
	b++;
    }

#undef MASK
    
#ifdef C_ALLOCA
    alloca(0);
#endif
    return True;
    
    outofmem:
    RErrorCode = RERR_NOMEMORY;
#ifdef C_ALLOCA
    alloca(0);
#endif
    return False;
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
    unsigned char *r, *g, *b, *a;

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];
    
    w = image->width;
    for (y=0; y<image->height - 1; y++) {
	for (x=0; x<image->width - 1; x++) {
	    v = *r + 2 * *(r + 1) + 2 * *(r + w) + *(r + w + 1);
	    *(r++) = v/6;
	    
	    v = *g + 2 * *(g + 1) + 2 * *(g + w) + *(g + w + 1);
	    *(g++) = v/6;
	    
	    v = *b + 2 * *(b + 1) + 2 * *(b + w) + *(b + w + 1);
	    *(b++) = v/6;
	}
	
	/* last column */
	v = 3 * *r + 3 * *(r + w);
	*(r++) = v/6;
	    
	v = 3 * *g + 3 * *(g + w);
	*(g++) = v/6;
	    
	v = 3 * *b + 3 * *(b + w);
	*(b++) = v/6;
    }
    
    /* last line */
    for (x=0; x<image->width - 1; x++) {
	v = 3 * *r + 3 * *(r + 1);
	*(r++) = v/6;
	
	v = 3 * *g + 3 * *(g + 1);
	*(g++) = v/6;
	    
	v = 3 * *b + 3 * *(b + 1);
	*(b++) = v/6;
    }

    return True;
}
#endif
