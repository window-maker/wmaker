/* directjpeg.c- loads a jpeg file directly into a XImage
 *
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1999-2003 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */


#include "../src/config.h"



#ifdef USE_JPEG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jpeglib.h>

#include "../wrlib/wraster.h"

#include <setjmp.h>


struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


static Bool
canLoad(RContext *rc)
{
    if (rc->depth != 16 || rc->vclass != TrueColor
	|| rc->red_offset!=11 || rc->green_offset!=5 || rc->blue_offset!=0)
	return False;

    return True;
}


static void
readData(RContext *rc, struct jpeg_decompress_struct *cinfo,
	 JSAMPROW *buffer, RXImage *ximg) 
{
    int i, j;
    unsigned long pixel;
    int y = 0;

    /* for 16bpp only */
    while (cinfo->output_scanline < cinfo->output_height) {

        jpeg_read_scanlines(cinfo, buffer, (JDIMENSION)1);

	if (cinfo->out_color_space==JCS_RGB) {
	    for (i=0,j=0; i<cinfo->image_width; i++) {

		printf("%i %i %i\n",
		       (((unsigned long)buffer[0][j])&0xf8)<<8,
		       (((unsigned long)buffer[0][j+1])&0xf4)<<3,
		       (((unsigned long)buffer[0][j+2]))>>3);
	    
		pixel = (((unsigned long)buffer[0][j++])&0xf8)<<8
		    |(((unsigned long)buffer[0][j++])&0xf4)<<3
		    |(((unsigned long)buffer[0][j++]))>>3;

		XPutPixel(ximg->image, i, y, pixel);
	    }
	} else {
	    for (i=0,j=0; i<cinfo->image_width; i++, j++) {

		pixel = (unsigned long)buffer[0][j]<<8
		    |(unsigned long)buffer[0][j]<<3
		    |(unsigned long)buffer[0][j]>>3;

		XPutPixel(ximg->image, i, y, pixel);
	    }
	}
	y++;
    }
}



Pixmap
LoadJPEG(RContext *rc, char *file_name, int *width, int *height)
{
    struct jpeg_decompress_struct cinfo;
    JSAMPROW buffer[1];
    FILE *file;
    struct my_error_mgr jerr;
    RXImage *ximg = NULL;
    unsigned char buf[8];
    Pixmap p = None;

    if (!canLoad(rc))
	return None;

    file = fopen(file_name, "rb");
    if (!file) {
	return None;
    }
    if (fread(buf, 2, 1, file) != 1) {
	fclose(file);
	return None;
    }
    if (buf[0] != 0xff || buf[1] != 0xd8) {
	fclose(file);
	return None;
    }
    rewind(file);

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
       /* If we get here, the JPEG code has signaled an error.
	* We need to clean up the JPEG object, close the input file, and return.
	*/
	jpeg_destroy_decompress(&cinfo);
	fclose(file);

	if (ximg) {
	    RDestroyXImage(rc, ximg);
	}
	
	return None;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, file);
  
    jpeg_read_header(&cinfo, TRUE);

    buffer[0] = (JSAMPROW)malloc(cinfo.image_width*cinfo.num_components);
    if (!buffer[0]) {
	RErrorCode = RERR_NOMEMORY;
	goto bye;
    }

    if(cinfo.jpeg_color_space==JCS_GRAYSCALE) {
       cinfo.out_color_space=JCS_GRAYSCALE;
    } else
        cinfo.out_color_space = JCS_RGB;
    cinfo.quantize_colors = FALSE;
    cinfo.do_fancy_upsampling = FALSE;
    cinfo.do_block_smoothing = FALSE;
    jpeg_calc_output_dimensions(&cinfo);

    ximg = RCreateXImage(rc, rc->depth, cinfo.image_width, cinfo.image_height);
    if (!ximg) {
       goto bye;
    }
    jpeg_start_decompress(&cinfo);

    readData(rc, &cinfo, buffer, ximg);

    jpeg_finish_decompress(&cinfo);

    p = XCreatePixmap(rc->dpy, rc->drawable, cinfo.image_width, 
		      cinfo.image_height, rc->depth);

    RPutXImage(rc, p, rc->copy_gc, ximg, 0, 0, 0, 0, cinfo.image_width,
	       cinfo.image_height);

    *width = cinfo.image_width;
    *height = cinfo.image_height;
    
  bye:
    jpeg_destroy_decompress(&cinfo);

    fclose(file);

    if (buffer[0])
	free(buffer[0]);  

    if (ximg)
	RDestroyXImage(rc, ximg);
    
    return p;
}

#endif /* USE_JPEG */

