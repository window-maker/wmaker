/* icon.c - window icon and dock and appicon parent
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <wraster.h>
#include <sys/stat.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "texture.h"
#include "window.h"
#include "icon.h"
#include "actions.h"
#include "funcs.h"
#include "stacking.h"
#include "application.h"
#include "defaults.h"
#include "appicon.h"
#include "wmspec.h"

/**** Global varianebles ****/
extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask
#define CACHE_ICON_PATH "/Library/WindowMaker/CachedPixmaps"
#define ICON_BORDER 3

extern Cursor wCursor[WCUR_LAST];

static void miniwindowExpose(WObjDescriptor * desc, XEvent * event);
static void miniwindowMouseDown(WObjDescriptor * desc, XEvent * event);
static void miniwindowDblClick(WObjDescriptor * desc, XEvent * event);

static WIcon *icon_create_core(WScreen *scr, int coord_x, int coord_y);

static void get_pixmap_icon_from_icon_win(WIcon *icon);
static int get_pixmap_icon_from_wm_hints(WIcon *icon);
static void get_pixmap_icon_from_user_icon(WIcon *icon);
static void get_pixmap_icon_from_default_icon(WIcon *icon);

static void icon_update_pixmap(WIcon *icon, RImage *image);

static RImage *get_default_image(WScreen *scr);
/****** Notification Observers ******/

static void appearanceObserver(void *self, WMNotification * notif)
{
	WIcon *icon = (WIcon *) self;
	uintptr_t flags = (uintptr_t)WMGetNotificationClientData(notif);

	if ((flags & WTextureSettings) || (flags & WFontSettings))
		icon->force_paint = 1;

	wIconPaint(icon);

	/* so that the appicon expose handlers will paint the appicon specific
	 * stuff */
	XClearArea(dpy, icon->core->window, 0, 0, icon->core->width, icon->core->height, True);
}

static void tileObserver(void *self, WMNotification * notif)
{
	WIcon *icon = (WIcon *) self;

	icon->force_paint = 1;
	wIconPaint(icon);

	XClearArea(dpy, icon->core->window, 0, 0, 1, 1, True);
}

/************************************/

INLINE static void getSize(Drawable d, unsigned int *w, unsigned int *h, unsigned int *dep)
{
	Window rjunk;
	int xjunk, yjunk;
	unsigned int bjunk;

	XGetGeometry(dpy, d, &rjunk, &xjunk, &yjunk, w, h, &bjunk, dep);
}

WIcon *icon_create_for_wwindow(WWindow *wwin)
{
	WScreen *scr = wwin->screen_ptr;
	WIcon *icon;
	char *file;

	icon = icon_create_core(scr, wwin->icon_x, wwin->icon_y);

	icon->owner = wwin;
	if (wwin->wm_hints && (wwin->wm_hints->flags & IconWindowHint)) {
		if (wwin->client_win == wwin->main_window) {
			WApplication *wapp;
			/* do not let miniwindow steal app-icon's icon window */
			wapp = wApplicationOf(wwin->client_win);
			if (!wapp || wapp->app_icon == NULL)
				icon->icon_win = wwin->wm_hints->icon_window;
		} else {
			icon->icon_win = wwin->wm_hints->icon_window;
		}
	}
#ifdef NO_MINIWINDOW_TITLES
	icon->show_title = 0;
#else
	icon->show_title = 1;
#endif

	icon->icon_name = wNETWMGetIconName(wwin->client_win);
	if (icon->icon_name)
		wwin->flags.net_has_icon_title = 1;
	else
		wGetIconName(dpy, wwin->client_win, &icon->icon_name);

	/* Get the application icon, default included */
	file = get_default_icon_filename(scr, wwin->wm_instance, wwin->wm_class, NULL, True);
	if (file) {
		icon->file = wstrdup(file);
		icon->file_image = get_rimage_from_file(scr, icon->file, wPreferences.icon_size);
		wfree(file);
	}

	icon->tile_type = TILE_NORMAL;

	wIconUpdate(icon);

	WMAddNotificationObserver(appearanceObserver, icon, WNIconAppearanceSettingsChanged, icon);
	WMAddNotificationObserver(tileObserver, icon, WNIconTileSettingsChanged, icon);

	return icon;
}

WIcon *icon_create_for_dock(WScreen *scr, char *command, char *wm_instance, char *wm_class, int tile)
{
	WIcon *icon;
	char *file = NULL;

	icon = icon_create_core(scr, 0, 0);

	/* Search the icon using instance and class, without default icon */
	file = get_default_icon_filename(scr, wm_instance, wm_class, command, False);
	if (file) {
		icon->file = wstrdup(file);
		icon->file_image = get_rimage_from_file(scr, icon->file, wPreferences.icon_size);
		wfree(file);
	}

	icon->tile_type = tile;

	wIconUpdate(icon);

	WMAddNotificationObserver(appearanceObserver, icon, WNIconAppearanceSettingsChanged, icon);
	WMAddNotificationObserver(tileObserver, icon, WNIconTileSettingsChanged, icon);

	return icon;
}

static WIcon *icon_create_core(WScreen *scr, int coord_x, int coord_y)
{
	WIcon *icon;
	unsigned long vmask = 0;
	XSetWindowAttributes attribs;

	icon = wmalloc(sizeof(WIcon));
	icon->core = wCoreCreateTopLevel(scr,
					 coord_x,
					 coord_y,
					 wPreferences.icon_size,
					 wPreferences.icon_size,
					 0, scr->w_depth, scr->w_visual, scr->w_colormap);

	if (wPreferences.use_saveunders) {
		vmask = CWSaveUnder;
		attribs.save_under = True;
	}

	/* a white border for selecting it */
	vmask |= CWBorderPixel;
	attribs.border_pixel = scr->white_pixel;

	XChangeWindowAttributes(dpy, icon->core->window, vmask, &attribs);

	/* will be overriden if this is a application icon */
	icon->core->descriptor.handle_mousedown = miniwindowMouseDown;
	icon->core->descriptor.handle_expose = miniwindowExpose;
	icon->core->descriptor.parent_type = WCLASS_MINIWINDOW;
	icon->core->descriptor.parent = icon;

	icon->core->stacking = wmalloc(sizeof(WStacking));
	icon->core->stacking->above = NULL;
	icon->core->stacking->under = NULL;
	icon->core->stacking->window_level = NORMAL_ICON_LEVEL;
	icon->core->stacking->child_of = NULL;

	/* Icon image */
	icon->file = NULL;
	icon->file_image = NULL;

	return icon;
}

void wIconDestroy(WIcon * icon)
{
	WCoreWindow *core = icon->core;
	WScreen *scr = core->screen_ptr;

	WMRemoveNotificationObserver(icon);

	if (icon->handlerID)
		WMDeleteTimerHandler(icon->handlerID);

	if (icon->icon_win) {
		int x = 0, y = 0;

		if (icon->owner) {
			x = icon->owner->icon_x;
			y = icon->owner->icon_y;
		}
		XUnmapWindow(dpy, icon->icon_win);
		XReparentWindow(dpy, icon->icon_win, scr->root_win, x, y);
	}
	if (icon->icon_name)
		XFree(icon->icon_name);

	if (icon->pixmap)
		XFreePixmap(dpy, icon->pixmap);

	if (icon->file)
		wfree(icon->file);

	if (icon->file_image != NULL)
		RReleaseImage(icon->file_image);

	wCoreDestroy(icon->core);
	wfree(icon);
}

static void drawIconTitle(WScreen * scr, Pixmap pixmap, int height)
{
	XFillRectangle(dpy, pixmap, scr->icon_title_texture->normal_gc, 0, 0, wPreferences.icon_size, height + 1);
	XDrawLine(dpy, pixmap, scr->icon_title_texture->light_gc, 0, 0, wPreferences.icon_size, 0);
	XDrawLine(dpy, pixmap, scr->icon_title_texture->light_gc, 0, 0, 0, height + 1);
	XDrawLine(dpy, pixmap, scr->icon_title_texture->dim_gc,
		  wPreferences.icon_size - 1, 0, wPreferences.icon_size - 1, height + 1);
}

static void icon_update_pixmap(WIcon *icon, RImage *image)
{
	RImage *tile;
	Pixmap pixmap;
	int x, y, sx, sy;
	unsigned w, h;
	int theight = 0;
	WScreen *scr = icon->core->screen_ptr;
	int titled = icon->show_title;

	if (icon->tile_type == TILE_NORMAL) {
		tile = RCloneImage(scr->icon_tile);
	} else {
		assert(scr->clip_tile);
		tile = RCloneImage(scr->clip_tile);
	}

	if (image) {
		w = (image->width > wPreferences.icon_size)
		    ? wPreferences.icon_size : image->width;
		x = (wPreferences.icon_size - w) / 2;
		sx = (image->width - w) / 2;

		if (titled)
			theight = WMFontHeight(scr->icon_title_font);

		h = (image->height + theight > wPreferences.icon_size
		     ? wPreferences.icon_size - theight : image->height);
		y = theight + (wPreferences.icon_size - theight - h) / 2;
		sy = (image->height - h) / 2;

		RCombineArea(tile, image, sx, sy, w, h, x, y);
	}

	if (icon->shadowed) {
		RColor color;

		color.red = scr->icon_back_texture->light.red >> 8;
		color.green = scr->icon_back_texture->light.green >> 8;
		color.blue = scr->icon_back_texture->light.blue >> 8;
		color.alpha = 150;	/* about 60% */
		RClearImage(tile, &color);
	}

	if (icon->highlighted) {
		RColor color;

		color.red = color.green = color.blue = 0;
		color.alpha = 160;
		RLightImage(tile, &color);
	}

	if (!RConvertImage(scr->rcontext, tile, &pixmap))
		wwarning(_("error rendering image:%s"), RMessageForError(RErrorCode));

	RReleaseImage(tile);

	if (titled)
		drawIconTitle(scr, pixmap, theight);

	icon->pixmap = pixmap;
}

void wIconChangeTitle(WIcon * icon, char *new_title)
{
	int changed;

	changed = (new_title == NULL && icon->icon_name != NULL)
	    || (new_title != NULL && icon->icon_name == NULL);

	if (icon->icon_name != NULL)
		XFree(icon->icon_name);

	icon->icon_name = new_title;

	if (changed)
		icon->force_paint = 1;
	wIconPaint(icon);
}

RImage *wIconValidateIconSize(RImage *icon, int max_size)
{
	RImage *nimage;

	if (!icon)
		return NULL;

	/* We should hold "ICON_BORDER" (~2) pixels to include the icon border */
	if ((icon->width - max_size) > -ICON_BORDER ||
	    (icon->height - max_size) > -ICON_BORDER) {
		nimage = RScaleImage(icon, max_size - ICON_BORDER,
				     (icon->height * (max_size - ICON_BORDER) / icon->width));
		RReleaseImage(icon);
		icon = nimage;
	}

	return icon;
}

Bool wIconChangeImageFile(WIcon * icon, char *file)
{
	WScreen *scr = icon->core->screen_ptr;
	RImage *image;
	char *path;
	int error = 0;

	if (icon->file_image) {
		RReleaseImage(icon->file_image);
		icon->file_image = NULL;
	}

	if (!file) {
		wIconUpdate(icon);
		return True;
	}

	path = FindImage(wPreferences.icon_path, file);

	if (path && (image = RLoadImage(scr->rcontext, path, 0))) {
		icon->file_image = wIconValidateIconSize(image, wPreferences.icon_size);
		wIconUpdate(icon);
	} else {
		error = 1;
	}

	if (path)
		wfree(path);

	return !error;
}

static char *get_name_for_wwin(WWindow *wwin)
{
	return get_name_for_instance_class(wwin->wm_instance, wwin->wm_class);
}

char *get_name_for_instance_class(char *wm_instance, char *wm_class)
{
	char *suffix;
	int len;

	if (wm_class && wm_instance) {
		len = strlen(wm_class) + strlen(wm_instance) + 2;
		suffix = wmalloc(len);
		snprintf(suffix, len, "%s.%s", wm_instance, wm_class);
	} else if (wm_class) {
		len = strlen(wm_class) + 1;
		suffix = wmalloc(len);
		snprintf(suffix, len, "%s", wm_class);
	} else if (wm_instance) {
		len = strlen(wm_instance) + 1;
		suffix = wmalloc(len);
		snprintf(suffix, len, "%s", wm_instance);
	} else {
		return NULL;
	}

	return suffix;
}

static char *get_icon_cache_path(void)
{
	char *prefix, *path;
	int len, ret;

	prefix = wusergnusteppath();
	len = strlen(prefix) + strlen(CACHE_ICON_PATH) + 2;
	path = wmalloc(len);
	snprintf(path, len, "%s%s/", prefix, CACHE_ICON_PATH);

	/* If the folder exists, exit */
	if (access(path, F_OK) == 0)
		return path;

	/* Create the folder */
	ret = wmkdirhier((const char *) path);

	/* Exit 1 on success, 0 on failure */
	if (ret == 1)
		return path;

	/* Fail */
	wfree(path);
	return NULL;
}

static RImage *get_wwindow_image_from_wmhints(WWindow *wwin, WIcon *icon)
{
	RImage *image = NULL;
	XWMHints *hints = wwin->wm_hints;

	if (hints && (hints->flags & IconPixmapHint) && hints->icon_pixmap != None)
		image = RCreateImageFromDrawable(icon->core->screen_ptr->rcontext,
						 hints->icon_pixmap,
						 (hints->flags & IconMaskHint)
						 ? hints->icon_mask : None);

	return image;
}

/*
 * wIconStore--
 * 	Stores the client supplied icon at CACHE_ICON_PATH
 * and returns the path for that icon. Returns NULL if there is no
 * client supplied icon or on failure.
 *
 * Side effects:
 * 	New directories might be created.
 */
char *wIconStore(WIcon * icon)
{
	char *path, *dir_path, *file;
	int len = 0;
	RImage *image = NULL;
	WWindow *wwin = icon->owner;

	if (!wwin)
		return NULL;

	dir_path = get_icon_cache_path();
	if (!dir_path)
		return NULL;

	file = get_name_for_wwin(wwin);
	if (!file) {
		wfree(dir_path);
		return NULL;
	}

	len = strlen(dir_path) + strlen(file) + 5;
	path = wmalloc(len);
	snprintf(path, len, "%s%s.xpm", dir_path, file);
	wfree(dir_path);
	wfree(file);

	/* If icon exists, exit */
	if (access(path, F_OK) == 0)
		return path;

	if (wwin->net_icon_image)
		image = RRetainImage(wwin->net_icon_image);
	else
		image = get_wwindow_image_from_wmhints(wwin, icon);

	if (!image) {
		wfree(path);
		return NULL;
	}

	if (!RSaveImage(image, path, "XPM")) {
		wfree(path);
		path = NULL;
	}

	RReleaseImage(image);

	return path;
}

static void cycleColor(void *data)
{
	WIcon *icon = (WIcon *) data;
	WScreen *scr = icon->core->screen_ptr;
	XGCValues gcv;

	icon->step--;
	gcv.dash_offset = icon->step;
	XChangeGC(dpy, scr->icon_select_gc, GCDashOffset, &gcv);

	XDrawRectangle(dpy, icon->core->window, scr->icon_select_gc, 0, 0,
		       icon->core->width - 1, icon->core->height - 1);
	icon->handlerID = WMAddTimerHandler(COLOR_CYCLE_DELAY, cycleColor, icon);
}

#ifdef NEWAPPICON
void wIconSetHighlited(WIcon *icon, Bool flag)
{
	if (icon->highlighted == flag)
		return;

	icon->highlighted = flag;
	icon->force_paint = True;
	wIconPaint(icon);
}
#endif

void wIconSelect(WIcon * icon)
{
	WScreen *scr = icon->core->screen_ptr;
	icon->selected = !icon->selected;

	if (icon->selected) {
		icon->step = 0;
		if (!wPreferences.dont_blink)
			icon->handlerID = WMAddTimerHandler(10, cycleColor, icon);
		else
			XDrawRectangle(dpy, icon->core->window, scr->icon_select_gc, 0, 0,
				       icon->core->width - 1, icon->core->height - 1);
	} else {
		if (icon->handlerID) {
			WMDeleteTimerHandler(icon->handlerID);
			icon->handlerID = NULL;
		}
		XClearArea(dpy, icon->core->window, 0, 0, icon->core->width, icon->core->height, True);
	}
}

void wIconUpdate(WIcon *icon)
{
	WScreen *scr = icon->core->screen_ptr;
	WWindow *wwin = icon->owner;

	assert(scr->icon_tile != NULL);

	if (icon->pixmap != None)
		XFreePixmap(dpy, icon->pixmap);

	icon->pixmap = None;

	if (wwin && WFLAGP(wwin, always_user_icon)) {
		/* Forced use user_icon */
		get_pixmap_icon_from_user_icon(icon);
	} else if (icon->icon_win != None) {
		/* Get the Pixmap from the WIcon */
		get_pixmap_icon_from_icon_win(icon);
	} else if (wwin && wwin->net_icon_image) {
		/* Use _NET_WM_ICON icon */
		icon_update_pixmap(icon, wwin->net_icon_image);
	} else if (wwin && wwin->wm_hints && (wwin->wm_hints->flags & IconPixmapHint)) {
		/* Get the Pixmap from the wm_hints, else, from the user */
		if (get_pixmap_icon_from_wm_hints(icon))
			get_pixmap_icon_from_user_icon(icon);
	} else {
		/* Get the Pixmap from the user */
		get_pixmap_icon_from_user_icon(icon);
	}

	/* No pixmap, set default background */
	if (icon->pixmap != None)
		XSetWindowBackgroundPixmap(dpy, icon->core->window, icon->pixmap);

	/* Paint it */
	XClearWindow(dpy, icon->core->window);
	wIconPaint(icon);
}

static void get_pixmap_icon_from_user_icon(WIcon *icon)
{
	/* If the icon has image, update it and continue */
	if (icon->file_image) {
		icon_update_pixmap(icon, icon->file_image);
		return;
	}

	get_pixmap_icon_from_default_icon(icon);
}

static void get_pixmap_icon_from_default_icon(WIcon *icon)
{
	WScreen *scr = icon->core->screen_ptr;

	/* If the icon don't have image, we should use the default image. */
	if (!scr->def_icon_rimage)
		scr->def_icon_rimage = get_default_image(scr);

	/* Now, create the pixmap using the default (saved) image */
	icon_update_pixmap(icon, scr->def_icon_rimage);
}

/* This function creates the RImage using the default icon */
static RImage *get_default_image(WScreen *scr)
{
	RImage *image = NULL;
	char *path, *file;

	/* Get the default icon */
	file = wDefaultGetIconFile(NULL, NULL, True);
	if (file) {
		path = FindImage(wPreferences.icon_path, file);
		image = get_rimage_from_file(scr, path, wPreferences.icon_size);

		if (!image)
			wwarning(_("could not find default icon \"%s\""), file);

		wfree(file);
	}

	return image;
}

/* Get the Pixmap from the WIcon of the WWindow */
static void get_pixmap_icon_from_icon_win(WIcon * icon)
{
	XWindowAttributes attr;
	WScreen *scr = icon->core->screen_ptr;
	int title_height = WMFontHeight(scr->icon_title_font);
	unsigned int width, height, depth;
	int theight;
	int resize = 0;
	Pixmap pixmap;

	getSize(icon->icon_win, &width, &height, &depth);

	if (width > wPreferences.icon_size) {
		resize = 1;
		width = wPreferences.icon_size;
	}

	if (height > wPreferences.icon_size) {
		resize = 1;
		height = wPreferences.icon_size;
	}

	if (icon->show_title && (height + title_height < wPreferences.icon_size)) {
		pixmap = XCreatePixmap(dpy, scr->w_win, wPreferences.icon_size,
				       wPreferences.icon_size, scr->w_depth);
		XSetClipMask(dpy, scr->copy_gc, None);
		XCopyArea(dpy, scr->icon_tile_pixmap, pixmap, scr->copy_gc, 0, 0,
			  wPreferences.icon_size, wPreferences.icon_size, 0, 0);
		drawIconTitle(scr, pixmap, title_height);
		theight = title_height;
	} else {
		pixmap = None;
		theight = 0;
		XSetWindowBackgroundPixmap(dpy, icon->core->window, scr->icon_tile_pixmap);
	}

	XSetWindowBorderWidth(dpy, icon->icon_win, 0);
	XReparentWindow(dpy, icon->icon_win, icon->core->window,
			(wPreferences.icon_size - width) / 2,
			theight + (wPreferences.icon_size - height - theight) / 2);
	if (resize)
		XResizeWindow(dpy, icon->icon_win, width, height);

	XMapWindow(dpy, icon->icon_win);
	XAddToSaveSet(dpy, icon->icon_win);

	/* Save it */
	icon->pixmap = pixmap;

	if ((XGetWindowAttributes(dpy, icon->icon_win, &attr)) &&
	    (attr.all_event_masks & ButtonPressMask))
			wHackedGrabButton(Button1, MOD_MASK, icon->core->window, True,
					  ButtonPressMask, GrabModeSync, GrabModeAsync,
					  None, wCursor[WCUR_ARROW]);
}

/* Get the Pixmap from the XWindow wm_hints */
static int get_pixmap_icon_from_wm_hints(WIcon *icon)
{
	Window jw;
	Pixmap pixmap;
	unsigned int w, h, ju, d;
	int ji, x, y;
	WWindow *wwin = icon->owner;
	WScreen *scr = icon->core->screen_ptr;
	int title_height = WMFontHeight(scr->icon_title_font);

	if (!XGetGeometry(dpy, wwin->wm_hints->icon_pixmap, &jw, &ji, &ji, &w, &h, &ju, &d)) {
		icon->owner->wm_hints->flags &= ~IconPixmapHint;
		return 1;
	}

	pixmap = XCreatePixmap(dpy, icon->core->window, wPreferences.icon_size,
			       wPreferences.icon_size, scr->w_depth);
	XSetClipMask(dpy, scr->copy_gc, None);
	XCopyArea(dpy, scr->icon_tile_pixmap, pixmap, scr->copy_gc, 0, 0,
		  wPreferences.icon_size, wPreferences.icon_size, 0, 0);

	if (w > wPreferences.icon_size)
		w = wPreferences.icon_size;
	x = (wPreferences.icon_size - w) / 2;

	if (icon->show_title && (title_height < wPreferences.icon_size)) {
		drawIconTitle(scr, pixmap, title_height);

		if (h > wPreferences.icon_size - title_height - 2) {
			h = wPreferences.icon_size - title_height - 2;
			y = title_height + 1;
		} else {
			y = (wPreferences.icon_size - h - title_height) / 2 + title_height + 1;
		}
	} else {
		if (w > wPreferences.icon_size)
			w = wPreferences.icon_size;
		y = (wPreferences.icon_size - h) / 2;
	}

	if (wwin->wm_hints->flags & IconMaskHint)
		XSetClipMask(dpy, scr->copy_gc, wwin->wm_hints->icon_mask);

	XSetClipOrigin(dpy, scr->copy_gc, x, y);

	if (d != scr->w_depth) {
		XSetForeground(dpy, scr->copy_gc, scr->black_pixel);
		XSetBackground(dpy, scr->copy_gc, scr->white_pixel);
		XCopyPlane(dpy, wwin->wm_hints->icon_pixmap, pixmap, scr->copy_gc, 0, 0, w, h, x, y, 1);
	} else {
		XCopyArea(dpy, wwin->wm_hints->icon_pixmap, pixmap, scr->copy_gc, 0, 0, w, h, x, y);
	}

	XSetClipOrigin(dpy, scr->copy_gc, 0, 0);

	icon->pixmap = pixmap;
	return (0);
}

void wIconPaint(WIcon * icon)
{
	WScreen *scr = icon->core->screen_ptr;
	int x;
	char *tmp;

	if (icon->force_paint) {
		icon->force_paint = 0;
		wIconUpdate(icon);
		return;
	}

	XClearWindow(dpy, icon->core->window);

	/* draw the icon title */
	if (icon->show_title && icon->icon_name != NULL) {
		int l;
		int w;

		tmp = ShrinkString(scr->icon_title_font, icon->icon_name, wPreferences.icon_size - 4);
		w = WMWidthOfString(scr->icon_title_font, tmp, l = strlen(tmp));

		if (w > icon->core->width - 4)
			x = (icon->core->width - 4) - w;
		else
			x = (icon->core->width - w) / 2;

		WMDrawString(scr->wmscreen, icon->core->window, scr->icon_title_color,
			     scr->icon_title_font, x, 1, tmp, l);
		wfree(tmp);
	}

	if (icon->selected)
		XDrawRectangle(dpy, icon->core->window, scr->icon_select_gc, 0, 0,
			       icon->core->width - 1, icon->core->height - 1);
}

/******************************************************************/

static void miniwindowExpose(WObjDescriptor * desc, XEvent * event)
{
	wIconPaint(desc->parent);
}

static void miniwindowDblClick(WObjDescriptor * desc, XEvent * event)
{
	WIcon *icon = desc->parent;

	assert(icon->owner != NULL);

	wDeiconifyWindow(icon->owner);
}

static void miniwindowMouseDown(WObjDescriptor * desc, XEvent * event)
{
	WIcon *icon = desc->parent;
	WWindow *wwin = icon->owner;
	XEvent ev;
	int x = wwin->icon_x, y = wwin->icon_y;
	int dx = event->xbutton.x, dy = event->xbutton.y;
	int grabbed = 0;
	int clickButton = event->xbutton.button;
	Bool hasMoved = False;

	if (WCHECK_STATE(WSTATE_MODAL))
		return;

	if (IsDoubleClick(icon->core->screen_ptr, event)) {
		miniwindowDblClick(desc, event);
		return;
	}

	if (event->xbutton.button == Button1) {
		if (event->xbutton.state & MOD_MASK)
			wLowerFrame(icon->core);
		else
			wRaiseFrame(icon->core);
		if (event->xbutton.state & ShiftMask) {
			wIconSelect(icon);
			wSelectWindow(icon->owner, !wwin->flags.selected);
		}
	} else if (event->xbutton.button == Button3) {
		WObjDescriptor *desc;

		OpenMiniwindowMenu(wwin, event->xbutton.x_root, event->xbutton.y_root);

		/* allow drag select of menu */
		desc = &wwin->screen_ptr->window_menu->menu->descriptor;
		event->xbutton.send_event = True;
		(*desc->handle_mousedown) (desc, event);

		return;
	}

	if (XGrabPointer(dpy, icon->core->window, False, ButtonMotionMask
			 | ButtonReleaseMask | ButtonPressMask, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
	}
	while (1) {
		WMMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask | ButtonPressMask
			    | ButtonMotionMask | ExposureMask, &ev);
		switch (ev.type) {
		case Expose:
			WMHandleEvent(&ev);
			break;

		case MotionNotify:
			hasMoved = True;
			if (!grabbed) {
				if (abs(dx - ev.xmotion.x) >= MOVE_THRESHOLD
				    || abs(dy - ev.xmotion.y) >= MOVE_THRESHOLD) {
					XChangeActivePointerGrab(dpy, ButtonMotionMask
								 | ButtonReleaseMask | ButtonPressMask,
								 wCursor[WCUR_MOVE], CurrentTime);
					grabbed = 1;
				} else {
					break;
				}
			}
			x = ev.xmotion.x_root - dx;
			y = ev.xmotion.y_root - dy;
			XMoveWindow(dpy, icon->core->window, x, y);
			break;

		case ButtonPress:
			break;

		case ButtonRelease:
			if (ev.xbutton.button != clickButton)
				break;

			if (wwin->icon_x != x || wwin->icon_y != y)
				wwin->flags.icon_moved = 1;

			XMoveWindow(dpy, icon->core->window, x, y);

			wwin->icon_x = x;
			wwin->icon_y = y;
			XUngrabPointer(dpy, CurrentTime);

			if (wPreferences.auto_arrange_icons)
				wArrangeIcons(wwin->screen_ptr, True);
			if (wPreferences.single_click && !hasMoved)
				miniwindowDblClick(desc, event);
			return;

		}
	}
}
