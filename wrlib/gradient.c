/* gradient.c - renders gradients
 *
 *  Raster graphics library
 *
 *  Copyright (c) 1997-2000 Alfredo K. Kojima
 *  Copyright (c) 1998-2000 Dan Pascu
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

#include <assert.h>

#include "wraster.h"


static RImage *renderHGradient(unsigned width, unsigned height, 
			       int r0, int g0, int b0, 
			       int rf, int gf, int bf);
static RImage *renderVGradient(unsigned width, unsigned height, 
			       int r0, int g0, int b0, 
			       int rf, int gf, int bf);
static RImage *renderDGradient(unsigned width, unsigned height, 
			       int r0, int g0, int b0, 
			       int rf, int gf, int bf);

static RImage *renderMHGradient(unsigned width, unsigned height, 
				RColor **colors, int count);
static RImage *renderMVGradient(unsigned width, unsigned height,
				RColor **colors, int count);
static RImage *renderMDGradient(unsigned width, unsigned height, 
				RColor **colors, int count);

RImage*
RRenderMultiGradient(unsigned width, unsigned height, RColor **colors, int style)
{
    int count;

    count = 0;
    while (colors[count]!=NULL) count++;

    if (count > 2) {
	switch (style) {
	 case RHorizontalGradient:
	    return renderMHGradient(width, height, colors, count);
	 case RVerticalGradient:
	    return renderMVGradient(width, height, colors, count);
	 case RDiagonalGradient:
	    return renderMDGradient(width, height, colors, count);
	}
    } else if (count > 1) {
	return RRenderGradient(width, height, colors[0], colors[1], style);
    } else if (count > 0) {
	return RRenderGradient(width, height, colors[0], colors[0], style);
    }
    assert(0);
    return NULL;
}



RImage*
RRenderGradient(unsigned width, unsigned height, RColor *from, RColor *to, 
		int style)
{
    switch (style) {
     case RHorizontalGradient:
	return renderHGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);
     case RVerticalGradient:
	return renderVGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);

     case RDiagonalGradient:
	return renderDGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);
    }
    assert(0);
    return NULL;
}


/*
 *----------------------------------------------------------------------
 * renderHGradient--
 * 	Renders a horizontal linear gradient of the specified size in the
 * RImage format with a border of the specified type. 
 * 
 * Returns:
 * 	A 24bit RImage with the gradient (no alpha channel).
 * 
 * Side effects:
 * 	None
 *---------------------------------------------------------------------- 
 */
static RImage*
renderHGradient(unsigned width, unsigned height, int r0, int g0, int b0, 
		int rf, int gf, int bf)
{    
    int i;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *ptr;

    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    ptr = image->data;

    r = r0 << 16;
    g = g0 << 16;
    b = b0 << 16;
    
    dr = ((rf-r0)<<16)/(int)width;
    dg = ((gf-g0)<<16)/(int)width;
    db = ((bf-b0)<<16)/(int)width;
    /* render the first line */
    for (i=0; i<width; i++) {
	*(ptr++) = (unsigned char)(r>>16);
	*(ptr++) = (unsigned char)(g>>16);
	*(ptr++) = (unsigned char)(b>>16);
	r += dr;
	g += dg;
	b += db;
    }

    /* copy the first line to the other lines */
    for (i=1; i<height; i++) {
	memcpy(&(image->data[i*width*3]), image->data, width*3);
    }
    return image;
}



/*
 *----------------------------------------------------------------------
 * renderVGradient--
 *      Renders a vertical linear gradient of the specified size in the
 * RImage format with a border of the specified type.
 *
 * Returns:
 *      A 24bit RImage with the gradient (no alpha channel).
 *
 * Side effects:
 *      None
 *----------------------------------------------------------------------
 */
static RImage*
renderVGradient(unsigned width, unsigned height, int r0, int g0, int b0,
		int rf, int gf, int bf)
{
    int i, j;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *ptr;
    unsigned int *iptr;
    unsigned char rr, gg, bb;

    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }    
    iptr = (unsigned int*)ptr = image->data;

    r = r0<<16;
    g = g0<<16;
    b = b0<<16;

    dr = ((rf-r0)<<16)/(int)height;
    dg = ((gf-g0)<<16)/(int)height;
    db = ((bf-b0)<<16)/(int)height;

    for (i=0; i<height; i++) {
	rr = r>>16;
	gg = g>>16;
	bb = b>>16;
	for (j=0; j<width/8; j++) {
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	    *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	}
	switch (width%8) {
	 case 7: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 6: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 5: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 4: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 3: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 2: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	 case 1: *(ptr++) = rr; *(ptr++) = gg; *(ptr++) = bb;
	}
        r+=dr;
        g+=dg;
        b+=db;
    }
    return image;
}


/*
 *----------------------------------------------------------------------
 * renderDGradient--
 *      Renders a diagonal linear gradient of the specified size in the
 * RImage format with a border of the specified type.
 *
 * Returns:
 *      A 24bit RImage with the gradient (no alpha channel).
 *
 * Side effects:
 *      None
 *----------------------------------------------------------------------
 */


static RImage*
renderDGradient(unsigned width, unsigned height, int r0, int g0, int b0,
		int rf, int gf, int bf)
{
    RImage *image, *tmp;
    int j;
    float a, offset;
    char *ptr;

    if (width == 1)
        return renderVGradient(width, height, r0, g0, b0, rf, gf, bf);
    else if (height == 1)
        return renderHGradient(width, height, r0, g0, b0, rf, gf, bf);

    image = RCreateImage(width, height, False);
    if (!image) {
        return NULL;
    }

    tmp = renderHGradient(2*width-1, 1, r0, g0, b0, rf, gf, bf);
    if (!tmp) {
        RDestroyImage(image);
        return NULL;
    }

    ptr = tmp->data;

    a = ((float)(width - 1))/((float)(height - 1));
    width = width * 3;

    /* copy the first line to the other lines with corresponding offset */
    for (j=0, offset=0.0; j<width*height; j += width) {
	memcpy(&(image->data[j]), &ptr[3*(int)offset], width);
        offset += a;
    }

    RDestroyImage(tmp);
    return image;
}


static RImage*
renderMHGradient(unsigned width, unsigned height, RColor **colors, int count)
{
    int i, j, k;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *ptr;
    unsigned width2;
    
    
    assert(count > 2);
    
    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    ptr = image->data;
    
    if (count > width)
	count = width;
    
    if (count > 1)
        width2 = width/(count-1);
    else
        width2 = width;
    
    k = 0;

    r = colors[0]->red << 16;
    g = colors[0]->green << 16;
    b = colors[0]->blue << 16;

    /* render the first line */
    for (i=1; i<count; i++) {
	dr = ((int)(colors[i]->red   - colors[i-1]->red)  <<16)/(int)width2;
	dg = ((int)(colors[i]->green - colors[i-1]->green)<<16)/(int)width2;
	db = ((int)(colors[i]->blue  - colors[i-1]->blue) <<16)/(int)width2;
	for (j=0; j<width2; j++) {
	    *ptr++ = (unsigned char)(r>>16);
	    *ptr++ = (unsigned char)(g>>16);
	    *ptr++ = (unsigned char)(b>>16);
	    r += dr;
	    g += dg;
	    b += db;
	    k++;
	}
	r = colors[i]->red << 16;
	g = colors[i]->green << 16;
	b = colors[i]->blue << 16;
    }
    for (j=k; j<width; j++) {
	*ptr++ = (unsigned char)(r>>16);
	*ptr++ = (unsigned char)(g>>16);
	*ptr++ = (unsigned char)(b>>16);
    }
    
    /* copy the first line to the other lines */
    for (i=1; i<height; i++) {
	memcpy(&(image->data[i*width*3]), image->data, width*3);
    }
    return image;
}




static RImage*
renderMVGradient(unsigned width, unsigned height, RColor **colors, int count)
{
    int i, j, k;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *ptr, *tmp;
    unsigned height2;
    int x;
    unsigned char rr, gg, bb;
    

    assert(count > 2);
    
    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    ptr = image->data;
    
    if (count > height)
	count = height;
    
    if (count > 1)
        height2 = height/(count-1);
    else
        height2 = height;
    
    k = 0;

    r = colors[0]->red << 16;
    g = colors[0]->green << 16;
    b = colors[0]->blue << 16;

    for (i=1; i<count; i++) {
	dr = ((int)(colors[i]->red   - colors[i-1]->red)  <<16)/(int)height2;
	dg = ((int)(colors[i]->green - colors[i-1]->green)<<16)/(int)height2;
	db = ((int)(colors[i]->blue  - colors[i-1]->blue) <<16)/(int)height2;

	for (j=0; j<height2; j++) {
	    rr = r>>16;
	    gg = g>>16;
	    bb = b>>16;

	    for (x=0; x<width/4; x++) {
		*ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
		*ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
		*ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
		*ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
	    }
	    switch (width%4) {
	     case 3: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
	     case 2: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
	     case 1: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
	    }
	    r += dr;
	    g += dg;
	    b += db;
	    k++;
	}
	r = colors[i]->red << 16;
	g = colors[i]->green << 16;
	b = colors[i]->blue << 16;
    }

    rr = r>>16;
    gg = g>>16;
    bb = b>>16;

    if (k<height) {
        tmp = ptr;
        for (x=0; x<width/4; x++) {
            *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
            *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
            *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
            *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
        }
        switch (width%4) {
        case 3: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
        case 2: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
        case 1: *ptr++ = rr; *ptr++ = gg; *ptr++ = bb;
        default: break;
        }

        for (j=k+1; j<height; j++) {
            memcpy(ptr, tmp, width*3);
            ptr += width*3;
        }
    }
    
    return image;
}


static RImage*
renderMDGradient(unsigned width, unsigned height, RColor **colors, int count)
{
    RImage *image, *tmp;
    float a, offset;
    int j;
    unsigned char *ptr;

    assert(count > 2);

    if (width == 1)
        return renderMVGradient(width, height, colors, count);
    else if (height == 1)
        return renderMHGradient(width, height, colors, count);

    image = RCreateImage(width, height, False);
    if (!image) {
        return NULL;
    }

    if (count > width)
        count = width;
    if (count > height)
        count = height;
 
    if (count > 2)
	tmp = renderMHGradient(2*width-1, 1, colors, count);
    else
	tmp = renderHGradient(2*width-1, 1, colors[0]->red<<8, 
			      colors[0]->green<<8, colors[0]->blue<<8,
			      colors[1]->red<<8, colors[1]->green<<8,
			      colors[1]->blue<<8);

    if (!tmp) {
        RDestroyImage(image);
        return NULL;
    }
    ptr = tmp->data;

    a = ((float)(width - 1))/((float)(height - 1));
    width = width * 3;

    /* copy the first line to the other lines with corresponding offset */
    for (j=0, offset=0; j<width*height; j += width) {
	memcpy(&(image->data[j]), &ptr[3*(int)offset], width);
        offset += a;
    }
    RDestroyImage(tmp);
    return image;
}
