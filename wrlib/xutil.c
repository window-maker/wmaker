/* xutil.c - utility functions for X
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

#ifdef XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* XSHM */

#include "wraster.h"


#ifdef XSHM

static int shmError;

static int (*oldErrorHandler)();

static int
errorHandler(Display *dpy, XErrorEvent *err)
{
    shmError=1;
    if(err->error_code!=BadAccess)
	(*oldErrorHandler)(dpy, err);

    return 0;
}


#endif


RXImage*
RCreateXImage(RContext *context, int depth, unsigned width, unsigned height)
{
    RXImage *rximg;
    Visual *visual = context->visual;
    
    rximg = malloc(sizeof(RXImage));
    if (!rximg) {
	RErrorCode = RERR_NOMEMORY;
	return NULL;
    }
    
#ifndef XSHM
    rximg->image = XCreateImage(context->dpy, visual, depth,
				ZPixmap, 0, NULL, width, height, 8, 0);
    if (!rximg->image) {
	free(rximg);
	RErrorCode = RERR_XERROR;
	return NULL;
    }
    rximg->image->data = malloc(rximg->image->bytes_per_line*height);
    if (!rximg->image->data) {
	XDestroyImage(rximg->image);
	free(rximg);
	RErrorCode = RERR_NOMEMORY;
	return NULL;
    }
    
#else /* XSHM */    
    if (!context->attribs->use_shared_memory) {
    retry_without_shm:
	rximg->is_shared = 0;
	rximg->image = XCreateImage(context->dpy, visual, depth,
				    ZPixmap, 0, NULL, width, height, 8, 0);
	if (!rximg->image) {
	    free(rximg);
	    RErrorCode = RERR_XERROR;
	    return NULL;
	}
	rximg->image->data = malloc(rximg->image->bytes_per_line*height);
	if (!rximg->image->data) {
	    XDestroyImage(rximg->image);
	    free(rximg);
	    RErrorCode = RERR_NOMEMORY;
	    return NULL;
	}
    } else {
	rximg->is_shared = 1;

	rximg->info.readOnly = False;

	rximg->image = XShmCreateImage(context->dpy, visual, depth,
				       ZPixmap, NULL, &rximg->info, width, 
				       height);	

	rximg->info.shmid = shmget(IPC_PRIVATE, 
				   rximg->image->bytes_per_line*height,
				   IPC_CREAT|0666);
	if (rximg->info.shmid < 0) {
	    context->attribs->use_shared_memory = 0;
	    perror("wrlib:could not allocate shared memory segment");
	    XDestroyImage(rximg->image);
	    goto retry_without_shm;
	}
	
	rximg->info.shmaddr = shmat(rximg->info.shmid, 0, 0);
	if (rximg->info.shmaddr == (void*)-1) {
	    context->attribs->use_shared_memory = 0;
	    if (shmctl(rximg->info.shmid, IPC_RMID, 0) < 0)
		perror("wrlib:shmctl");
	    perror("wrlib:could not allocate shared memory");
	    XDestroyImage(rximg->image);
	    goto retry_without_shm;
	}
	
	shmError = 0;
	XSync(context->dpy, False);
	oldErrorHandler = XSetErrorHandler(errorHandler);
	XShmAttach(context->dpy, &rximg->info);	
	XSync(context->dpy, False);
	XSetErrorHandler(oldErrorHandler);

	rximg->image->data = rximg->info.shmaddr;
/*	rximg->image->obdata = &(rximg->info);*/

	if (shmError) {
	    context->attribs->use_shared_memory = 0;
	    XDestroyImage(rximg->image);
	    if (shmdt(rximg->info.shmaddr) < 0)
		perror("wrlib:shmdt");
	    if (shmctl(rximg->info.shmid, IPC_RMID, 0) < 0)
		perror("wrlib:shmctl");
	    printf("wrlib:error attaching shared memory segment to XImage\n");
	    goto retry_without_shm;
	}
    }
#endif /* XSHM */
    
    return rximg;
}


void
RDestroyXImage(RContext *context, RXImage *rximage)
{
#ifndef XSHM
    XDestroyImage(rximage->image);
    free(rximage);
#else /* XSHM */
    if (rximage->is_shared) {
	XShmDetach(context->dpy, &rximage->info);
	XDestroyImage(rximage->image);
	if (shmdt(rximage->info.shmaddr) < 0)
	    perror("wrlib:shmdt");
	if (shmctl(rximage->info.shmid, IPC_RMID, 0) < 0)
	    perror("wrlib:shmctl");
    } else {
	XDestroyImage(rximage->image);
    }
#endif
}


void
RPutXImage(RContext *context, Drawable d, GC gc, RXImage *ximage, int src_x,
	   int src_y, int dest_x, int dest_y, 
	   unsigned int width, unsigned int height)
{
#ifndef XSHM
    XPutImage(context->dpy, d, gc, ximage->image, src_x, src_y, dest_x,
	      dest_y, width, height);
#else
    if (ximage->is_shared) {
	XShmPutImage(context->dpy, d, gc, ximage->image, src_x, src_y,
		     dest_x, dest_y, width, height, False);
    } else {
	XPutImage(context->dpy, d, gc, ximage->image, src_x, src_y, dest_x,
		  dest_y, width, height);
    }
    XFlush(context->dpy);
#endif /* XSHM */
}


#ifdef XSHM
Pixmap
R_CreateXImageMappedPixmap(RContext *context, RXImage *rximage)
{
    Pixmap pix;

    pix = XShmCreatePixmap(context->dpy, context->drawable, 
			   rximage->image->data, &rximage->info,
			   rximage->image->width, rximage->image->height,
			   rximage->image->depth);

    return pix;
}

#endif /* XSHM */

