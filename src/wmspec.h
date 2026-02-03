/*  wmspec.h-- support for the wm-spec Hints
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 1998-2003 Alfredo K. Kojima
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



#ifndef _WMSPEC_H_
#define _WMSPEC_H_

#include "screen.h"
#include "window.h"
#include <X11/Xlib.h>

/*
 * These constant provide information on the kind of window move/resize when
 * it is initiated by the application instead of by WindowMaker. They are
 * parameter for the client message _NET_WM_MOVERESIZE, as defined by the
 * FreeDesktop wm-spec standard:
 *   http://standards.freedesktop.org/wm-spec/1.5/ar01s04.html
 */
#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8	/* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9	/* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10	/* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL           11	/* cancel operation */

void wNETWMInitStuff(WScreen *scr);
void wNETWMCleanup(WScreen *scr);
void wNETWMUpdateWorkarea(WScreen *scr);
Bool wNETWMGetUsableArea(WScreen *scr, int head, WArea *area);
void wNETWMCheckInitialClientState(WWindow *wwin);
void wNETWMCheckInitialFrameState(WWindow *wwin);
Bool wNETWMProcessClientMessage(XClientMessageEvent *event);
void wNETWMCheckClientHints(WWindow *wwin, int *layer, int *workspace);
void wNETWMCheckClientHintChange(WWindow *wwin, XPropertyEvent *event);
void wNETWMUpdateActions(WWindow *wwin, Bool del);
void wNETWMUpdateDesktop(WScreen *scr);
void wNETWMPositionSplash(WWindow *wwin, int *x, int *y, int width, int height);
int wNETWMGetPidForWindow(Window window);
int wNETWMGetCurrentDesktopFromHint(WScreen *scr);
char *wNETWMGetIconName(Window window);
char *wNETWMGetWindowName(Window window);
void wNETFrameExtents(WWindow *wwin);
void wNETCleanupFrameExtents(WWindow *wwin);
RImage *get_window_image_from_x11(Window window);
#endif
