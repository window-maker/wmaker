
#include <unistd.h>

#include "WINGsP.h"

#include <X11/Xutil.h>

#include "GNUstep.h"

extern struct W_Application WMApplication;

void WMSetApplicationIconWindow(WMScreen * scr, Window window)
{
	scr->applicationIconWindow = window;

	if (scr->groupLeader) {
		XWMHints *hints;

		hints = XGetWMHints(scr->display, scr->groupLeader);
		hints->flags |= IconWindowHint;
		hints->icon_window = window;

		XSetWMHints(scr->display, scr->groupLeader, hints);
		XFree(hints);
	}
}

void WMSetApplicationIconImage(WMScreen * scr, cairo_surface_t *image)
{
	WMPixmap *icon;

	if (scr->applicationIconImage == image)
		return;

	if (scr->applicationIconImage)
        cairo_surface_destroy(scr->applicationIconImage);

	scr->applicationIconImage = cairo_surface_reference(image);

	/* TODO: check whether we should set the pixmap only if there's none yet */
	if (image != NULL && (icon = WMCreatePixmapFromCairo(scr, image, 128)) != NULL) {
		WMSetApplicationIconPixmap(scr, icon);
		WMReleasePixmap(icon);
	}
}

cairo_surface_t *WMGetApplicationIconImage(WMScreen * scr)
{
	return scr->applicationIconImage;
}

void WMSetApplicationIconPixmap(WMScreen * scr, WMPixmap * icon)
{
	if (scr->applicationIconPixmap == icon)
		return;

	if (scr->applicationIconPixmap)
		WMReleasePixmap(scr->applicationIconPixmap);

	scr->applicationIconPixmap = WMRetainPixmap(icon);

	if (scr->groupLeader) {
		XWMHints *hints;

		hints = XGetWMHints(scr->display, scr->groupLeader);
		hints->flags |= IconPixmapHint | IconMaskHint;
		hints->icon_pixmap = (icon != NULL ? icon->pixmap : None);
		hints->icon_mask = (icon != NULL ? icon->mask : None);

		XSetWMHints(scr->display, scr->groupLeader, hints);
		XFree(hints);
	}
}

WMPixmap *WMGetApplicationIconPixmap(WMScreen * scr)
{
	return scr->applicationIconPixmap;
}

WMPixmap *WMCreateApplicationIconBlendedPixmap(WMScreen * scr, cairo_pattern_t *color)
{
	WMPixmap *pix;

	if (scr->applicationIconImage) {
        int release = 0;

		if (!color) {
            release = 1;
			color = cairo_pattern_create_rgb(0xae/255.0, 0xaa/255.0, 0xae/255.0);
        }

		pix = WMCreateBlendedPixmapFromCairo(scr, scr->applicationIconImage, color);

        if (release)
            cairo_pattern_destroy(color);
	} else {
		pix = NULL;
	}

	return pix;
}

void WMSetApplicationHasAppIcon(WMScreen * scr, Bool flag)
{
	scr->aflags.hasAppIcon = ((flag == 0) ? 0 : 1);
}

void W_InitApplication(WMScreen * scr)
{
	Window leader;
	XClassHint *classHint;
	XWMHints *hints;

	leader = XCreateSimpleWindow(scr->display, scr->rootWin, -1, -1, 1, 1, 0, 0, 0);

	if (!scr->aflags.simpleApplication) {
		classHint = XAllocClassHint();
		classHint->res_name = "groupLeader";
		classHint->res_class = WMApplication.applicationName;
		XSetClassHint(scr->display, leader, classHint);
		XFree(classHint);

		XSetCommand(scr->display, leader, WMApplication.argv, WMApplication.argc);

		hints = XAllocWMHints();

		hints->flags = WindowGroupHint;
		hints->window_group = leader;

		/* This code will never actually be reached, because to have
		 * scr->applicationIconPixmap set we need to have a screen first,
		 * but this function is called in the screen creation process.
		 * -Dan
		 */
		if (scr->applicationIconPixmap) {
			hints->flags |= IconPixmapHint;
			hints->icon_pixmap = scr->applicationIconPixmap->pixmap;
			if (scr->applicationIconPixmap->mask) {
				hints->flags |= IconMaskHint;
				hints->icon_mask = scr->applicationIconPixmap->mask;
			}
		}

		XSetWMHints(scr->display, leader, hints);

		XFree(hints);
	}
	scr->groupLeader = leader;
}
