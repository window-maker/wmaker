/* xpixmap.c - Make RImage from Pixmap or XImage
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "wraster.h"



static int
get_shifts(unsigned long mask)
{
    int i=0;
    
    while (mask) {
	mask>>=1;
	i++;
    }
    return i;
}


RImage*
RCreateImageFromXImage(RContext *context, XImage *image, XImage *mask)
{
    RImage *img;
    int x, y;
    unsigned long pixel;
    unsigned char *r, *g, *b, *a;
    int rshift, gshift, bshift;
    int rmask, gmask, bmask;

    assert(image!=NULL);
    assert(image->format==ZPixmap);
    assert(!mask || mask->format==ZPixmap);
    
    img = RCreateImage(image->width, image->height, mask!=NULL);
    if (!img) {
	return NULL;
    }
      

    /* I don't know why, but XGetImage() for pixmaps don't set the
     * {red,green,blue}_mask values correctly. 
     */ 
    if (context->depth==image->depth) {
     	rmask = context->visual->red_mask;
    	gmask = context->visual->green_mask;
    	bmask = context->visual->blue_mask;
    } else {
    	rmask = image->red_mask;
    	gmask = image->green_mask;
    	bmask = image->blue_mask;
    }

    /* how many bits to shift to normalize the color into 8bpp */
    rshift = get_shifts(rmask) - 8;
    gshift = get_shifts(gmask) - 8;
    bshift = get_shifts(bmask) - 8;

    r = img->data[0];
    g = img->data[1];
    b = img->data[2];
    a = img->data[3];

#define NORMALIZE_RED(pixel)	((rshift>0) ? ((pixel) & rmask) >> rshift \
					: ((pixel) & rmask) << -rshift)
#define NORMALIZE_GREEN(pixel)	((gshift>0) ? ((pixel) & gmask) >> gshift \
					: ((pixel) & gmask) << -gshift)
#define NORMALIZE_BLUE(pixel)	((bshift>0) ? ((pixel) & bmask) >> bshift \
					: ((pixel) & bmask) << -bshift)

    if (image->depth==1) {
	for (y = 0; y < image->height; y++) {
	    for (x = 0; x < image->width; x++) {
		pixel = XGetPixel(image, x, y);
		if (pixel) {
		    *(r++) = *(g++) = *(b++) = 0;
		} else {
		    *(r++) = *(g++) = *(b++) = 0xff;		    
		}
	    }
	}
    } else {
	for (y = 0; y < image->height; y++) {
	    for (x = 0; x < image->width; x++) {
		pixel = XGetPixel(image, x, y);
		*(r++) = NORMALIZE_RED(pixel);
		*(g++) = NORMALIZE_GREEN(pixel);
		*(b++) = NORMALIZE_BLUE(pixel);
	    }
	}
    }

    if (mask && a) {
	for (y = 0; y < mask->height; y++) {
	    for (x = 0; x < mask->width; x++) {
		if (XGetPixel(mask, x, y)) {
		    *(a++) = 0xff;
		} else {
		    *(a++) = 0;
		}
	    }
	}
    }
    return img;
}



RImage*
RCreateImageFromDrawable(RContext *context, Drawable drawable, Pixmap mask)
{
    RImage *image;
    XImage *pimg, *mimg;
    unsigned int w, h, bar;
    int foo;
    Window baz;


    assert(drawable!=None);
    
    if (!XGetGeometry(context->dpy, drawable, &baz, &foo, &foo, 
		      &w, &h, &bar, &bar)) {
	printf("wrlib:invalid window or pixmap passed to RCreateImageFromPixmap\n");
	return NULL;
    }
    pimg = XGetImage(context->dpy, drawable, 0, 0, w, h, AllPlanes, 
		     ZPixmap);
    
    if (!pimg) {
	RErrorCode = RERR_XERROR;
	return NULL;
    }
    mimg = NULL;
    if (mask) {
	if (XGetGeometry(context->dpy, mask, &baz, &foo, &foo, 
			  &w, &h, &bar, &bar)) {
	    mimg = XGetImage(context->dpy, mask, 0, 0, w, h, AllPlanes, 
			     ZPixmap);
	}
    }

    image = RCreateImageFromXImage(context, pimg, mimg);

    XDestroyImage(pimg);
    if (mimg)
      XDestroyImage(mimg);

    return image;
}
