/* raster.c - main and other misc stuff 
 * 
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

#include <assert.h>


char *WRasterLibVersion="0.9";

int RErrorCode=RERR_NONE;


#define HAS_ALPHA(I)	((I)->format == RRGBAFormat)


RImage*
RCreateImage(unsigned width, unsigned height, int alpha)
{
    RImage *image=NULL;
    
    assert(width>0 && height>0);

    image = malloc(sizeof(RImage));
    if (!image) {
	RErrorCode = RERR_NOMEMORY;
        return NULL;
    }

    memset(image, 0, sizeof(RImage));
    image->width = width;
    image->height = height;
    if (alpha) {
	image->format = RRGBAFormat;
    } else {
	image->format = RRGBFormat;
    }
    /* the +4 is to give extra bytes at the end of the buffer,
     * so that we can optimize image conversion for MMX(tm).. see convert.c
     */
    image->data = malloc(width * height * (alpha ? 4 : 3) + 4);
    if (!image->data) {
	RErrorCode = RERR_NOMEMORY;
	free(image);
	image = NULL;
    }

    return image;
}



RImage*
RCloneImage(RImage *image)
{
    RImage *new_image;

    assert(image!=NULL);
    
    new_image = RCreateImage(image->width, image->height, HAS_ALPHA(image));
    if (!new_image)
      return NULL;
    
    new_image->background = image->background;
    memcpy(new_image->data, image->data,
	   image->width*image->height*(HAS_ALPHA(image) ? 4 : 3));

    return new_image;
}


RImage*
RGetSubImage(RImage *image, int x, int y, unsigned width, unsigned height)
{
    int i, ofs;
    RImage *new_image;
    unsigned total_line_size, line_size;
    
    assert(image!=NULL);
    assert(x>=0 && y>=0);
    assert(x<image->width && y<image->height);
    assert(width>0 && height>0);
    
    if (x+width > image->width)
	width = image->width-x;
    if (y+height > image->height)
	height = image->height-y;

    new_image = RCreateImage(width, height, HAS_ALPHA(image));

    if (!new_image)
	return NULL;
    new_image->background = image->background;

    total_line_size = image->width * (HAS_ALPHA(image) ? 4 : 3);
    line_size = width * (HAS_ALPHA(image) ? 4 : 3);

    ofs = x*(HAS_ALPHA(image) ? 4 : 3);

    for (i=0; i<height; i++) {
	memcpy(&new_image->data[i*line_size], 
	       &image->data[i*total_line_size+ofs], line_size);
    }
    return new_image;
}


void 
RDestroyImage(RImage *image)
{
    assert(image!=NULL);

    free(image->data);
    free(image);
}


/*
 *---------------------------------------------------------------------- 
 * RCombineImages-
 * 	Combines two equal sized images with alpha image. The second
 * image will be placed on top of the first one.
 *---------------------------------------------------------------------- 
 */
void
RCombineImages(RImage *image, RImage *src)
{
    assert(image->width == src->width);
    assert(image->height == src->height);

    if (!HAS_ALPHA(src)) {
	if (!HAS_ALPHA(image)) {
	    memcpy(image->data, src->data, image->height*image->width*3);
	} else {
	    int x, y;
	    unsigned char *d, *s;
	    
	    d = image->data;
	    s = src->data;
	    for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {
		    *d++ = *s++;
		    *d++ = *s++;
		    *d++ = *s++;
		    d++;
		}
	    }
	}
    } else {
	register int i;
	unsigned char *d;
	unsigned char *s;
	int alpha, calpha;

	d = image->data;
	s = src->data;

	if (!HAS_ALPHA(image)) {
	    for (i=0; i<image->height*image->width; i++) {
		alpha = *(s+3);
		calpha = 255 - alpha;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		s++;
	    }
	} else {
	    for (i=0; i<image->height*image->width; i++) {
		alpha = *(s+3);
		calpha = 255 - alpha;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		d++; s++;
		*d++ |= *s++;
	    }
	}
    }
}




void
RCombineImagesWithOpaqueness(RImage *image, RImage *src, int opaqueness)
{
    register int i;
    unsigned char *d;
    unsigned char *s;
    int c_opaqueness;

    assert(image->width == src->width);
    assert(image->height == src->height);

    d = image->data;
    s = src->data;

    c_opaqueness = 255 - opaqueness;
#define OP opaqueness
    if (!HAS_ALPHA(src)) {
	int dalpha = HAS_ALPHA(image);
#define COP c_opaqueness
	for (i=0; i < image->width*image->height; i++) {
	    *d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
	    d++; s++;
	    *d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
	    d++; s++;
	    *d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
	    d++; s++;
	    if (dalpha) {
		d++;
	    }
	}
#undef COP
    } else {
	int tmp;

	if (!HAS_ALPHA(image)) {
	    for (i=0; i<image->width*image->height; i++) {
		tmp = (*(s+3) * opaqueness)/256;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		s++;
	    }
	} else {
	    for (i=0; i<image->width*image->height; i++) {
		tmp = (*(s+3) * opaqueness)/256;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		*d = (((int)*d * (255-tmp)) + ((int)*s * tmp))/256;
		d++; s++;
		*d |= tmp;
		d++; s++;
	    }
	}
    }
#undef OP
}


void
RCombineArea(RImage *image, RImage *src, int sx, int sy, unsigned width,
	     unsigned height, int dx, int dy)
{
    int x, y, dwi, swi;
    unsigned char *d;
    unsigned char *s;
    int alpha, calpha;


    assert(dy < image->height);
    assert(dx < image->width);

    assert(sy + height <= src->height);
    assert(sx + width <= src->width);

    
    if (height > image->height - dy)
	height = image->height - dy;

    if (!HAS_ALPHA(src)) {
	if (!HAS_ALPHA(image)) {
	    swi = (src->width - width) * 3;
	    dwi = (image->width - width) * 3;

	    d = image->data + dy*(int)image->width*3 + dx;
	    s = src->data + sy*(int)src->width*3 + sx;

	    for (y=0; y < height; y++) {
		memcpy(d, s, width*3);
		d += dwi;
		s += swi;
	    }
	} else {
	    swi = (src->width - width) * 3;
	    dwi = (image->width - width) * 4;

	    d = image->data + dy*(int)image->width*4 + dx;
	    s = src->data + sy*(int)src->width*3 + sx;

	    for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
		    *d++ = *s++;
		    *d++ = *s++;
		    *d++ = *s++;
		    d++;
		}
		d += dwi;
		s += swi;
	    }
	}
    } else {
	int dalpha = HAS_ALPHA(image);

	swi = (src->width - width) * 4;
	s = src->data + sy*(int)src->width*4 + sx;
	if (!HAS_ALPHA(image)) {
	    dwi = (image->width - width) * 3;
	    d = image->data + (dy*(int)image->width+dx)*3;
	} else {
	    dwi = (image->width - width) * 4;
	    d = image->data + (dy*(int)image->width+dx)*4;
	}

	for (y=0; y < height; y++) {
	    for (x=0; x < width; x++) {
		alpha = *(s+3);
		calpha = 255 - alpha;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		s++; d++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		s++; d++;
		*d = (((int)*d * calpha) + ((int)*s * alpha))/256;
		s++; d++;
		s++;
		if (dalpha)
		    d++;
	    }
	    d += dwi;
	    s += swi;
	}
    }
}


void
RCombineAreaWithOpaqueness(RImage *image, RImage *src, int sx, int sy, 
			   unsigned width, unsigned height, int dx, int dy,
			   int opaqueness)
{
    int x, y, dwi, swi;
    int c_opaqueness;
    unsigned char *d;
    unsigned char *s;
    int dalpha = HAS_ALPHA(image);
    int dch = (dalpha ? 4 : 3);

    assert(dy <= image->height);
    assert(dx <= image->width);

    assert(sy <= height);
    assert(sx <= width);


    /* clip */
    width -= sx;
    height -= sy;

    if (height > image->height - dy)
	height = image->height - dy;

    d = image->data + dy*image->width*dch + dx;
    dwi = (image->width - width)*dch;

    c_opaqueness = 255 - opaqueness;
#define OP opaqueness
    if (!HAS_ALPHA(src)) {
#define COP c_opaqueness

	s = src->data + sy*src->width*3;	
	swi = (src->width - width) * 3;
	
	for (y=0; y < height; y++) {
	    for (x=0; x < width; x++) {
		*d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
		s++; d++;
		*d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
		s++; d++;
		*d = (((int)*d *(int)COP) + ((int)*s *(int)OP))/256;
		s++; d++;
		if (dalpha)
		    d++;
	    }
	    d += dwi; s += swi;
	}
#undef COP
    } else {
        int tmp;

	s = src->data + sy*src->width*4;
	swi = (src->width - width) * 4;
	
	for (y=0; y < height; y++) {
	    for (x=0; x < width; x++) {
	        tmp= (*(s+3) * opaqueness)/256;
		*d = (((int)*d *(int)(255-tmp)) + ((int)*s *(int)tmp))/256;
		d++; s++;
		*d = (((int)*d *(int)(255-tmp)) + ((int)*s *(int)tmp))/256;
		d++; s++;
		*d = (((int)*d *(int)(255-tmp)) + ((int)*s *(int)tmp))/256;
		d++; s++;
		s++;
		if (dalpha)
		    d++;
	    }
	    d += dwi; s += swi;
	}
    }
#undef OP
}




void
RCombineImageWithColor(RImage *image, RColor *color)
{
    register int i;
    unsigned char *d;
    int alpha, nalpha, r, g, b;
 
    d = image->data;
    
    if (!HAS_ALPHA(image)) {
	/* Image has no alpha channel, so we consider it to be all 255.
	 * Thus there are no transparent parts to be filled. */
	return;
    }
    r = color->red;
    g = color->green;
    b = color->blue;

    for (i=0; i < image->width*image->height; i++) {
	alpha = *(d+3);
	nalpha = 255 - alpha;

	*d = (((int)*d * alpha) + (r * nalpha))/256;
	d++;
	*d = (((int)*d * alpha) + (g * nalpha))/256;
	d++;
	*d = (((int)*d * alpha) + (b * nalpha))/256;
	d++;
	d++;
    }
}




RImage*
RMakeTiledImage(RImage *tile, unsigned width, unsigned height)
{
    int x, y;
    unsigned w;
    unsigned long tile_size = tile->width * tile->height;
    unsigned long tx = 0;
    RImage *image;
    unsigned char *s, *d;

    if (width == tile->width && height == tile->height)
        image = RCloneImage(tile);
    else if (width <= tile->width && height <= tile->height)
        image = RGetSubImage(tile, 0, 0, width, height);
    else {
	int has_alpha = HAS_ALPHA(tile);

        image = RCreateImage(width, height, has_alpha);

        d = image->data;
        s = tile->data;

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x += tile->width) {

                w = (width - x < tile->width) ? width - x : tile->width;

		if (has_alpha) {
		    w *= 3;
		    memcpy(d, s+tx*3, w);
		} else {
		    w *= 4;
		    memcpy(d, s+tx*4, w);
		}
                d += w;
            }
	    
            tx = (tx + tile->width) % tile_size;
        }
    }
    return image;
}


RImage*
RMakeCenteredImage(RImage *image, unsigned width, unsigned height, RColor *color)
{
    int x, y, w, h, sx, sy;
    RImage *tmp;

    tmp = RCreateImage(width, height, False);
    if (!tmp) {
        return NULL;
    }

    RClearImage(tmp, color);

    if (image->height < height) {
        h = image->height;
        y = (height - h)/2;
        sy = 0;
    } else {
        sy = (image->height - height)/2;
        y = 0;
        h = height;
    }
    if (image->width < width) {
        w = image->width;
        x = (width - w)/2;
        sx = 0;
    } else {
        sx = (image->width - width)/2;
        x = 0;
        w = width;
    }
    RCombineArea(tmp, image, sx, sy, w, h, x, y);

    return tmp;
}
