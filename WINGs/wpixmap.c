
#include "WINGsP.h"

#include <wraster.h>


WMPixmap*
WMRetainPixmap(WMPixmap *pixmap)
{
    if (pixmap)
	pixmap->refCount++;

    return pixmap;
}


void
WMReleasePixmap(WMPixmap *pixmap)
{
    wassertr(pixmap!=NULL);

    pixmap->refCount--;

    if (pixmap->refCount<1) {
	if (pixmap->pixmap)
	    XFreePixmap(pixmap->screen->display, pixmap->pixmap);
	if (pixmap->mask)
	    XFreePixmap(pixmap->screen->display, pixmap->mask);
	wfree(pixmap);
    }
}


WMPixmap*
WMCreatePixmap(WMScreen *scrPtr, int width, int height, int depth, Bool masked)
{
    WMPixmap *pixPtr;

    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->width = width;
    pixPtr->height = height;
    pixPtr->depth = depth;
    pixPtr->refCount = 1;

    pixPtr->pixmap = XCreatePixmap(scrPtr->display, W_DRAWABLE(scrPtr),
				   width, height, depth);
    if (masked) {
	    pixPtr->mask = XCreatePixmap(scrPtr->display, W_DRAWABLE(scrPtr),
					 width, height, 1);
    } else {
	pixPtr->mask = None;
    }

    return pixPtr;    
}


WMPixmap*
WMCreatePixmapFromXPixmaps(WMScreen *scrPtr, Pixmap pixmap, Pixmap mask,
			   int width, int height, int depth)
{
    WMPixmap *pixPtr;

    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->pixmap = pixmap;
    pixPtr->mask = mask;
    pixPtr->width = width;
    pixPtr->height = height;
    pixPtr->depth = depth;
    pixPtr->refCount = 1;
    
    return pixPtr;
}




WMPixmap*
WMCreatePixmapFromFile(WMScreen *scrPtr, char *fileName)
{
    WMPixmap *pixPtr;
    RImage *image;
    
    image = RLoadImage(scrPtr->rcontext, fileName, 0);
    if (!image)
	return NULL;

    pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 127);

    RDestroyImage(image);
    
    return pixPtr;
}


WMPixmap*
WMCreatePixmapFromRImage(WMScreen *scrPtr, RImage *image, int threshold)
{
    WMPixmap *pixPtr;
    Pixmap pixmap, mask;

    if (!RConvertImageMask(scrPtr->rcontext, image, &pixmap, &mask, 
			   threshold)) {
	return NULL;
    }
    
    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->pixmap = pixmap;
    pixPtr->mask = mask;
    pixPtr->width = image->width;
    pixPtr->height = image->height;
    pixPtr->depth = scrPtr->depth;
    pixPtr->refCount = 1;

    return pixPtr;    
}


WMPixmap*
WMCreateBlendedPixmapFromFile(WMScreen *scrPtr, char *fileName, RColor *color)
{
    WMPixmap *pixPtr;
    RImage *image;

    
    image = RLoadImage(scrPtr->rcontext, fileName, 0);
    if (!image)
	return NULL;
    
    RCombineImageWithColor(image, color);

    pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 0);
    
    RDestroyImage(image);
    
    return pixPtr;
}


WMPixmap*
WMCreatePixmapFromXPMData(WMScreen *scrPtr, char **data)
{
    WMPixmap *pixPtr;
    RImage *image;

    image = RGetImageFromXPMData(scrPtr->rcontext, data);
    if (!image)
	return NULL;
    
    pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 127);

    RDestroyImage(image);
    
    return pixPtr;
}


Pixmap
WMGetPixmapXID(WMPixmap *pixmap)
{
    wassertrv(pixmap != NULL, None);

    return pixmap->pixmap;
}


Pixmap
WMGetPixmapMaskXID(WMPixmap *pixmap)
{
    wassertrv(pixmap != NULL, None);

    return pixmap->mask;
}


WMSize 
WMGetPixmapSize(WMPixmap *pixmap)
{
    WMSize size = {0,0};

    wassertrv(pixmap != NULL, size);
    
    size.width = pixmap->width;
    size.height = pixmap->height;

    return size;
}


WMPixmap*
WMGetSystemPixmap(WMScreen *scr, int image)
{
    switch (image) {
     case WSIReturnArrow:
	return WMRetainPixmap(scr->buttonArrow);
	
     case WSIHighlightedReturnArrow:
	return WMRetainPixmap(scr->pushedButtonArrow);
	
     case WSIScrollerDimple:
	return WMRetainPixmap(scr->scrollerDimple);
	
     case WSIArrowLeft:
	return WMRetainPixmap(scr->leftArrow);
	
     case WSIHighlightedArrowLeft:
	return WMRetainPixmap(scr->hiLeftArrow);
	
     case WSIArrowRight:
	return WMRetainPixmap(scr->rightArrow);
	
     case WSIHighlightedArrowRight:
	return WMRetainPixmap(scr->hiRightArrow);
	
     case WSIArrowUp:
	return WMRetainPixmap(scr->upArrow);
	
     case WSIHighlightedArrowUp:
	return WMRetainPixmap(scr->hiUpArrow);
	
     case WSIArrowDown:
	return WMRetainPixmap(scr->downArrow);
	
     case WSIHighlightedArrowDown:
	return WMRetainPixmap(scr->hiDownArrow);

     case WSICheckMark:
	return WMRetainPixmap(scr->checkMark);

     default:
	return NULL;
    }
}



void
WMDrawPixmap(WMPixmap *pixmap, Drawable d, int x, int y)
{
    WMScreen *scr = pixmap->screen;

    XSetClipMask(scr->display, scr->clipGC, pixmap->mask);
    XSetClipOrigin(scr->display, scr->clipGC, x, y);

    XCopyArea(scr->display, pixmap->pixmap, d, scr->clipGC, 0, 0, 
	      pixmap->width, pixmap->height, x, y);
}
