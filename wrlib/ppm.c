/* ppm.c - load PPM image from file
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wraster.h"


static RImage*
load_graymap(char *file_name, FILE *file, int w, int h, int max, int raw)
{
    RImage *image;
    
    image = RCreateImage(w, h, 0);
    if (!image) {
	return NULL;
    }
    if (!raw) {
	
    } else {
	if (max<256) {
	    if (!fgets(image->data[0], w*h, file))
		goto short_file;
	    memcpy(image->data[0], image->data[1], w*h);
	    memcpy(image->data[0], image->data[2], w*h);
	} else {
	    
	}
    }
    
    return image;
    
  short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    return NULL;
}


static RImage*
load_pixmap(char *file_name, FILE *file, int w, int h, int max, int raw)
{
    RImage *image;
    int i;
    char buf[3];
    char *r, *g, *b;
    
    image = RCreateImage(w, h, 0);
    if (!image) {
	return NULL;
    }
    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    if (!raw) {
	
    } else {
	if (max<256) {
	    i = 0;
	    while (i < w*h) {
		if (fread(buf, 1, 3, file)!=3)
		    goto short_file;
		*(r++) = buf[0];
		*(g++) = buf[1];
		*(b++) = buf[2];
		i++;
	    }
	} else {
	    
	}
    }
    
    return image;
    
  short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    return NULL;
}


RImage*
RLoadPPM(RContext *context, char *file_name, int index)
{
    FILE *file;
    RImage *image = NULL;
    char buffer[256];
    int w, h, m;
    int type;

#define GETL() if (!fgets(buffer, 255, file)) goto short_file
    
    file = fopen(file_name, "r");
    if (!file) {
	RErrorCode = RERR_OPEN;
	return NULL;
    }
    
    /* get signature */
    GETL();

    /* only accept raw pixmaps or graymaps */
    if (buffer[0] != 'P' || (buffer[1] != '5' && buffer[1] != '6')) {
	RErrorCode = RERR_BADFORMAT;
	fclose(file);
	return NULL;
    }
    
    type = buffer[1];
    
    /* skip comments */
    while (1) {
	GETL();
	
	if (buffer[0]!='#')
	    break;
    }
    
    /* get size */
    if (sscanf(buffer, "%i %i", &w, &h)!=2)
	goto bad_file;
    

    GETL();
    if (sscanf(buffer, "%i", &m)!=1 || m < 1)
	goto bad_file;

    if (type=='5')
	image = load_graymap(file_name, file, w, h, m, type=='5');
    else if (type=='6')
	image = load_pixmap(file_name, file, w, h, m, type=='6');

    fclose(file);
    return image;
    
  bad_file:
  short_file:
    RErrorCode = RERR_BADIMAGEFILE;
    fclose(file);
    return NULL;
}

