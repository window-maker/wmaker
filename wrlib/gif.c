/* gif.c - load GIF image from file
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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


#ifdef USE_GIF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gif_lib.h>

#include "wraster.h"


static int InterlacedOffset[] = { 0, 4, 2, 1 };
static int InterlacedJumps[] = { 8, 8, 4, 2 };


/*
 * Partially based on code in gif2rgb from giflib, by Gershon Elber.
 */
RImage*
RLoadGIF(RContext *context, char *file, int index)
{
    RImage *image = NULL;
    GifFileType *gif = NULL;
    GifPixelType *buffer = NULL;
    int i, j, k, ofs = 0;
    int width, height;
    GifRecordType recType;
    ColorMapObject *colormap;
    unsigned char rmap[256];
    unsigned char gmap[256];
    unsigned char bmap[256];

    if (index < 0)
	index = 0;

    /* default error message */
    RErrorCode = RERR_BADINDEX;

    gif = DGifOpenFileName(file);
    
    if (!gif) {
	switch (GifLastError()) {
	 case D_GIF_ERR_OPEN_FAILED:
	    RErrorCode = RERR_OPEN;
	    break;
	 case D_GIF_ERR_READ_FAILED:
	    RErrorCode = RERR_READ;
	    break;
	 default:
	    RErrorCode = RERR_BADIMAGEFILE;
	    break;
	}
	return NULL;
    }

    colormap = gif->SColorMap;

    i = 0;
    
    do {
	int extCode;
	GifByteType *extension;

	if (DGifGetRecordType(gif, &recType) == GIF_ERROR) {
	    goto giferr;
	}
	switch (recType) {
	 case IMAGE_DESC_RECORD_TYPE:
	    if (i++ != index)
		break;

	    if (DGifGetImageDesc(gif)==GIF_ERROR) {
		goto giferr;
	    }
	    
	    width = gif->Image.Width;
	    height = gif->Image.Height;
	    
	    if (gif->Image.ColorMap)
		colormap = gif->Image.ColorMap;

	    /* the gif specs talk about a default colormap, but it
	     * doesnt say what the heck is this default colormap */
	    if (!colormap) {
		/*
		 * Well, since the spec says the colormap can be anything,
		 * lets just render it with whatever garbage the stack 
		 * has :)
		 * 
		
		goto bye;
		 */
	    } else {
		for (j = 0; j < colormap->ColorCount; j++) {
		    rmap[j] = colormap->Colors[j].Red;
		    gmap[j] = colormap->Colors[j].Green;
		    bmap[j] = colormap->Colors[j].Blue;
		}
	    }

	    buffer = malloc(width * sizeof(GifColorType));
	    if (!buffer) {
		RErrorCode = RERR_NOMEMORY;
		goto bye;
	    }

	    image = RCreateImage(width, height, False);
	    if (!image) {
		goto bye;
	    }

	    if (gif->Image.Interlace) {
		int l;

		for (j = 0; j < 4; j++) {
		    for (k = InterlacedOffset[j]; k < height;
			 k += InterlacedJumps[j]) {
			if (DGifGetLine(gif, buffer, width)==GIF_ERROR) {
			    goto giferr;
			}
			ofs = k*width;
			for (l = 0; l < width; l++, ofs++) {
			    int pixel = buffer[l];
			    image->data[0][ofs] = rmap[pixel];
			    image->data[1][ofs] = gmap[pixel];
			    image->data[2][ofs] = bmap[pixel];
			}
		    }
		}
	    } else {
		for (j = 0; j < height; j++) {
		    if (DGifGetLine(gif, buffer, width)==GIF_ERROR) {
			goto giferr;
		    }
		    for (k = 0; k < width; k++, ofs++) {
			int pixel = buffer[k];
			image->data[0][ofs] = rmap[pixel];
			image->data[1][ofs] = gmap[pixel];
			image->data[2][ofs] = bmap[pixel];
		    }
		}
	    }
	    break;

	 case EXTENSION_RECORD_TYPE:
	    /* skip all extension blocks */
	    if (DGifGetExtension(gif, &extCode, &extension)==GIF_ERROR) {
		goto giferr;
	    }
	    while (extension) {
		if (DGifGetExtensionNext(gif, &extension)==GIF_ERROR) {
		    goto giferr;
		}
	    }
	    break;

	 default:
	    break;
	}
    } while (recType != TERMINATE_RECORD_TYPE && i <= index);

    /* yuck! */
    goto did_not_get_any_errors;
giferr:
    switch (GifLastError()) {
     case D_GIF_ERR_OPEN_FAILED:
	RErrorCode = RERR_OPEN;
	break;
     case D_GIF_ERR_READ_FAILED:
	RErrorCode = RERR_READ;
	break;
     default:
	RErrorCode = RERR_BADIMAGEFILE;
	break;
    }    
bye:
    if (image)
	RDestroyImage(image);
    image = NULL;
did_not_get_any_errors:

    if (buffer)
	free(buffer);

    if (gif)
	DGifCloseFile(gif);

    return image;
}

#endif /* USE_GIF */
