/* tiff.c - load TIFF image from file
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


#ifdef USE_TIFF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tiff.h>
#include <tiffio.h>


#include "wraster.h"


RImage*
RLoadTIFF(RContext *context, char *file, int index)
{
    RImage *image = NULL;
    TIFF *tif;
    int i;
    unsigned char *r, *g, *b, *a;
    uint16 alpha, amode;
    uint32 width, height;
    uint32 *data, *ptr;
    uint16 extrasamples;
    uint16 *sampleinfo;
    
    
    tif = TIFFOpen(file, "r");
    if (!tif)
      return NULL;

    /* seek index */
    i = index;
    while (i>0) {
	if (!TIFFReadDirectory(tif)) {
	    RErrorCode = RERR_BADINDEX;
	    TIFFClose(tif);
	    return NULL;
	}
	i--;
    }
    
    /* get info */
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
			  &extrasamples, &sampleinfo);
    
    alpha = (extrasamples == 1 &&
	     ((sampleinfo[0] == EXTRASAMPLE_ASSOCALPHA) || (sampleinfo[0] == EXTRASAMPLE_UNASSALPHA)));
    amode = (extrasamples == 1 && sampleinfo[0] == EXTRASAMPLE_ASSOCALPHA);
    
    if (width<1 || height<1) {
	RErrorCode = RERR_BADIMAGEFILE;
	TIFFClose(tif);
	return NULL;
    }
      
    /* read data */
    ptr = data = (uint32*)_TIFFmalloc(width * height * sizeof(uint32));
    
    if (!data) {
	RErrorCode = RERR_NOMEMORY;
    } else {
	if (!TIFFReadRGBAImage(tif, width, height, data, 0)) {
	    RErrorCode = RERR_BADIMAGEFILE;
	} else {
	    
	    /* convert data */
	    image = RCreateImage(width, height, alpha);
	    
	    if (image) {
		int x, y;
		
		r = image->data[0];
		g = image->data[1];
		b = image->data[2];
		a = image->data[3];

		/* data seems to be stored upside down */
		data += width * (height-1);
		for (y=0; y<height; y++) {
		    for (x=0; x<width; x++) {
		    
			*(r) = (*data) & 0xff;
			*(g) = (*data >> 8) & 0xff;
			*(b) = (*data >> 16) & 0xff;

			if (alpha) {
			    *(a) = (*data >> 24) & 0xff;
			    
		    	 if (amode && (*a > 0)) {
			       *r = (*r * 255) / *(a);
			       *g = (*g * 255) / *(a);
			       *b = (*b * 255) / *(a);
			    }
				 
				 a++;
			}
			
			r++; g++; b++;
			data++;
		    }
		    data -= 2*width;
		}
	    }
	}
	_TIFFfree(ptr);
    }
    
    TIFFClose(tif);
    
    return image;
}

#endif /* USE_TIFF */
