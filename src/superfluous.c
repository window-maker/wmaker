/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
 *  Copyright (c) 1998-2003 Dan Pascu
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

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <wraster.h>

#include "WindowMaker.h"
#include "screen.h"
#include "superfluous.h"
#include "dock.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "actions.h"

extern WPreferences wPreferences;

#ifdef DEMATERIALIZE_ICON
void DoKaboom(WScreen * scr, Window win, int x, int y)
{
	RImage *icon;
	RImage *back;
	RImage *image;
	Pixmap pixmap;
	XImage *ximage;
	GC gc;
	XGCValues gcv;
	int i;
	int w, h;
	int run;
	XEvent event;

	h = w = wPreferences.icon_size;
	if (x < 0 || x + w > scr->scr_width || y < 0 || y + h > scr->scr_height)
		return;

	icon = RCreateImageFromDrawable(scr->rcontext, win, None);
	if (!icon)
		return;

	gcv.foreground = scr->white_pixel;
	gcv.background = scr->black_pixel;
	gcv.graphics_exposures = False;
	gcv.subwindow_mode = IncludeInferiors;
	gc = XCreateGC(dpy, scr->w_win, GCForeground | GCBackground | GCSubwindowMode | GCGraphicsExposures, &gcv);

	XGrabServer(dpy);
	RConvertImage(scr->rcontext, icon, &pixmap);
	XUnmapWindow(dpy, win);

	ximage = XGetImage(dpy, scr->root_win, x, y, w, h, AllPlanes, ZPixmap);
	XCopyArea(dpy, pixmap, scr->root_win, gc, 0, 0, w, h, x, y);
	XFreePixmap(dpy, pixmap);

	back = RCreateImageFromXImage(scr->rcontext, ximage, NULL);
	XDestroyImage(ximage);
	if (!back) {
		RReleaseImage(icon);
		return;
	}

	for (i = 0, run = 0; i < DEMATERIALIZE_STEPS; i++) {
		XEvent foo;
		if (!run && XCheckTypedEvent(dpy, ButtonPress, &foo)) {
			run = 1;
			XPutBackEvent(dpy, &foo);
		}
		image = RCloneImage(back);
		RCombineImagesWithOpaqueness(image, icon,
					     (DEMATERIALIZE_STEPS - 1 - i) * 256 / (DEMATERIALIZE_STEPS + 2));
		RConvertImage(scr->rcontext, image, &pixmap);
		XCopyArea(dpy, pixmap, scr->root_win, gc, 0, 0, w, h, x, y);
		XFreePixmap(dpy, pixmap);
		XFlush(dpy);
		if (!run)
			wusleep(1000);
	}

	while (XCheckTypedEvent(dpy, MotionNotify, &event)) {
	}
	XFlush(dpy);

	XUngrabServer(dpy);
	XFreeGC(dpy, gc);
	RReleaseImage(icon);
	RReleaseImage(back);
}
#endif				/* DEMATERIALIZE_ICON */

#ifdef NORMAL_ICON_KABOOM
void DoKaboom(WScreen * scr, Window win, int x, int y)
{
	int i, j, k;
	int sw = scr->scr_width, sh = scr->scr_height;
#define KAB_PRECISION	4
	int px[PIECES];
	short py[PIECES];
#ifdef ICON_KABOOM_EXTRA
	short ptx[2][PIECES], pty[2][PIECES];
	int ll;
#endif
	char pvx[PIECES], pvy[PIECES];
	/* in MkLinux/PPC gcc seems to think that char is unsigned? */
	signed char ax[PIECES], ay[PIECES];
	Pixmap tmp;

	XSetClipMask(dpy, scr->copy_gc, None);
	tmp = XCreatePixmap(dpy, scr->root_win, wPreferences.icon_size, wPreferences.icon_size, scr->depth);
	if (scr->w_visual == DefaultVisual(dpy, scr->screen))
		XCopyArea(dpy, win, tmp, scr->copy_gc, 0, 0, wPreferences.icon_size, wPreferences.icon_size, 0, 0);
	else {
		XImage *image;

		image = XGetImage(dpy, win, 0, 0, wPreferences.icon_size,
				  wPreferences.icon_size, AllPlanes, ZPixmap);
		if (!image) {
			XUnmapWindow(dpy, win);
			return;
		}
		XPutImage(dpy, tmp, scr->copy_gc, image, 0, 0, 0, 0,
			  wPreferences.icon_size, wPreferences.icon_size);
		XDestroyImage(image);
	}

	for (i = 0, k = 0; i < wPreferences.icon_size / ICON_KABOOM_PIECE_SIZE && k < PIECES; i++) {
		for (j = 0; j < wPreferences.icon_size / ICON_KABOOM_PIECE_SIZE && k < PIECES; j++) {
			if (rand() % 2) {
				ax[k] = i;
				ay[k] = j;
				px[k] = (x + i * ICON_KABOOM_PIECE_SIZE) << KAB_PRECISION;
				py[k] = y + j * ICON_KABOOM_PIECE_SIZE;
				pvx[k] = rand() % (1 << (KAB_PRECISION + 3)) - (1 << (KAB_PRECISION + 3)) / 2;
				pvy[k] = -15 - rand() % 7;
#ifdef ICON_KABOOM_EXTRA
				for (ll = 0; ll < 2; ll++) {
					ptx[ll][k] = px[k];
					pty[ll][k] = py[k];
				}
#endif
				k++;
			} else {
				ax[k] = -1;
			}
		}
	}

	XUnmapWindow(dpy, win);

	j = k;
	while (k > 0) {
		XEvent foo;

		if (XCheckTypedEvent(dpy, ButtonPress, &foo)) {
			XPutBackEvent(dpy, &foo);
			XClearWindow(dpy, scr->root_win);
			break;
		}

		for (i = 0; i < j; i++) {
			if (ax[i] >= 0) {
				int _px = px[i] >> KAB_PRECISION;
#ifdef ICON_KABOOM_EXTRA
				XClearArea(dpy, scr->root_win, ptx[1][i], pty[1][i],
					   ICON_KABOOM_PIECE_SIZE, ICON_KABOOM_PIECE_SIZE, False);

				ptx[1][i] = ptx[0][i];
				pty[1][i] = pty[0][i];
				ptx[0][i] = _px;
				pty[0][i] = py[i];
#else
				XClearArea(dpy, scr->root_win, _px, py[i],
					   ICON_KABOOM_PIECE_SIZE, ICON_KABOOM_PIECE_SIZE, False);
#endif
				px[i] += pvx[i];
				py[i] += pvy[i];
				_px = px[i] >> KAB_PRECISION;
				pvy[i]++;
				if (_px < -wPreferences.icon_size || _px > sw || py[i] >= sh) {
#ifdef ICON_KABOOM_EXTRA
					if (py[i] > sh && _px < sw && _px > 0) {
						pvy[i] = -(pvy[i] / 2);
						if (abs(pvy[i]) > 3) {
							py[i] = sh - ICON_KABOOM_PIECE_SIZE;
							XCopyArea(dpy, tmp, scr->root_win, scr->copy_gc,
								  ax[i] * ICON_KABOOM_PIECE_SIZE,
								  ay[i] * ICON_KABOOM_PIECE_SIZE,
								  ICON_KABOOM_PIECE_SIZE,
								  ICON_KABOOM_PIECE_SIZE, _px, py[i]);
						} else {
							ax[i] = -1;
						}
					} else {
						ax[i] = -1;
					}
					if (ax[i] < 0) {
						for (ll = 0; ll < 2; ll++)
							XClearArea(dpy, scr->root_win, ptx[ll][i], pty[ll][i],
								   ICON_KABOOM_PIECE_SIZE,
								   ICON_KABOOM_PIECE_SIZE, False);
						k--;
					}
#else				/* !ICON_KABOOM_EXTRA */
					ax[i] = -1;
					k--;
#endif				/* !ICON_KABOOM_EXTRA */
				} else {
					XCopyArea(dpy, tmp, scr->root_win, scr->copy_gc,
						  ax[i] * ICON_KABOOM_PIECE_SIZE, ay[i] * ICON_KABOOM_PIECE_SIZE,
						  ICON_KABOOM_PIECE_SIZE, ICON_KABOOM_PIECE_SIZE, _px, py[i]);
				}
			}
		}

		XFlush(dpy);
		wusleep(MINIATURIZE_ANIMATION_DELAY_Z * 2);
	}

	XFreePixmap(dpy, tmp);
}
#endif				/* NORMAL_ICON_KABOOM */

Pixmap MakeGhostDock(WDock * dock, int sx, int dx, int y)
{
	WScreen *scr = dock->screen_ptr;
	XImage *img;
	RImage *back, *dock_image;
	Pixmap pixmap;
	int i, virtual_tiles, h, j, n;
	unsigned long red_mask, green_mask, blue_mask;

	virtual_tiles = 0;
	for (i = 0; i < dock->max_icons; i++) {
		if (dock->icon_array[i] != NULL && dock->icon_array[i]->yindex > virtual_tiles)
			virtual_tiles = dock->icon_array[i]->yindex;
	}
	virtual_tiles++;
	h = virtual_tiles * wPreferences.icon_size;
	h = (y + h > scr->scr_height) ? scr->scr_height - y : h;
	virtual_tiles = h / wPreferences.icon_size;	/* The visible ones */
	if (h % wPreferences.icon_size)
		virtual_tiles++;	/* There is one partially visible tile at end */

	img = XGetImage(dpy, scr->root_win, dx, y, wPreferences.icon_size, h, AllPlanes, ZPixmap);
	if (!img)
		return None;

	red_mask = img->red_mask;
	green_mask = img->green_mask;
	blue_mask = img->blue_mask;

	back = RCreateImageFromXImage(scr->rcontext, img, NULL);
	XDestroyImage(img);
	if (!back) {
		return None;
	}

	for (i = 0; i < dock->max_icons; i++) {
		if (dock->icon_array[i] != NULL && dock->icon_array[i]->yindex < virtual_tiles) {
			Pixmap which;
			j = dock->icon_array[i]->yindex * wPreferences.icon_size;
			n = (h - j < wPreferences.icon_size) ? h - j : wPreferences.icon_size;
			if (dock->icon_array[i]->icon->pixmap)
				which = dock->icon_array[i]->icon->pixmap;
			else
				which = dock->icon_array[i]->icon->core->window;

			img = XGetImage(dpy, which, 0, 0, wPreferences.icon_size, n, AllPlanes, ZPixmap);

			if (!img) {
				RReleaseImage(back);
				return None;
			}
			img->red_mask = red_mask;
			img->green_mask = green_mask;
			img->blue_mask = blue_mask;

			dock_image = RCreateImageFromXImage(scr->rcontext, img, NULL);
			XDestroyImage(img);
			if (!dock_image) {
				RReleaseImage(back);
				return None;
			}
			RCombineAreaWithOpaqueness(back, dock_image, 0, 0,
						   wPreferences.icon_size, n, 0, j, 30 * 256 / 100);
			RReleaseImage(dock_image);
		}
	}

	RConvertImage(scr->rcontext, back, &pixmap);

	RReleaseImage(back);

	return pixmap;
}

Pixmap MakeGhostIcon(WScreen * scr, Drawable drawable)
{
	RImage *back;
	RColor color;
	Pixmap pixmap;

	if (!drawable)
		return None;

	back = RCreateImageFromDrawable(scr->rcontext, drawable, None);
	if (!back)
		return None;

	color.red = 0xff;
	color.green = 0xff;
	color.blue = 0xff;
	color.alpha = 200;

	RClearImage(back, &color);
	RConvertImage(scr->rcontext, back, &pixmap);

	RReleaseImage(back);

	return pixmap;
}

#ifdef WINDOW_BIRTH_ZOOM

void DoWindowBirth(WWindow *wwin)
{
	int center_x, center_y;
	int width = wwin->frame->core->width;
	int height = wwin->frame->core->height;
	int w = WMIN(width, 20);
	int h = WMIN(height, 20);
	WScreen *scr = wwin->screen_ptr;

	center_x = wwin->frame_x + (width - w) / 2;
	center_y = wwin->frame_y + (height - h) / 2;

	animateResize(scr, center_x, center_y, 1, 1, wwin->frame_x, wwin->frame_y, width, height);
}
#else
void DoWindowBirth(WWindow *wwin)
{
	/* dummy stub */
}
#endif
