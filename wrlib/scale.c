/* scale.c - image scaling
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

