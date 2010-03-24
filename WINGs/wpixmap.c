
#include "WINGsP.h"
#include <cairo-xlib.h>

WMPixmap *WMRetainPixmap(WMPixmap * pixmap)
{
	if (pixmap)
		pixmap->refCount++;

	return pixmap;
}

void WMReleasePixmap(WMPixmap * pixmap)
{
	wassertr(pixmap != NULL);

	pixmap->refCount--;

	if (pixmap->refCount < 1) {
		if (pixmap->pixmap)
			XFreePixmap(pixmap->screen->display, pixmap->pixmap);
		if (pixmap->mask)
			XFreePixmap(pixmap->screen->display, pixmap->mask);
		wfree(pixmap);
	}
}

WMPixmap *WMCreatePixmap(WMScreen * scrPtr, int width, int height, int depth, Bool masked)
{
	WMPixmap *pixPtr;

	pixPtr = wmalloc(sizeof(WMPixmap));
	pixPtr->screen = scrPtr;
	pixPtr->width = width;
	pixPtr->height = height;
	pixPtr->depth = depth;
	pixPtr->refCount = 1;

	pixPtr->pixmap = XCreatePixmap(scrPtr->display, DefaultRootWindow(scrPtr->display),
			width, height, depth);
	if (masked) {
		pixPtr->mask = XCreatePixmap(scrPtr->display, DefaultRootWindow(scrPtr->display),
				width, height, 1);
	} else {
		pixPtr->mask = None;
	}

	return pixPtr;
}

WMPixmap *WMCreatePixmapFromXPixmaps(WMScreen * scrPtr, Pixmap pixmap, Pixmap mask,
				     int width, int height, int depth)
{
	WMPixmap *pixPtr;

	pixPtr = wmalloc(sizeof(WMPixmap));
	pixPtr->screen = scrPtr;
	pixPtr->pixmap = pixmap;
	pixPtr->mask = mask;
	pixPtr->width = width;
	pixPtr->height = height;
	pixPtr->depth = depth;
	pixPtr->refCount = 1;

	return pixPtr;
}

WMPixmap* WMCreatePixmapFromFile(WMScreen *scrPtr, char *fileName)
{
	return WMCreateBlendedPixmapFromFile(scrPtr, fileName, NULL);
}

WMPixmap* WMCreatePixmapFromCairo(WMScreen *scrPtr, cairo_surface_t *image, int threshold)
{
	WMPixmap *pixPtr;
	Pixmap pixmap, mask;
	cairo_t *cairo;
	cairo_surface_t *surface;

	pixmap = XCreatePixmap(scrPtr->display,
			DefaultRootWindow(scrPtr->display),
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image),
			scrPtr->depth);

	mask = XCreatePixmap(scrPtr->display, DefaultRootWindow(scrPtr->display),
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image),
			1);


	surface= cairo_xlib_surface_create(scrPtr->display,
			pixmap,
			scrPtr->visual,
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image));
	cairo= cairo_create(surface);

	cairo_set_source_surface(cairo, image, 0, 0);
	cairo_rectangle(cairo, 0, 0,
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image));
	cairo_fill(cairo);

	cairo_destroy(cairo);
	cairo_surface_destroy(surface);


	surface= cairo_xlib_surface_create_for_bitmap(scrPtr->display,
			mask,
			ScreenOfDisplay(scrPtr->display, scrPtr->screen),
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image));
	cairo= cairo_create(surface);

	cairo_set_source_surface(cairo, image, 0, 0);
	cairo_rectangle(cairo, 0, 0,
			cairo_image_surface_get_width(image),
			cairo_image_surface_get_height(image));
	//XXX create a buffer rendering version of the image and apply the
	// threshold there. then render it to the bitmap
	cairo_fill(cairo);

	cairo_destroy(cairo);
	cairo_surface_destroy(surface);


	pixPtr = wmalloc(sizeof(WMPixmap));
	pixPtr->screen = scrPtr;
	pixPtr->pixmap = pixmap;
	pixPtr->mask = mask;
	pixPtr->width = cairo_image_surface_get_width(image);
	pixPtr->height = cairo_image_surface_get_height(image);
	pixPtr->depth = scrPtr->depth;
	pixPtr->refCount = 1;

	return pixPtr;
}

WMPixmap* WMCreateBlendedPixmapFromCairo(WMScreen *scrPtr, cairo_surface_t *image, cairo_pattern_t *color)
{
	return NULL;
}


WMPixmap* WMCreateBlendedPixmapFromFile(WMScreen *scrPtr, char *fileName, cairo_pattern_t *color)
{
	WMPixmap *pixPtr;
	cairo_surface_t *surf = cairo_image_surface_create_from_png(fileName);

	if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy(surf);
		return NULL;
	}

	//XXX apply color

	pixPtr = WMCreatePixmapFromCairo(scrPtr, surf, 127);

	cairo_surface_destroy(surf);

	return pixPtr;
}

WMPixmap *WMCreatePixmapFromXPMData(WMScreen * scrPtr, char **data)
{
#if 0 //TODO
	WMPixmap *pixPtr;
	RImage *image;

	image = RGetImageFromXPMData(scrPtr->rcontext, data);
	if (!image)
		return NULL;

	pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 127);

	RReleaseImage(image);

	return pixPtr;
#endif
	return NULL;
}

Pixmap WMGetPixmapXID(WMPixmap * pixmap)
{
	wassertrv(pixmap != NULL, None);

	return pixmap->pixmap;
}

Pixmap WMGetPixmapMaskXID(WMPixmap * pixmap)
{
	wassertrv(pixmap != NULL, None);

	return pixmap->mask;
}

WMSize WMGetPixmapSize(WMPixmap * pixmap)
{
	WMSize size = { 0, 0 };

	wassertrv(pixmap != NULL, size);

	size.width = pixmap->width;
	size.height = pixmap->height;

	return size;
}

WMPixmap *WMGetSystemPixmap(WMScreen * scr, int image)
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

void WMDrawPixmap(WMPixmap * pixmap, Drawable d, int x, int y)
{
	WMScreen *scr = pixmap->screen;

	XSetClipMask(scr->display, scr->clipGC, pixmap->mask);
	XSetClipOrigin(scr->display, scr->clipGC, x, y);

	XCopyArea(scr->display, pixmap->pixmap, d, scr->clipGC, 0, 0, pixmap->width, pixmap->height, x, y);
}
