/* raster.c - main and other misc stuff 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "wraster.h"

#include <assert.h>


char *WRasterLibVersion="0.9";

int RErrorCode=RERR_NONE;



RImage*
RCreateImage(unsigned width, unsigned height, int alpha)
{
    RImage *image=NULL;
    int i;

    assert(width>0 && height>0);

    image = malloc(sizeof(RImage));
    if (!image) {
	RErrorCode = RERR_NOMEMORY;
        return NULL;
    }

    memset(image, 0, sizeof(RImage));
    image->width = width;
    image->height = height;
    for (i=0; i<3+(alpha?1:0); i++) {
	image->data[i] = malloc(width*height);
	if (!image->data[i])
	  goto error;
    }
    return image;

    error:
    for (i=0; i<4; i++) {
	if (image->data[i])
	  free(image->data[i]);
    }
    if (image)
      free(image);
    RErrorCode = RERR_NOMEMORY;
    return NULL;
}



RImage*
RCloneImage(RImage *image)
{
    RImage *new_image;

    assert(image!=NULL);
    
    new_image = RCreateImage(image->width, image->height, image->data[3]!=NULL);
    if (!new_image)
      return NULL;
    
    new_image->background = image->background;
    memcpy(new_image->data[0], image->data[0], image->width*image->height);
    memcpy(new_image->data[1], image->data[1], image->width*image->height);
    memcpy(new_image->data[2], image->data[2], image->width*image->height);
    if (image->data[3])
      memcpy(new_image->data[3], image->data[3], image->width*image->height);

    return new_image;
}


RImage*
RGetSubImage(RImage *image, int x, int y, unsigned width, unsigned height)
{
    int i, ofs;
    RImage *new_image;
    unsigned char *sr, *sg, *sb, *sa;
    unsigned char *dr, *dg, *db, *da;
    
    assert(image!=NULL);
    assert(x>=0 && y>=0);
    assert(x<image->width && y<image->height);
    assert(width>0 && height>0);
    
    if (x+width > image->width)
	width = image->width-x;
    if (y+height > image->height)
	height = image->height-y;

    new_image = RCreateImage(width, height, image->data[3]!=NULL);
    if (!new_image)
	return NULL;
    new_image->background = image->background;
    ofs = image->width*y+x;

    sr = image->data[0]+ofs;
    sg = image->data[1]+ofs;
    sb = image->data[2]+ofs;
    sa = image->data[3]+ofs;
    
    dr = new_image->data[0];
    dg = new_image->data[1];
    db = new_image->data[2];
    da = new_image->data[3];
    
    for (i=0; i<height; i++) {
	memcpy(dr, sr, width);
	memcpy(dg, sg, width);
	memcpy(db, sb, width);
	sr += image->width;
	sg += image->width;
	sb += image->width;
	dr += width;
	dg += width;
	db += width;
	if (da) {
	    memcpy(da, sa, width);
	    sa += image->width;
	    da += width;
	}
    }
    return new_image;
}


void 
RDestroyImage(RImage *image)
{
    int i;

    assert(image!=NULL);

    for (i=0; i<4; i++) {
	if (image->data[i])
	  free(image->data[i]);
    }
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
    register int i;
    unsigned char *dr, *dg, *db, *da;
    unsigned char *sr, *sg, *sb, *sa;
    int alpha, calpha;


    assert(image->width == src->width);
    assert(image->height == src->height);

    dr = image->data[0];
    dg = image->data[1];
    db = image->data[2];
    da = image->data[3];

    sr = src->data[0];
    sg = src->data[1];
    sb = src->data[2];
    sa = src->data[3];

    if (!sa) {
	memcpy(dr, sr, image->height*image->width);
	memcpy(dg, sg, image->height*image->width);
	memcpy(db, sb, image->height*image->width);
    } else {
	for (i=0; i<image->height*image->width; i++) {
	    alpha = *sa;
	    calpha = 255 - *sa;
	    *dr = (((int)*dr * calpha) + ((int)*sr * alpha))/256;
	    *dg = (((int)*dg * calpha) + ((int)*sg * alpha))/256;
	    *db = (((int)*db * calpha) + ((int)*sb * alpha))/256;
	    if (image->data[3])
		*da++ |= *sa;
	    dr++;    dg++;    db++;    
	    sr++;    sg++;    sb++;
	    sa++;
	}
    }
}




void
RCombineImagesWithOpaqueness(RImage *image, RImage *src, int opaqueness)
{
    register int i;
    unsigned char *dr, *dg, *db, *da;
    unsigned char *sr, *sg, *sb, *sa;
    int c_opaqueness;


    assert(image->width == src->width);
    assert(image->height == src->height);

    dr = image->data[0];
    dg = image->data[1];
    db = image->data[2];
    da = image->data[3];

    sr = src->data[0];
    sg = src->data[1];
    sb = src->data[2];
    sa = src->data[3];

    c_opaqueness = 255 - opaqueness;
#define OP opaqueness
    if (!src->data[3]) {
#define COP c_opaqueness
	for (i=0; i<image->width*image->height; i++) {
	    *dr = (((int)*dr *(int)COP) + ((int)*sr *(int)OP))/256;
	    *dg = (((int)*dg *(int)COP) + ((int)*sg *(int)OP))/256;
	    *db = (((int)*db *(int)COP) + ((int)*sb *(int)OP))/256;
	    dr++;    dg++;    db++;
	    sr++;    sg++;    sb++;
	}
#undef COP
    } else {
	int tmp;

	if (image->data[3]) {
	    for (i=0; i<image->width*image->height; i++) {
		tmp = (*sa * opaqueness)/256;
		*dr = (((int)*dr * (255-tmp)) + ((int)*sr * tmp))/256;
		*dg = (((int)*dg * (255-tmp)) + ((int)*sg * tmp))/256;
		*db = (((int)*db * (255-tmp)) + ((int)*sb * tmp))/256;
		*da |= tmp;

		dr++;    dg++;    db++;
		sr++;    sg++;    sb++;
		sa++;
		da++;
	    }
	} else {
	    for (i=0; i<image->width*image->height; i++) {
		tmp = (*sa * opaqueness)/256;
		*dr = (((int)*dr * (255-tmp)) + ((int)*sr * tmp))/256;
		*dg = (((int)*dg * (255-tmp)) + ((int)*sg * tmp))/256;
		*db = (((int)*db * (255-tmp)) + ((int)*sb * tmp))/256;

		dr++;    dg++;    db++;
		sr++;    sg++;    sb++;
		sa++;
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
    unsigned char *dr, *dg, *db;
    unsigned char *sr, *sg, *sb, *sa;
    int alpha, calpha;


    assert(dy < image->height);
    assert(dx < image->width);

    assert(sy + height <= src->height);
    assert(sx + width <= src->width);

    dr = image->data[0] + dy*(int)image->width + dx;
    dg = image->data[1] + dy*(int)image->width + dx;
    db = image->data[2] + dy*(int)image->width + dx;

    sr = src->data[0] + sy*(int)src->width + sx;
    sg = src->data[1] + sy*(int)src->width + sx;
    sb = src->data[2] + sy*(int)src->width + sx;
    sa = src->data[3] + sy*(int)src->width + sx;

    swi = src->width - width;
    dwi = image->width - width;

    if (height > image->height - dy)
	height = image->height - dy;

    if (!src->data[3]) {
	for (y=sy; y<height+sy; y++) {
	    for (x=sx; x<width+sx; x++) {
		*(dr++) = *(sr++);
		*(dg++) = *(sg++);
		*(db++) = *(sb++);
	    }
	    dr += dwi;	dg += dwi;	db += dwi;
	    sr += swi;	sg += swi;	sb += swi;
	}
    } else {
	for (y=sy; y<height+sy; y++) {
	    for (x=sx; x<width+sx; x++) {
		alpha = *sa;
		calpha = 255 - alpha;
		*dr = (((int)*dr * calpha) + ((int)*sr * alpha))/256;
		*dg = (((int)*dg * calpha) + ((int)*sg * alpha))/256;
		*db = (((int)*db * calpha) + ((int)*sb * alpha))/256;
		dr++;    dg++;    db++;
		sr++;    sg++;    sb++;
		sa++;
	    }
	    dr += dwi;	dg += dwi;	db += dwi;
	    sr += swi;	sg += swi;	sb += swi;
	    sa += swi;
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
    unsigned char *dr, *dg, *db;
    unsigned char *sr, *sg, *sb, *sa;

    assert(dy <= image->height);
    assert(dx <= image->width);

    assert(sy <= height);
    assert(sx <= width);

    dr = image->data[0] + dy*image->width + dx;
    dg = image->data[1] + dy*image->width + dx;
    db = image->data[2] + dy*image->width + dx;

    sr = src->data[0] + sy*src->width;
    sg = src->data[1] + sy*src->width;
    sb = src->data[2] + sy*src->width;
    sa = src->data[3] + sy*src->width;

    swi = src->width - width;
    dwi = image->width - width;

    /* clip */
    width -= sx;
    height -= sy;

    if (height > image->height - dy)
	height = image->height - dy;
    
    c_opaqueness = 255 - opaqueness;
#define OP opaqueness
    if (!src->data[3]) {
#define COP c_opaqueness
	for (y=0; y<height; y++) {
	    for (x=0; x<width; x++) {
		*dr = (((int)*dr *(int)COP) + ((int)*sr *(int)OP))/256;
		*dg = (((int)*dg *(int)COP) + ((int)*sg *(int)OP))/256;
		*db = (((int)*db *(int)COP) + ((int)*sb *(int)OP))/256;
		dr++;    dg++;    db++;
		sr++;    sg++;    sb++;
	    }
	    dr += dwi;	dg += dwi;	db += dwi;
	    sr += swi;	sg += swi;	sb += swi;
	}
#undef COP
    } else {
	int tmp;
	
	for (y=0; y<height; y++) {
	    for (x=0; x<width; x++) {
	        tmp= (*sa * opaqueness)/256;
		*dr = (((int)*dr *(int)(255-tmp)) + ((int)*sr *(int)tmp))/256;
		*dg = (((int)*dg *(int)(255-tmp)) + ((int)*sg *(int)tmp))/256;
		*db = (((int)*db *(int)(255-tmp)) + ((int)*sb *(int)tmp))/256;
		dr++;    dg++;    db++;
		sr++;    sg++;    sb++;
		sa++;
	    }
	    dr += dwi;	dg += dwi;	db += dwi;
	    sr += swi;	sg += swi;	sb += swi;
	    sa += swi;
	}
    }
#undef OP
}




void
RCombineImageWithColor(RImage *image, RColor *color)
{
    register int i;
    unsigned char *dr, *dg, *db, *da;
    int alpha, nalpha, r, g, b;
 
    dr = image->data[0];
    dg = image->data[1];
    db = image->data[2];
    da = image->data[3];
    
    if (!da) {
	/* Image has no alpha channel, so we consider it to be all 255 */
	return;
    }
    r = color->red;
    g = color->green;
    b = color->blue;

    for (i=0; i<image->width*image->height; i++) {
	alpha = *da;
	nalpha = 255 - alpha;

	*dr = (((int)*dr * alpha) + (r * nalpha))/256;
	*dg = (((int)*dg * alpha) + (g * nalpha))/256;
	*db = (((int)*db * alpha) + (b * nalpha))/256;
	dr++;    dg++;    db++;    da++;
    }
}


RImage*
RMakeTiledImage(RImage *tile, unsigned width, unsigned height)
{
    int x, y;
    unsigned w;
    unsigned long tile_size = tile->width * tile->height;
    unsigned long tx = 0;
    int have_alpha = (tile->data[3]!=NULL);
    RImage *image;
    unsigned char *sr, *sg, *sb, *sa;
    unsigned char *dr, *dg, *db, *da;

    if (width == tile->width && height == tile->height)
        image = RCloneImage(tile);
    else if (width <= tile->width && height <= tile->height)
        image = RGetSubImage(tile, 0, 0, width, height);
    else {

        image = RCreateImage(width, height, have_alpha);

        dr = image->data[0];
        dg = image->data[1];
        db = image->data[2];
        da = image->data[3];

        sr = tile->data[0];
        sg = tile->data[1];
        sb = tile->data[2];
        sa = tile->data[3];

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x += tile->width) {

                w = (width - x < tile->width) ? width - x : tile->width;

                memcpy(dr, sr+tx, w);
                memcpy(dg, sg+tx, w);
                memcpy(db, sb+tx, w);
                if (have_alpha) {
                    memcpy(da, sa+tx, w);
                    da += w;
                }

                dr += w;
                dg += w;
                db += w;

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
