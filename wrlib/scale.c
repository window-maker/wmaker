/* scale.c - image scaling
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 1997, 1988, 1999 Alfredo K. Kojima
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
#include <math.h>

#ifndef PI
#define PI 	3.131592
#endif

#include <assert.h>

#include "wraster.h"


/*
 *----------------------------------------------------------------------
 * RScaleImage--
 * 	Creates a scaled copy of an image.
 * 
 * Returns:
 * 	The new scaled image.
 * 
 *---------------------------------------------------------------------- 
 */
RImage*
RScaleImage(RImage *image, unsigned new_width, unsigned new_height)
{
    int ox;
    int px, py;
    register int x, y, t;
    int dx, dy;
    unsigned char *sr, *sg, *sb, *sa;
    unsigned char *dr, *dg, *db, *da;
    RImage *img;

    assert(new_width >= 0 && new_height >= 0);

    if (new_width == image->width && new_height == image->height)
	return RCloneImage(image);

    img = RCreateImage(new_width, new_height, image->data[3]!=NULL);
    
    if (!img)
      return NULL;

    /* fixed point math idea taken from Imlib by 
     * Carsten Haitzler (Rasterman) */
    dx = (image->width<<16)/new_width;
    dy = (image->height<<16)/new_height;

    py = 0;
    
    dr = img->data[0];
    dg = img->data[1];
    db = img->data[2];
    da = img->data[3];

    if (image->data[3]!=NULL) {
	int ot;
	ot = -1;
	for (y=0; y<new_height; y++) {
	    t = image->width*(py>>16);

	    sr = image->data[0]+t;
	    sg = image->data[1]+t;
	    sb = image->data[2]+t;
	    sa = image->data[3]+t;

	    ot = t;
	    ox = 0;
	    px = 0;
	    for (x=0; x<new_width; x++) {
		px += dx;

		*(dr++) = *sr;
		*(dg++) = *sg;
		*(db++) = *sb;
		*(da++) = *sa;
		
		t = (px - ox)>>16;
		ox += t<<16;
		    
		sr += t;
		sg += t;
		sb += t;
		sa += t;
	    }
	    py += dy;
	}
    } else {
	int ot;
	ot = -1;
	for (y=0; y<new_height; y++) {
	    t = image->width*(py>>16);

	    sr = image->data[0]+t;
	    sg = image->data[1]+t;
	    sb = image->data[2]+t;
	    
	    ot = t;
	    ox = 0;
	    px = 0;
	    for (x=0; x<new_width; x++) {
		px += dx;
		
		*(dr++) = *sr;
		*(dg++) = *sg;
		*(db++) = *sb;
		
		t = (px-ox)>>16;
		ox += t<<16;
		
		sr += t;
		sg += t;
		sb += t;
	    }
	    py += dy;
	}
    }
    
    return img;
}




/*
 * Filtered Image Rescaling code copy/pasted from
 * Graphics Gems III
 * Public Domain 1991 by Dale Schumacher
 */


/*
 *	filter function definitions
 */

#define	filter_support		(1.0)

static double
filter(t)
double t;
{
    /* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
    if(t < 0.0) t = -t;
    if(t < 1.0) return((2.0 * t - 3.0) * t * t + 1.0);
    return(0.0);
}

#define	box_support		(0.5)

static double
box_filter(t)
double t;
{
    if((t > -0.5) && (t <= 0.5)) return(1.0);
    return(0.0);
}

#define	triangle_support	(1.0)

static double
triangle_filter(t)
double t;
{
    if(t < 0.0) t = -t;
    if(t < 1.0) return(1.0 - t);
    return(0.0);
}

#define	bell_support		(1.5)

static double
bell_filter(t)		/* box (*) box (*) box */
double t;
{
    if(t < 0) t = -t;
    if(t < .5) return(.75 - (t * t));
    if(t < 1.5) {
	t = (t - 1.5);
	return(.5 * (t * t));
    }
    return(0.0);
}

#define	B_spline_support	(2.0)

static double
B_spline_filter(t)	/* box (*) box (*) box (*) box */
double t;
{
    double tt;
    
    if(t < 0) t = -t;
    if(t < 1) {
	tt = t * t;
	return((.5 * tt * t) - tt + (2.0 / 3.0));
    } else if(t < 2) {
	t = 2 - t;
	return((1.0 / 6.0) * (t * t * t));
    }
    return(0.0);
}

static double
sinc(x)
double x;
{
    x *= PI;
    if(x != 0) return(sin(x) / x);
    return(1.0);
}

#define	Lanczos3_support	(3.0)

static double
Lanczos3_filter(t)
double t;
{
    if(t < 0) t = -t;
    if(t < 3.0) return(sinc(t) * sinc(t/3.0));
    return(0.0);
}

#define	Mitchell_support	(2.0)

#define	B	(1.0 / 3.0)
#define	C	(1.0 / 3.0)

static double
Mitchell_filter(t)
double t;
{
    double tt;
    
    tt = t * t;
    if(t < 0) t = -t;
    if(t < 1.0) {
	t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt))
	     + ((-18.0 + 12.0 * B + 6.0 * C) * tt)
	     + (6.0 - 2 * B));
	return(t / 6.0);
    } else if(t < 2.0) {
	t = (((-1.0 * B - 6.0 * C) * (t * tt))
	     + ((6.0 * B + 30.0 * C) * tt)
	     + ((-12.0 * B - 48.0 * C) * t)
	     + (8.0 * B + 24 * C));
	return(t / 6.0);
    }
    return(0.0);
}

/*
 *	image rescaling routine
 */

typedef struct {
    int pixel;
    double	weight;
} CONTRIB;

typedef struct {
    int	n;		/* number of contributors */
    CONTRIB	*p;		/* pointer to list of contributions */
} CLIST;

CLIST	*contrib;		/* array of contribution lists */

/* clamp the input to the specified range */
#define CLAMP(v,l,h)    ((v)<(l) ? (l) : (v) > (h) ? (h) : v)


static double (*filterf)() = Mitchell_filter;
static double fwidth = Mitchell_support;


RImage*
RSmoothScaleImage(RImage *src, int newWidth, int newHeight)
{
    
    RImage *tmp;		       /* intermediate image */
    double xscale, yscale;	       /* zoom scale factors */
    int i, j, k;		       /* loop variables */
    int n;			       /* pixel number */
    double center, left, right;	       /* filter calculation variables */
    double width, fscale;	       /* filter calculation variables */
    double rweight, gweight, bweight;
    RImage *dst;
    unsigned char *rp, *gp, *bp;
    unsigned char *rsp, *gsp, *bsp;

    dst = RCreateImage(newWidth, newHeight, False);

    /* create intermediate image to hold horizontal zoom */
    tmp = RCreateImage(dst->width, src->height, False);
    xscale = (double)newWidth / (double)src->width;
    yscale = (double)newHeight / (double)src->height;

    /* pre-calculate filter contributions for a row */
    contrib = (CLIST *)calloc(newWidth, sizeof(CLIST));
    if (xscale < 1.0) {
	width = fwidth / xscale;
	fscale = 1.0 / xscale;
	for (i = 0; i < newWidth; ++i) {
	    contrib[i].n = 0;
	    contrib[i].p = (CONTRIB *)calloc((int)(width * 2 + 1),
					     sizeof(CONTRIB));
	    center = (double) i / xscale;
	    left = ceil(center - width);
	    right = floor(center + width);
	    for(j = left; j <= right; ++j) {
		rweight = center - (double) j;
		rweight = (*filterf)(rweight / fscale) / fscale;
		if(j < 0) {
		    n = -j;
		} else if(j >= src->width) {
		    n = (src->width - j) + src->width - 1;
		} else {
		    n = j;
		}
		k = contrib[i].n++;
		contrib[i].p[k].pixel = n;
		contrib[i].p[k].weight = rweight;
	    }
	}
    } else {
	for(i = 0; i < newWidth; ++i) {
	    contrib[i].n = 0;
	    contrib[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
					     sizeof(CONTRIB));
	    center = (double) i / xscale;
	    left = ceil(center - fwidth);
	    right = floor(center + fwidth);
	    for(j = left; j <= right; ++j) {
		rweight = center - (double) j;
		rweight = (*filterf)(rweight);
		if(j < 0) {
		    n = -j;
		} else if(j >= src->width) {
		    n = (src->width - j) + src->width - 1;
		} else {
		    n = j;
		}
		k = contrib[i].n++;
		contrib[i].p[k].pixel = n;
		contrib[i].p[k].weight = rweight;
	    }
	}
    }
    
    /* apply filter to zoom horizontally from src to tmp */
    rp = tmp->data[0];
    gp = tmp->data[1];
    bp = tmp->data[2];

    for(k = 0; k < tmp->height; ++k) {
	rsp = src->data[0] + src->width*k;
	gsp = src->data[1] + src->width*k;
	bsp = src->data[2] + src->width*k;

	for(i = 0; i < tmp->width; ++i) {
	    rweight = gweight = bweight = 0.0;
	    for(j = 0; j < contrib[i].n; ++j) {
		rweight += rsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
		gweight += gsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
		bweight += bsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
	    }
	    *rp++ = CLAMP(rweight, 0, 255);
	    *gp++ = CLAMP(gweight, 0, 255);
	    *bp++ = CLAMP(bweight, 0, 255);
	}
    }

    /* free the memory allocated for horizontal filter weights */
    for(i = 0; i < tmp->width; ++i) {
	free(contrib[i].p);
    }
    free(contrib);
    
    /* pre-calculate filter contributions for a column */
    contrib = (CLIST *)calloc(dst->height, sizeof(CLIST));
    if(yscale < 1.0) {
	width = fwidth / yscale;
	fscale = 1.0 / yscale;
	for(i = 0; i < dst->height; ++i) {
	    contrib[i].n = 0;
	    contrib[i].p = (CONTRIB *)calloc((int) (width * 2 + 1),
					     sizeof(CONTRIB));
	    center = (double) i / yscale;
	    left = ceil(center - width);
	    right = floor(center + width);
	    for(j = left; j <= right; ++j) {
		rweight = center - (double) j;
		rweight = (*filterf)(rweight / fscale) / fscale;
		if(j < 0) {
		    n = -j;
		} else if(j >= tmp->height) {
		    n = (tmp->height - j) + tmp->height - 1;
		} else {
		    n = j;
		}
		k = contrib[i].n++;
		contrib[i].p[k].pixel = n;
		contrib[i].p[k].weight = rweight;
	    }
	}
    } else {
	for(i = 0; i < dst->height; ++i) {
	    contrib[i].n = 0;
	    contrib[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
					     sizeof(CONTRIB));
	    center = (double) i / yscale;
	    left = ceil(center - fwidth);
	    right = floor(center + fwidth);
	    for(j = left; j <= right; ++j) {
		rweight = center - (double) j;
		rweight = (*filterf)(rweight);
		if(j < 0) {
		    n = -j;
		} else if(j >= tmp->height) {
		    n = (tmp->height - j) + tmp->height - 1;
		} else {
		    n = j;
		}
		k = contrib[i].n++;
		contrib[i].p[k].pixel = n;
		contrib[i].p[k].weight = rweight;
	    }
	}
    }

    /* apply filter to zoom vertically from tmp to dst */
    rsp = malloc(tmp->height);
    gsp = malloc(tmp->height);
    bsp = malloc(tmp->height);

    for(k = 0; k < newWidth; ++k) {
	rp = dst->data[0] + k;
	gp = dst->data[1] + k;
	bp = dst->data[2] + k;

	/* copy a column into a row */
	{
	    int i;
	    unsigned char *p, *d;

	    d = rsp;
	    for(i = tmp->height, p = tmp->data[0] + k; i-- > 0; 
		p += tmp->width) {
		*d++ = *p;
	    }
	    d = gsp;
	    for(i = tmp->height, p = tmp->data[1] + k; i-- > 0; 
		p += tmp->width) {
		*d++ = *p;
	    }
	    d = bsp;
	    for(i = tmp->height, p = tmp->data[2] + k; i-- > 0;
		p += tmp->width) {
		*d++ = *p;
	    }
	}
	for(i = 0; i < newHeight; ++i) {
	    rweight = gweight = bweight = 0.0;
	    for(j = 0; j < contrib[i].n; ++j) {
		rweight += rsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
		gweight += gsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
		bweight += bsp[contrib[i].p[j].pixel] * contrib[i].p[j].weight;
	    }
	    *rp = CLAMP(rweight, 0, 255);
	    *gp = CLAMP(gweight, 0, 255);
	    *bp = CLAMP(bweight, 0, 255);
	    rp += newWidth;
	    gp += newWidth;
	    bp += newWidth;
	}
    }
    free(rsp);
    free(gsp);
    free(bsp);
    
    /* free the memory allocated for vertical filter weights */
    for(i = 0; i < dst->height; ++i) {
	free(contrib[i].p);
    }
    free(contrib);

    RDestroyImage(tmp);

    return dst;
}

