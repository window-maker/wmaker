/* png.c - load PNG image from file
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


#ifdef USE_PNG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <png.h>

#include "wraster.h"


RImage*
RLoadPNG(RContext *context, char *file, int index)
{
    char *tmp;
    RImage *image=NULL;
    FILE *f;
    png_structp png;
    png_infop pinfo, einfo;
    png_color_16p bkcolor;
    int alpha;
    int x, y, i;
    double gamma, sgamma;
    png_uint_32 width, height;
    int depth, junk, color_type;
    png_bytep *png_rows;
    unsigned char *r, *g, *b, *a;
    
    f = fopen(file, "r");
    if (!f) {
	RErrorCode = RERR_OPEN;
	return NULL;
    }
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, 
				 (png_error_ptr)NULL, (png_error_ptr)NULL);
    if (!png) {
	RErrorCode = RERR_NOMEMORY;
	fclose(f);
	return NULL;
    }
    
    pinfo = png_create_info_struct(png);
    if (!pinfo) {
	RErrorCode = RERR_NOMEMORY;
	fclose(f);
	png_destroy_read_struct(&png, NULL, NULL);
	return NULL;
    }

    einfo = png_create_info_struct(png);
    if (!einfo) {
	RErrorCode = RERR_NOMEMORY;
	fclose(f);
	png_destroy_read_struct(&png, &pinfo, NULL);
	return NULL;
    }

    RErrorCode = RERR_INTERNAL;
    if (setjmp(png->jmpbuf)) {
	fclose(f);
	png_destroy_read_struct(&png, &pinfo, &einfo);
	if (image)
	  RDestroyImage(image);
	return NULL;
    }
    
    png_init_io(png, f);
    
    png_read_info(png, pinfo);
    
    png_get_IHDR(png, pinfo, &width, &height, &depth, &color_type,
		 &junk, &junk, &junk);

    
    /* check for an alpha channel */
    if (png_get_valid(png, pinfo, PNG_INFO_tRNS))
      alpha = True;
    else
      alpha = (color_type & PNG_COLOR_MASK_ALPHA);

    /* allocate RImage */
    image = RCreateImage(width, height, alpha);
    if (!image) {
	fclose(f);
	png_destroy_read_struct(&png, &pinfo, &einfo);
	return NULL;
    }
    
    /* normalize to 8bpp with alpha channel */
    if (color_type == PNG_COLOR_TYPE_PALETTE && depth < 8)
        png_set_expand(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && depth < 8)
        png_set_expand(png);

    if (png_get_valid(png, pinfo, PNG_INFO_tRNS))
        png_set_expand(png);

    if (depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png);

    /* set gamma correction */
    if ((context->attribs->flags & RC_GammaCorrection)
	&& context->depth != 8) {
	sgamma = (context->attribs->rgamma + context->attribs->ggamma +
		  context->attribs->bgamma) / 3;
    } else if ((tmp = getenv("DISPLAY_GAMMA")) != NULL) {
	sgamma = atof(tmp);
	if (sgamma==0)
	    sgamma = 1;
    } else {
	/* no, this is correct. Old gimp versions are broken and save wrong
	 * data. Upgrade gimp. */
	sgamma = 2.0;
    }
    
    if (png_get_gAMA(png, pinfo, &gamma))
      png_set_gamma(png, sgamma, gamma);
    else
      png_set_gamma(png, sgamma, 0.45);

    /* do the transforms */
    png_read_update_info(png, pinfo);

    /* set background color */
    if (png_get_bKGD(png, pinfo, &bkcolor)) {
	image->background.red = bkcolor->red >> 8;
	image->background.green = bkcolor->green >> 8;
	image->background.blue = bkcolor->blue >> 8;
    }

    png_rows = alloca(sizeof(char*)*height);
    if (!png_rows) {
	RErrorCode = RERR_NOMEMORY;
	fclose(f);
	RDestroyImage(image);
	png_destroy_read_struct(&png, &pinfo, &einfo);
#ifdef C_ALLOCA
	alloca(0);
#endif
	return NULL;
    }
    for (y=0; y<height; y++) {
	png_rows[y] = alloca(png_get_rowbytes(png, pinfo));
	if (!png_rows[y]) {
	    RErrorCode = RERR_NOMEMORY;
	    fclose(f);
	    RDestroyImage(image);
	    png_destroy_read_struct(&png, &pinfo, &einfo);
#ifdef C_ALLOCA
	    alloca(0);
#endif
	    return NULL;
	}
    }
    /* read data */
    png_read_image(png, png_rows);
    
    png_read_end(png, einfo);
    
    png_destroy_read_struct(&png, &pinfo, &einfo);
    
    fclose(f);

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];

    /* convert to RImage */
    if (alpha) {
	for (y=0; y<height; y++) {
	    for (x=0, i=0; x<width; x++) {
		*(r++) = *(png_rows[y]+i++);
		*(g++) = *(png_rows[y]+i++);
		*(b++) = *(png_rows[y]+i++);
		*(a++) = *(png_rows[y]+i++);
	    }
	}
    } else {
	for (y=0; y<height; y++) {
	    for (x=0, i=0; x<width; x++) {
		*(r++) = *(png_rows[y]+i++);
		*(g++) = *(png_rows[y]+i++);
		*(b++) = *(png_rows[y]+i++);
	    }
	}	
    }
#ifdef C_ALLOCA
    alloca(0);
#endif
    return image;
}

#endif /* USE_PNG */
