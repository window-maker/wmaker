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

#define MIN(a,b)	((a)<(b) ? (a) : (b))


typedef struct RConversionTable {
    unsigned short table[256];
    unsigned short index;
    struct RConversionTable *next;
} RConversionTable;


/*
 * Lookup table for index*3/8
 */
static char errorTable1[]={
    0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 
	12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 
	18, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 
	24, 24, 24, 25, 25, 25, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 
	30, 30, 30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 
	36, 36, 36, 37, 37, 37, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 
	42, 42, 42, 43, 43, 43, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 
	48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 
	54, 54, 54, 55, 55, 55, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 
	60, 60, 60, 61, 61, 61, 62, 62, 63, 63, 63, 64, 64, 64, 65, 65, 
	66, 66, 66, 67, 67, 67, 68, 68, 69, 69, 69, 70, 70, 70, 71, 71, 
	72, 72, 72, 73, 73, 73, 74, 74, 75, 75, 75, 76, 76, 76, 77, 77, 
	78, 78, 78, 79, 79, 79, 80, 80, 81, 81, 81, 82, 82, 82, 83, 83, 
	84, 84, 84, 85, 85, 85, 86, 86, 87, 87, 87, 88, 88, 88, 89, 89, 
	90, 90, 90, 91, 91, 91, 92, 92, 93, 93, 93, 94, 94, 94, 95 
};
/*
 * Lookup table for index*2/8
 */
static char errorTable2[]={
    0, 1, 2, 1, 2, 3, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 
	4, 5, 6, 5, 6, 7, 6, 7, 6, 7, 8, 7, 8, 9, 8, 9, 
	8, 9, 10, 9, 10, 11, 10, 11, 10, 11, 12, 11, 12, 13, 12, 13, 
	12, 13, 14, 13, 14, 15, 14, 15, 14, 15, 16, 15, 16, 17, 16, 17, 
	16, 17, 18, 17, 18, 19, 18, 19, 18, 19, 20, 19, 20, 21, 20, 21, 
	20, 21, 22, 21, 22, 23, 22, 23, 22, 23, 24, 23, 24, 25, 24, 25, 
	24, 25, 26, 25, 26, 27, 26, 27, 26, 27, 28, 27, 28, 29, 28, 29, 
	28, 29, 30, 29, 30, 31, 30, 31, 30, 31, 32, 31, 32, 33, 32, 33, 
	32, 33, 34, 33, 34, 35, 34, 35, 34, 35, 36, 35, 36, 37, 36, 37, 
	36, 37, 38, 37, 38, 39, 38, 39, 38, 39, 40, 39, 40, 41, 40, 41, 
	40, 41, 42, 41, 42, 43, 42, 43, 42, 43, 44, 43, 44, 45, 44, 45, 
	44, 45, 46, 45, 46, 47, 46, 47, 46, 47, 48, 47, 48, 49, 48, 49, 
	48, 49, 50, 49, 50, 51, 50, 51, 50, 51, 52, 51, 52, 53, 52, 53, 
	52, 53, 54, 53, 54, 55, 54, 55, 54, 55, 56, 55, 56, 57, 56, 57, 
	56, 57, 58, 57, 58, 59, 58, 59, 58, 59, 60, 59, 60, 61, 60, 61, 
	60, 61, 62, 61, 62, 63, 62, 63, 62, 63, 64, 63, 64, 65, 64 
};



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
image2TrueColor(RContext *ctx, RImage *image)
{
    RXImage *ximg;
    register int x, y, r, g, b;
    unsigned char *red, *grn, *blu;
    unsigned long pixel;
    unsigned short rmask, gmask, bmask;
    unsigned short roffs, goffs, boffs;
    unsigned short *rtable, *gtable, *btable;

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
        sprintf(RErrorString, "out of memory");
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode==RM_MATCH) {
        /* fake match */
#ifdef DEBUG
        puts("true color match");
#endif
	for (y=0; y < image->height; y++) {
	    for (x=0; x < image->width; x++) {
		/* reduce pixel */
                r = rtable[*red++];
                g = gtable[*grn++];
                b = btable[*blu++];
                pixel = (r<<roffs) | (g<<goffs) | (b<<boffs);
		XPutPixel(ximg->image, x, y, pixel);
	    }
	}
    } else {
        /* dither */
	unsigned char *rerr, *gerr, *berr;
	unsigned char *nrerr, *ngerr, *nberr;
	unsigned char *rerr_, *gerr_, *berr_;
	unsigned char *nrerr_, *ngerr_, *nberr_;
	unsigned char *terr;
	register int ac, err;
	int width = image->width;
	int dr=0xff-rmask;
	int dg=0xff-gmask;
        int db=0xff-bmask;

	while ((dr & 1)==0) dr = dr >> 1;
	while ((dg & 1)==0) dg = dg >> 1;
	while ((db & 1)==0) db = db >> 1;

#ifdef DEBUG
        puts("true color dither");
#endif
	rerr_ = rerr = (unsigned char*)alloca((width+2)*sizeof(char));
	gerr_ = gerr = (unsigned char*)alloca((width+2)*sizeof(char));
	berr_ = berr = (unsigned char*)alloca((width+2)*sizeof(char));
	nrerr_ = nrerr = (unsigned char*)alloca((width+2)*sizeof(char));
	ngerr_ = ngerr = (unsigned char*)alloca((width+2)*sizeof(char));
	nberr_ = nberr = (unsigned char*)alloca((width+2)*sizeof(char));
	if (!rerr || !gerr || !berr || !nrerr || !ngerr || !nberr) {
	    sprintf(RErrorString, "out of memory");
	    RDestroyXImage(ctx, ximg);
	    return NULL;
	}
	for (x=0; x<width; x++) {
	    rerr[x] = *red++;
	    gerr[x] = *grn++;
	    berr[x] = *blu++;
	}
        rerr[x] = gerr[x] = berr[x] = 0;
        /* convert and dither the image to XImage */
	for (y=0; y<image->height; y++) {
	    if (y<image->height-1) {
		memcpy(nrerr, red, width);
		memcpy(ngerr, grn, width);
		memcpy(nberr, blu, width);
		red+=width;
		grn+=width;
		blu+=width;
		/* last column */
		nrerr[x] = *(red-1);
		ngerr[x] = *(grn-1);
		nberr[x] = *(blu-1);
	    }
	    for (x=0; x<width; x++) {
		/* reduce pixel */
                pixel = (rtable[*rerr]<<roffs) | (gtable[*gerr]<<goffs)
		    | (btable[*berr]<<boffs);
		XPutPixel(ximg->image, x, y, pixel);
		/* calc error */
		err = *rerr&dr;
		rerr++;
		/* distribute error */
		ac = errorTable1[err] + *rerr;
		*rerr=MIN(ac, 0xff);
		ac = errorTable1[err] + *nrerr;
		*nrerr=MIN(ac, 0xff);
		nrerr++;
		ac = *nrerr + errorTable2[err];
		*nrerr=MIN(ac,0xff);

		/* calc error */
		err = *gerr&dg;
		gerr++;
		/* distribute error */
		ac = errorTable1[err] + *gerr;
		*gerr=MIN(ac, 0xff);
		ac = errorTable1[err] + *ngerr;
		*ngerr=MIN(ac, 0xff);
		ngerr++;
		ac = *ngerr + errorTable2[err];
		*ngerr=MIN(ac,0xff);

		/* calc error */
		err = *berr&db;
		berr++;
		/* distribute error */
		ac = errorTable1[err] + *berr;
		*berr=MIN(ac, 0xff);
		ac = errorTable1[err] + *nberr;
		*nberr=MIN(ac, 0xff);
		nberr++;
		ac = *nberr + errorTable2[err];
		*nberr=MIN(ac,0xff);
	    }
	    /* skip to next line */
	    rerr = nrerr_;
	    nrerr = rerr_;
	    terr = nrerr_;
	    nrerr_ = rerr_;
	    rerr_ = terr;
	    
	    gerr = ngerr_;
	    ngerr = gerr_;
	    terr = ngerr_;
	    ngerr_ = gerr_;
	    gerr_ = terr;

	    berr = nberr_;
	    nberr = berr_;
	    terr = nberr_;
	    nberr_ = berr_;
	    berr_ = terr;
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
        sprintf(RErrorString, "out of memory");
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode == RM_MATCH) {
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
	    sprintf(RErrorString, "out of memory");
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
        sprintf(RErrorString, "out of memory");
        RDestroyXImage(ctx, ximg);
        return NULL;
    }

    if (ctx->attribs->render_mode == RM_MATCH) {
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
	    sprintf(RErrorString, "out of memory");
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

    assert(context!=NULL);
    assert(image!=NULL);
    assert(pixmap!=NULL);

    /* clear error message */
    RErrorString[0] = 0;

    if (context->vclass == TrueColor)
      ximg = image2TrueColor(context, image);
    else if (context->vclass == PseudoColor || context->vclass == StaticColor)
      ximg = image2PseudoColor(context, image);
    else if (context->vclass == GrayScale || context->vclass == StaticGray)
      ximg = image2GrayScale(context, image);

    if (!ximg) {
	strcat(RErrorString, ":could not convert RImage to XImage");
#ifdef C_ALLOCA
	alloca(0);
#endif
	return False;
    }
    *pixmap = XCreatePixmap(context->dpy, context->drawable, image->width,
			    image->height, context->depth);
    RPutXImage(context, *pixmap, context->copy_gc, ximg, 0, 0, 0, 0,
	      image->width, image->height);

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
	strcat(RErrorString, ":could not convert RImage mask to XImage");
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
    RErrorString[0] = 0;

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
	    (rtable[color->green]<<goffs) | (rtable[color->blue]<<boffs);

	retColor->red = rtable[color->red] << 8;	
	retColor->green = rtable[color->green] << 8;
	retColor->blue = rtable[color->blue] << 8;
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
	    sprintf(RErrorString, "out of memory");
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
	sprintf(RErrorString, "internal bug:unsupported visual:%i",
		context->vclass);
	return False;
    }

    return True;
}
