/* convert.c - convert RImage to Pixmap
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


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include "wraster.h"

#ifdef XSHM
Pixmap R_CreateXImageMappedPixmap(RContext *context, RXImage *ximage);

#endif


typedef struct RConversionTable {
    unsigned short table[256];
    unsigned short index;
    struct RConversionTable *next;
} RConversionTable;


static RConversionTable *conversionTable = NULL;


static unsigned short*
computeTable(unsigned short mask)
{
    RConversionTable *tmp = conversionTable;
    int i;

    while (tmp) {
        if (tmp->index == mask)
            break;
        tmp = tmp->next;
    }

    if (tmp)
        return tmp->table;

    tmp = (RConversionTable *)malloc(sizeof(RConversionTable));
    if (tmp == NULL)
        return NULL;

    for (i=0;i<256;i++)
        tmp->table[i] = (i*mask + 0x7f)/0xff;

    tmp->index = mask;
    tmp->next = conversionTable;
    conversionTable = tmp;
    return tmp->table;
}



static RXImage*
image2TrueColorD16(RContext *ctx, RImage *image)
{
    RXImage *ximg;
    register int x, y, r, g, b;
    unsigned char *red, *grn, *blu;
    unsigned short rmask, gmask, bmask;
    unsigned short roffs, goffs, boffs;
    unsigned short *rtable, *gtable, *btable;
    int ofs;

    ximg = RCreateXImage(ctx, ctx->depth, image->width, image->height);
    if (!ximg) {
	return NULL;
    }

    red = image->data[0];
    grn = image->data[1];
    blu = image->data[2];
    
    roffs = ctx->red_offset;
    goffs = ctx->green_offset;
    boffs = ctx->blue_offset;
    
    rmask = ctx->visual->red_mask >> roffs;
    gmask = ctx->visual->green_mask >> goffs;
    bmask = ctx->visual->blue_mask >> boffs;

    rtable = computeTable(rmask);
    gtable = computeTable(gmask);
    btable = computeTable(bmask);

    if (rtable==NULL || gtable==NULL || btable==NULL) {
	RErrorCode = RERR_NOMEMORY;
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    {
        /* dither */
	short *rerr, *gerr, *berr;
	short *nrerr, *ngerr, *nberr;
	short *terr;
	unsigned short *dataP;
	int line_offset;
	int rer, ger, ber;
	const int dr=0xff/rmask;
	const int dg=0xff/gmask;
        const int db=0xff/bmask;

	rerr = (short*)alloca((image->width+2)*sizeof(short));
	gerr = (short*)alloca((image->width+2)*sizeof(short));
	berr = (short*)alloca((image->width+2)*sizeof(short));
	nrerr = (short*)alloca((image->width+2)*sizeof(short));
	ngerr = (short*)alloca((image->width+2)*sizeof(short));
	nberr = (short*)alloca((image->width+2)*sizeof(short));
	if (!rerr || !gerr || !berr || !nrerr || !ngerr || !nberr) {
	    RErrorCode = RERR_NOMEMORY;
	    RDestroyXImage(ctx, ximg);
	    return NULL;
	}
	for (x=0; x<image->width; x++) {
	    rerr[x] = red[x];
	    gerr[x] = grn[x];
	    berr[x] = blu[x];
	}
        rerr[x] = gerr[x] = berr[x] = 0;
	
	dataP = (unsigned short*)ximg->image->data;
	line_offset = ximg->image->bytes_per_line - image->width * 2;

        /* convert and dither the image to XImage */
	for (y=0, ofs=0; y<image->height; y++) {
	    if (y<image->height-1) {
		int x1;
		for (x=0, x1=ofs+image->width; x<image->width; x++, x1++) {
		    nrerr[x] = red[x1];
		    ngerr[x] = grn[x1];
		    nberr[x] = blu[x1];
		}
		/* last column */
		x1--;
		nrerr[x] = red[x1];
		ngerr[x] = grn[x1];
		nberr[x] = blu[x1];
	    }
	    for (x=0; x<image->width; x++) {
		/* reduce pixel */
                if (rerr[x]>0xff) rerr[x]=0xff; else if (rerr[x]<0) rerr[x]=0;
                if (gerr[x]>0xff) gerr[x]=0xff; else if (gerr[x]<0) gerr[x]=0;
                if (berr[x]>0xff) berr[x]=0xff; else if (berr[x]<0) berr[x]=0;

                r = rtable[rerr[x]];
                g = gtable[gerr[x]];
                b = btable[berr[x]];

		*(dataP++) = (r<<roffs) | (g<<goffs) | (b<<boffs);

		/* calc error */
		rer = rerr[x] - r*dr;
		ger = gerr[x] - g*dg;
		ber = berr[x] - b*db;

		/* distribute error */
		r = (rer*3)/8;
		g = (ger*3)/8;
		b = (ber*3)/8;
                /* x+1, y */
		rerr[x+1]+=r;
		gerr[x+1]+=g;
                berr[x+1]+=b;
		/* x, y+1 */
		nrerr[x]+=r;
		ngerr[x]+=g;
		nberr[x]+=b;
		/* x+1, y+1 */
		nrerr[x+1]+=rer-2*r;
		ngerr[x+1]+=ger-2*g;
		nberr[x+1]+=ber-2*b;
	    }
	    ofs += image->width;
	    (char*)dataP += line_offset;
	    /* skip to next line */
	    terr = rerr;
	    rerr = nrerr;
	    nrerr = terr;

	    terr = gerr;
	    gerr = ngerr;
	    ngerr = terr;

	    terr = berr;
	    berr = nberr;
	    nberr = terr;
	}
    }
    return ximg;
}


static RXImage*
image2TrueColor(RContext *ctx, RImage *image)
{
    RXImage *ximg;
    register int x, y, r, g, b;
    unsigned char *red, *grn, *blu;
    unsigned long pixel;
    unsigned short rmask, gmask, bmask;
    unsigned short roffs, goffs, boffs;
    unsigned short *rtable, *gtable, *btable;
    int ofs;

    ximg = RCreateXImage(ctx, ctx->depth, image->width, image->height);
    if (!ximg) {
	return NULL;
    }

    red = image->data[0];
    grn = image->data[1];
    blu = image->data[2];
    
    roffs = ctx->red_offset;
    goffs = ctx->green_offset;
    boffs = ctx->blue_offset;
    
    rmask = ctx->visual->red_mask >> roffs;
    gmask = ctx->visual->green_mask >> goffs;
    bmask = ctx->visual->blue_mask >> boffs;

#if 0
    /* this do not seem to increase speed. Only 0.06 second faster in
     * rendering a 800x600 image to pixmap. 1.12 sec instead of 1.18.
     * But does not require a 256*256*256 lookup table.
     */
    if (ctx->depth==24) {
#ifdef DEBUG
        puts("true color match for 24bpp");
#endif
	for (y=0; y < image->height; y++) {
	    for (x=0; x < image->width; x++) {
                pixel = (*(red++)<<roffs) | (*(grn++)<<goffs) | (*(blu++)<<boffs);
		XPutPixel(ximg->image, x, y, pixel);
	    }
        }
        return ximg;
    }
#endif

    rtable = computeTable(rmask);
    gtable = computeTable(gmask);
    btable = computeTable(bmask);

    if (rtable==NULL || gtable==NULL || btable==NULL) {
	RErrorCode = RERR_NOMEMORY;
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode==RBestMatchRendering) {
        /* fake match */
#ifdef DEBUG
        puts("true color match");
#endif
	for (y=0, ofs=0; y < image->height; y++) {
	    for (x=0; x < image->width; x++, ofs++) {
		/* reduce pixel */
                r = rtable[red[ofs]];
                g = gtable[grn[ofs]];
                b = btable[blu[ofs]];
                pixel = (r<<roffs) | (g<<goffs) | (b<<boffs);
		XPutPixel(ximg->image, x, y, pixel);
	    }
	}
    } else {
        /* dither */
	short *rerr, *gerr, *berr;
	short *nrerr, *ngerr, *nberr;
	short *terr;
	int rer, ger, ber;
	const int dr=0xff/rmask;
	const int dg=0xff/gmask;
        const int db=0xff/bmask;

#ifdef DEBUG
        puts("true color dither");
#endif
	rerr = (short*)alloca((image->width+2)*sizeof(short));
	gerr = (short*)alloca((image->width+2)*sizeof(short));
	berr = (short*)alloca((image->width+2)*sizeof(short));
	nrerr = (short*)alloca((image->width+2)*sizeof(short));
	ngerr = (short*)alloca((image->width+2)*sizeof(short));
	nberr = (short*)alloca((image->width+2)*sizeof(short));
	if (!rerr || !gerr || !berr || !nrerr || !ngerr || !nberr) {
	    RErrorCode = RERR_NOMEMORY;
	    RDestroyXImage(ctx, ximg);
	    return NULL;
	}
	for (x=0; x<image->width; x++) {
	    rerr[x] = red[x];
	    gerr[x] = grn[x];
	    berr[x] = blu[x];
	}
        rerr[x] = gerr[x] = berr[x] = 0;
        /* convert and dither the image to XImage */
	for (y=0, ofs=0; y<image->height; y++) {
	    if (y<image->height-1) {
		int x1;
		for (x=0, x1=ofs+image->width; x<image->width; x++, x1++) {
		    nrerr[x] = red[x1];
		    ngerr[x] = grn[x1];
		    nberr[x] = blu[x1];
		}
		/* last column */
		x1--;
		nrerr[x] = red[x1];
		ngerr[x] = grn[x1];
		nberr[x] = blu[x1];
	    }
	    for (x=0; x<image->width; x++) {
		/* reduce pixel */
                if (rerr[x]>0xff) rerr[x]=0xff; else if (rerr[x]<0) rerr[x]=0;
                if (gerr[x]>0xff) gerr[x]=0xff; else if (gerr[x]<0) gerr[x]=0;
                if (berr[x]>0xff) berr[x]=0xff; else if (berr[x]<0) berr[x]=0;

                r = rtable[rerr[x]];
                g = gtable[gerr[x]];
                b = btable[berr[x]];

                pixel = (r<<roffs) | (g<<goffs) | (b<<boffs);
		XPutPixel(ximg->image, x, y, pixel);
		/* calc error */
		rer = rerr[x] - r*dr;
		ger = gerr[x] - g*dg;
		ber = berr[x] - b*db;

		/* distribute error */
		r = (rer*3)/8;
		g = (ger*3)/8;
		b = (ber*3)/8;
                /* x+1, y */
		rerr[x+1]+=r;
		gerr[x+1]+=g;
                berr[x+1]+=b;
		/* x, y+1 */
		nrerr[x]+=r;
		ngerr[x]+=g;
		nberr[x]+=b;
		/* x+1, y+1 */
		nrerr[x+1]+=rer-2*r;
		ngerr[x+1]+=ger-2*g;
		nberr[x+1]+=ber-2*b;
	    }
	    ofs+=image->width;
	    /* skip to next line */
	    terr = rerr;
	    rerr = nrerr;
	    nrerr = terr;

	    terr = gerr;
	    gerr = ngerr;
	    ngerr = terr;

	    terr = berr;
	    berr = nberr;
	    nberr = terr;
	}
    }
    return ximg;
}




static RXImage*
image2PseudoColor(RContext *ctx, RImage *image)
{
    RXImage *ximg;
    register int x, y, r, g, b;
    unsigned char *red, *grn, *blu;
    unsigned long pixel;
    const int cpc=ctx->attribs->colors_per_channel;
    const unsigned short rmask = cpc-1; /* different sizes could be used */
    const unsigned short gmask = rmask; /* for r,g,b */
    const unsigned short bmask = rmask;
    unsigned short *rtable, *gtable, *btable;
    const int cpccpc = cpc*cpc;
    unsigned char *data;
    int ofs;
    /*register unsigned char maxrgb = 0xff;*/

    ximg = RCreateXImage(ctx, ctx->depth, image->width, image->height);
    if (!ximg) {
	return NULL;
    }

    red = image->data[0];
    grn = image->data[1];
    blu = image->data[2];

    data = ximg->image->data;

    /* Tables are same at the moment because rmask==gmask==bmask. */
    rtable = computeTable(rmask);
    gtable = computeTable(gmask);
    btable = computeTable(bmask);

    if (rtable==NULL || gtable==NULL || btable==NULL) {
	RErrorCode = RERR_NOMEMORY;
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode == RBestMatchRendering) {
        /* fake match */
#ifdef DEBUG
        printf("pseudo color match with %d colors per channel\n", cpc);
#endif
	for (y=0, ofs = 0; y<image->height; y++) {
	    for (x=0; x<image->width; x++, ofs++) {
		/* reduce pixel */
                r = rtable[red[ofs]];
                g = gtable[grn[ofs]];
                b = btable[blu[ofs]];
		pixel = r*cpccpc + g*cpc + b;
                /*data[ofs] = ctx->colors[pixel].pixel;*/
                XPutPixel(ximg->image, x, y, ctx->colors[pixel].pixel);
	    }
	}
    } else {
	/* dither */
	short *rerr, *gerr, *berr;
	short *nrerr, *ngerr, *nberr;
	short *terr;
	int rer, ger, ber;
	const int dr=0xff/rmask;
	const int dg=0xff/gmask;
	const int db=0xff/bmask;

#ifdef DEBUG
        printf("pseudo color dithering with %d colors per channel\n", cpc);
#endif
	rerr = (short*)alloca((image->width+2)*sizeof(short));
	gerr = (short*)alloca((image->width+2)*sizeof(short));
	berr = (short*)alloca((image->width+2)*sizeof(short));
	nrerr = (short*)alloca((image->width+2)*sizeof(short));
	ngerr = (short*)alloca((image->width+2)*sizeof(short));
	nberr = (short*)alloca((image->width+2)*sizeof(short));
	if (!rerr || !gerr || !berr || !nrerr || !ngerr || !nberr) {
	    RErrorCode = RERR_NOMEMORY;
	    RDestroyXImage(ctx, ximg);
	    return NULL;
	}
	for (x=0; x<image->width; x++) {
	    rerr[x] = red[x];
	    gerr[x] = grn[x];
	    berr[x] = blu[x];
	}
        rerr[x] = gerr[x] = berr[x] = 0;
	/* convert and dither the image to XImage */
	for (y=0, ofs=0; y<image->height; y++) {
	    if (y<image->height-1) {
		int x1;
		for (x=0, x1=ofs+image->width; x<image->width; x++, x1++) {
		    nrerr[x] = red[x1];
		    ngerr[x] = grn[x1];
		    nberr[x] = blu[x1];
		}
		/* last column */
		x1--;
		nrerr[x] = red[x1];
		ngerr[x] = grn[x1];
		nberr[x] = blu[x1];
	    }
	    for (x=0; x<image->width; x++, ofs++) {
		/* reduce pixel */
                if (rerr[x]>0xff) rerr[x]=0xff; else if (rerr[x]<0) rerr[x]=0;
                if (gerr[x]>0xff) gerr[x]=0xff; else if (gerr[x]<0) gerr[x]=0;
                if (berr[x]>0xff) berr[x]=0xff; else if (berr[x]<0) berr[x]=0;

                r = rtable[rerr[x]];
                g = gtable[gerr[x]];
                b = btable[berr[x]];

		pixel = r*cpccpc + g*cpc + b;
                /*data[ofs] = ctx->colors[pixel].pixel;*/
                XPutPixel(ximg->image, x, y, ctx->colors[pixel].pixel);

		/* calc error */
		rer = rerr[x] - r*dr;
		ger = gerr[x] - g*dg;
		ber = berr[x] - b*db;

		/* distribute error */
		rerr[x+1]+=(rer*7)/16;
		gerr[x+1]+=(ger*7)/16;
		berr[x+1]+=(ber*7)/16;
		
		nrerr[x]+=(rer*5)/16;
		ngerr[x]+=(ger*5)/16;
		nberr[x]+=(ber*5)/16;

		if (x>0) {
		    nrerr[x-1]+=(rer*3)/16;
		    ngerr[x-1]+=(ger*3)/16;
		    nberr[x-1]+=(ber*3)/16;
		}

		nrerr[x+1]+=rer/16;
		ngerr[x+1]+=ger/16;
		nberr[x+1]+=ber/16;
#if 0
		/* distribute error */
		r = (rer*3)/8;
		g = (ger*3)/8;
		b = (ber*3)/8;
		/* x+1, y */
		rerr[x+1]+=r;
		gerr[x+1]+=g;
		berr[x+1]+=b;
		/* x, y+1 */
		nrerr[x]+=r;
		ngerr[x]+=g;
		nberr[x]+=b;
		/* x+1, y+1 */
		nrerr[x+1]+=rer-2*r;
		ngerr[x+1]+=ger-2*g;
		nberr[x+1]+=ber-2*b;
#endif
	    }
	    /* skip to next line */
	    terr = rerr;
	    rerr = nrerr;
	    nrerr = terr;

	    terr = gerr;
	    gerr = ngerr;
	    ngerr = terr;

	    terr = berr;
	    berr = nberr;
	    nberr = terr;
	}
    }
    ximg->image->data = (char*)data;

    return ximg;
}


static RXImage*
image2GrayScale(RContext *ctx, RImage *image)
{
    RXImage *ximg;
    register int x, y, g;
    unsigned char *red, *grn, *blu;
    const int cpc=ctx->attribs->colors_per_channel;
    unsigned short gmask;
    unsigned short *table;
    unsigned char *data;
    int ofs;
    /*register unsigned char maxrgb = 0xff;*/

    ximg = RCreateXImage(ctx, ctx->depth, image->width, image->height);
    if (!ximg) {
	return NULL;
    }

    red = image->data[0];
    grn = image->data[1];
    blu = image->data[2];

    data = ximg->image->data;

    if (ctx->vclass == StaticGray)
	gmask = (1<<ctx->depth) - 1; /* use all grays */
    else
	gmask  = cpc*cpc*cpc-1;

    table = computeTable(gmask);

    if (table==NULL) {
	RErrorCode = RERR_NOMEMORY;
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode == RBestMatchRendering) {
        /* fake match */
#ifdef DEBUG
        printf("grayscale match with %d colors per channel\n", cpc);
#endif
	for (y=0, ofs = 0; y<image->height; y++) {
	    for (x=0; x<image->width; x++, ofs++) {
                /* reduce pixel */
                g = table[(red[ofs]*30+grn[ofs]*59+blu[ofs]*11)/100];

                /*data[ofs] = ctx->colors[g].pixel;*/
                XPutPixel(ximg->image, x, y, ctx->colors[g].pixel);
	    }
	}
    } else {
	/* dither */
	short *gerr;
	short *ngerr;
	short *terr;
	int ger;
	const int dg=0xff/gmask;

#ifdef DEBUG
        printf("grayscale dither with %d colors per channel\n", cpc);
#endif
	gerr = (short*)alloca((image->width+2)*sizeof(short));
	ngerr = (short*)alloca((image->width+2)*sizeof(short));
	if (!gerr || !ngerr) {
	    RErrorCode = RERR_NOMEMORY;
	    RDestroyXImage(ctx, ximg);
	    return NULL;
	}
	for (x=0; x<image->width; x++) {
	    gerr[x] = (red[x]*30 + grn[x]*59 + blu[x]*11)/100;
	}
	gerr[x] = 0;
	/* convert and dither the image to XImage */
	for (y=0, ofs=0; y<image->height; y++) {
	    if (y<image->height-1) {
		int x1;
		for (x=0, x1=ofs+image->width; x<image->width; x++, x1++) {
		    ngerr[x] = (red[x1]*30 + grn[x1]*59 + blu[x1]*11)/100;
		}
		/* last column */
		x1--;
		ngerr[x] = (red[x1]*30 + grn[x1]*59 + blu[x1]*11)/100;
	    }
	    for (x=0; x<image->width; x++, ofs++) {
		/* reduce pixel */
                if (gerr[x]>0xff) gerr[x]=0xff; else if (gerr[x]<0) gerr[x]=0;

                g = table[gerr[x]];

                /*data[ofs] = ctx->colors[g].pixel;*/
                XPutPixel(ximg->image, x, y, ctx->colors[g].pixel);
		/* calc error */
		ger = gerr[x] - g*dg;

		/* distribute error */
		g = (ger*3)/8;
		/* x+1, y */
		gerr[x+1]+=g;
		/* x, y+1 */
		ngerr[x]+=g;
		/* x+1, y+1 */
		ngerr[x+1]+=ger-2*g;
	    }
	    /* skip to next line */
	    terr = gerr;
	    gerr = ngerr;
	    ngerr = terr;
	}
    }
    ximg->image->data = (char*)data;

    return ximg;
}


static RXImage*
image2Bitmap(RContext *ctx, RImage *image, int threshold)
{
    RXImage *ximg;
    unsigned char *alpha;
    int x, y;

    ximg = RCreateXImage(ctx, 1, image->width, image->height);
    if (!ximg) {
	return NULL;
    }
    alpha = image->data[3];
    
    for (y = 0; y < image->height; y++) {
	for (x = 0; x < image->width; x++) {
	    XPutPixel(ximg->image, x, y, (*alpha <= threshold ? 0 : 1));
	    alpha++;
	}
    }
    
    return ximg;
}



int 
RConvertImage(RContext *context, RImage *image, Pixmap *pixmap)
{
    RXImage *ximg=NULL;
#ifdef XSHM
    Pixmap tmp;
#endif
    
    assert(context!=NULL);
    assert(image!=NULL);
    assert(pixmap!=NULL);

    /* clear error message */    
    if (context->vclass == TrueColor) {

	if (context->attribs->render_mode == RDitheredRendering
	    && (context->depth == 15 || context->depth == 16))
	    ximg = image2TrueColorD16(context, image);
	else
	    ximg = image2TrueColor(context, image);

    } else if (context->vclass == PseudoColor || context->vclass == StaticColor)
	    ximg = image2PseudoColor(context, image);
    else if (context->vclass == GrayScale || context->vclass == StaticGray)
	ximg = image2GrayScale(context, image);
    
    if (!ximg) {
#ifdef C_ALLOCA
	alloca(0);
#endif
	return False;
    }


    *pixmap = XCreatePixmap(context->dpy, context->drawable, image->width,
			    image->height, context->depth);
    
#ifdef XSHM
    if (context->flags.use_shared_pixmap && ximg->is_shared)
        tmp = R_CreateXImageMappedPixmap(context, ximg);
    else
	tmp = None;
    if (tmp) {
	/*
	 * We have to copy the shm Pixmap into a normal Pixmap because
	 * otherwise, we would have to control when Pixmaps are freed so
	 * that we can detach their shm segments. This is a problem if the
	 * program crash, leaving stale shared memory segments in the
	 * system (lots of them). But with some work, we can optimize
	 * things and remove this XCopyArea. This will require
	 * explicitly freeing all pixmaps when exiting or restarting
	 * wmaker.
	 */
	XCopyArea(context->dpy, tmp, *pixmap, context->copy_gc, 0, 0,
		  image->width, image->height, 0, 0);
	XFreePixmap(context->dpy, tmp);
    } else {
	RPutXImage(context, *pixmap, context->copy_gc, ximg, 0, 0, 0, 0,
		   image->width, image->height);
    }
#else /* !XSHM */
    RPutXImage(context, *pixmap, context->copy_gc, ximg, 0, 0, 0, 0,
	       image->width, image->height);
#endif /* !XSHM */

    RDestroyXImage(context, ximg);

#ifdef C_ALLOCA
    alloca(0);
#endif
    return True;
}


int 
RConvertImageMask(RContext *context, RImage *image, Pixmap *pixmap, 
		  Pixmap *mask, int threshold)
{
    GC gc;
    XGCValues gcv;
    RXImage *ximg=NULL;

    assert(context!=NULL);
    assert(image!=NULL);
    assert(pixmap!=NULL);
    assert(mask!=NULL);

    if (!RConvertImage(context, image, pixmap))
	return False;
    
    if (image->data[3]==NULL) {
	*mask = None;
	return True;
    }

    ximg = image2Bitmap(context, image, threshold);
    
    if (!ximg) {
#ifdef C_ALLOCA
	alloca(0);
#endif
	return False;
    }
    *mask = XCreatePixmap(context->dpy, context->drawable, image->width,
			  image->height, 1);
    gcv.foreground = context->black;
    gcv.background = context->white;
    gcv.graphics_exposures = False;
    gc = XCreateGC(context->dpy, *mask, GCForeground|GCBackground
		   |GCGraphicsExposures, &gcv);
    RPutXImage(context, *mask, gc, ximg, 0, 0, 0, 0,
	       image->width, image->height);
    RDestroyXImage(context, ximg);

#ifdef C_ALLOCA
    alloca(0);
#endif
    return True;
}


Bool
RGetClosestXColor(RContext *context, RColor *color, XColor *retColor)
{
    if (context->vclass == TrueColor) {
	unsigned short rmask, gmask, bmask;
	unsigned short roffs, goffs, boffs;
	unsigned short *rtable, *gtable, *btable;
    
	roffs = context->red_offset;
	goffs = context->green_offset;
	boffs = context->blue_offset;
    
	rmask = context->visual->red_mask >> roffs;
	gmask = context->visual->green_mask >> goffs;
	bmask = context->visual->blue_mask >> boffs;
	
	rtable = computeTable(rmask);
	gtable = computeTable(gmask);
	btable = computeTable(bmask);

        retColor->pixel = (rtable[color->red]<<roffs) |
	    (gtable[color->green]<<goffs) | (btable[color->blue]<<boffs);

	retColor->red = color->red << 8;
	retColor->green = color->green << 8;
	retColor->blue = color->blue << 8;
	retColor->flags = DoRed|DoGreen|DoBlue;
	
    } else if (context->vclass == PseudoColor || context->vclass == StaticColor) {
	const int cpc=context->attribs->colors_per_channel;
	const unsigned short rmask = cpc-1; /* different sizes could be used */
	const unsigned short gmask = rmask; /* for r,g,b */
	const unsigned short bmask = rmask;
	unsigned short *rtable, *gtable, *btable;
	const int cpccpc = cpc*cpc;
	int index;

	rtable = computeTable(rmask);
	gtable = computeTable(gmask);
	btable = computeTable(bmask);

	if (rtable==NULL || gtable==NULL || btable==NULL) {
	    RErrorCode = RERR_NOMEMORY;
	    return False;
	}
	index = rtable[color->red]*cpccpc + gtable[color->green]*cpc 
	    + btable[color->blue];
	*retColor = context->colors[index];
    } else if (context->vclass == GrayScale || context->vclass == StaticGray) {

	const int cpc = context->attribs->colors_per_channel;
	unsigned short gmask;
	unsigned short *table;
	int index;

	if (context->vclass == StaticGray)
	    gmask = (1<<context->depth) - 1; /* use all grays */
	else
	    gmask  = cpc*cpc*cpc-1;

	table = computeTable(gmask);
	if (!table)
	    return False;

	index = table[(color->red*30 + color->green*59 + color->blue*11)/100];
	
	*retColor = context->colors[index];
    } else {
	RErrorCode = RERR_INTERNAL;
	return False;
    }

    return True;
}

