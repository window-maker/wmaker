/*
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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
#include <X11/keysym.h>
#include <stdio.h>
#include <unistd.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "client.h"
#include "icon.h"
#include "funcs.h"
#include "actions.h"
#include "workspace.h"

#include "list.h"

/* How many different types of geometry/position
 display thingies are there? */
#define NUM_DISPLAYS 4

#define LEFT            1
#define RIGHT           2
#define HORIZONTAL      (LEFT|RIGHT)
#define UP              4
#define DOWN            8
#define VERTICAL        (UP|DOWN)

/****** Global Variables ******/
extern Time LastTimestamp;

extern Cursor wCursor[WCUR_LAST];

extern WPreferences wPreferences;

extern Atom _XA_WM_PROTOCOLS;

/** Locals **/
static LinkedList *wSelectedWindows=NULL;



void
wGetGeometryWindowSize(WScreen *scr, unsigned int *width, 
		       unsigned int *height)
{
#ifdef I18N_MB
    *width = XmbTextEscapement(scr->info_text_font->font, "-8888 x -8888", 13);
    *height = (3 * scr->info_text_font->height) / 2;
#else
    *width = XTextWidth(scr->info_text_font->font, "-8888 x -8888", 13);
    *height = (3 * scr->info_text_font->font->ascent) / 2;
#endif
}


/*
 *----------------------------------------------------------------------
 * moveGeometryDisplayCentered
 *
 * routine that moves the geometry/position window on scr so it is
 * centered over the given coordinates (x,y). Also the window position
 * is clamped so it stays on the screen at all times.
 *----------------------------------------------------------------------
 */
static void
moveGeometryDisplayCentered(WScreen *scr, int x, int y)
{
    x -= scr->geometry_display_width / 2;
    y -= scr->geometry_display_height / 2;
    
    if (x < 1)
	x = 1;
    else if (x > (scr->scr_width - scr->geometry_display_width - 3))
	x = scr->scr_width - scr->geometry_display_width - 3;
    
    if (y < 1)
	y = 1;
    else if (y > (scr->scr_height - scr->geometry_display_height - 3))
	y = scr->scr_height - scr->geometry_display_height - 3;
    
    XMoveWindow(dpy, scr->geometry_display, x, y);
}


static void
showPosition(WWindow *wwin, int x, int y)
{
    WScreen *scr = wwin->screen_ptr;
    GC gc = scr->info_text_gc;
    char num[16];
    int fw, fh;

    if (wPreferences.move_display == WDIS_NEW) {
#if 0
        int width = wwin->frame->core->width;
        int height = wwin->frame->core->height;

	GC lgc = scr->line_gc;
	XSetForeground(dpy, lgc, scr->line_pixel);
	sprintf(num, "%i", x);

	XDrawLine(dpy, scr->root_win, lgc, 0, y-1, scr->scr_width, y-1);
	XDrawLine(dpy, scr->root_win, lgc, 0, y+height+2, scr->scr_width,
		  y+height+2);
	XDrawLine(dpy, scr->root_win, lgc, x-1, 0, x-1, scr->scr_height);
	XDrawLine(dpy, scr->root_win, lgc, x+width+2, 0, x+width+2,
		  scr->scr_height);
#endif
    } else {
	XClearWindow(dpy, scr->geometry_display);
	sprintf(num, "%+i  %-+i", x, y);
	fw = wTextWidth(scr->info_text_font->font, num, strlen(num));
	
	XSetForeground(dpy, gc, scr->window_title_pixel[WS_UNFOCUSED]);
	
	fh = scr->info_text_font->height;
	wDrawString(scr->geometry_display, scr->info_text_font, gc, 
		    (scr->geometry_display_width - 2 - fw) / 2,
		    (scr->geometry_display_height-fh)/2 + scr->info_text_font->y,
		    num, strlen(num));
    }
}


static void
cyclePositionDisplay(WWindow *wwin, int x, int y, int w, int h)
{
    WScreen *scr = wwin->screen_ptr;
    
    wPreferences.move_display++;
    wPreferences.move_display %= NUM_DISPLAYS;
    
    if (wPreferences.move_display == WDIS_NEW) {
	XUnmapWindow(dpy, scr->geometry_display);
    } else {
	if (wPreferences.move_display == WDIS_CENTER) {
	    moveGeometryDisplayCentered(scr,
					scr->scr_width/2, scr->scr_height/2);
	} else if (wPreferences.move_display == WDIS_TOPLEFT) {
	    moveGeometryDisplayCentered(scr, 1, 1);
	} else if (wPreferences.move_display == WDIS_FRAME_CENTER) {
	    moveGeometryDisplayCentered(scr, x + w/2, y + h/2);
	}
	XMapRaised(dpy, scr->geometry_display);
	showPosition(wwin, x, y);
    }
}


static void
mapPositionDisplay(WWindow *wwin, int x, int y, int w, int h)
{
    WScreen *scr = wwin->screen_ptr;
    
    if (wPreferences.move_display == WDIS_NEW) {
	return;
    } else if (wPreferences.move_display == WDIS_CENTER) {
	moveGeometryDisplayCentered(scr, scr->scr_width / 2,
				    scr->scr_height / 2);
    } else if (wPreferences.move_display == WDIS_TOPLEFT) {
	moveGeometryDisplayCentered(scr, 1, 1);
    } else if (wPreferences.move_display == WDIS_FRAME_CENTER) {
	moveGeometryDisplayCentered(scr, x + w/2, y + h/2);
    }
    XMapRaised(dpy, scr->geometry_display);
    showPosition(wwin, x, y);
}

#define unmapPositionDisplay(w) \
XUnmapWindow(dpy, (w)->screen_ptr->geometry_display);


static void
showGeometry(WWindow *wwin, int x1, int y1, int x2, int y2, int direction)
{
    WScreen *scr = wwin->screen_ptr;
    Window root = scr->root_win;
    GC gc = scr->line_gc;
    int ty, by, my, x, y, mx, s;
    char num[16];
    XSegment segment[4];
    int fw, fh;
    
    ty = y1 + wwin->frame->top_width;
    by = y2 - wwin->frame->bottom_width;
    fw = wTextWidth(scr->info_text_font->font, "8888", 4);
    fh = scr->info_text_font->height;

    if (wPreferences.size_display == WDIS_NEW) {
	XSetForeground(dpy, gc, scr->line_pixel);
	
	/* vertical geometry */
	if (((direction & LEFT) && (x2 < scr->scr_width - fw)) || (x1 < fw)) {
	    x = x2;
	    s = -15;
	} else {
	    x = x1;
	    s = 15;
	}
	my = (ty + by) / 2;
	
	/* top arrow */
	/* end bar */
	segment[0].x1 = x - (s + 6);  segment[0].y1 = ty;
	segment[0].x2 = x - (s - 10); segment[0].y2 = ty;
	
	/* arrowhead */
	segment[1].x1 = x - (s - 2); segment[1].y1 = ty + 1;
	segment[1].x2 = x - (s - 5); segment[1].y2 = ty + 7;
	
	segment[2].x1 = x - (s - 2); segment[2].y1 = ty + 1;
	segment[2].x2 = x - (s + 1); segment[2].y2 = ty + 7;
	
	/* line */
	segment[3].x1 = x - (s - 2); segment[3].y1 = ty + 1;
	segment[3].x2 = x - (s - 2); segment[3].y2 = my - fh/2 - 1;
	
	XDrawSegments(dpy, root, gc, segment, 4);
	
	/* bottom arrow */
	/* end bar */
	segment[0].y1 = by;
	segment[0].y2 = by;
	
	/* arrowhead */
	segment[1].y1 = by - 1;
	segment[1].y2 = by - 7;
	
	segment[2].y1 = by - 1;
	segment[2].y2 = by - 7;
	
	/* line */
	segment[3].y1 = my + fh/2 + 2;
	segment[3].y2 = by - 1;
	
	XDrawSegments(dpy, root, gc, segment, 4);
	
	sprintf(num, "%i", (by - ty - wwin->normal_hints->base_height) /
		wwin->normal_hints->height_inc);
	fw = wTextWidth(scr->info_text_font->font, num, strlen(num));
	
	/* XSetForeground(dpy, gc, scr->window_title_pixel[WS_UNFOCUSED]); */
	
	/* Display the height. */
	wDrawString(root, scr->info_text_font, gc,
		      x - s + 3 - fw/2, my - fh/2 + scr->info_text_font->y + 1,
		      num, strlen(num));
	XSetForeground(dpy, gc, scr->line_pixel);
	/* horizontal geometry */
	if (y1 < 15) {
	    y = y2;
	    s = -15;
	} else {
	    y = y1;
	    s = 15;
	}
	mx = x1 + (x2 - x1)/2;
	sprintf(num, "%i", (x2 - x1 - wwin->normal_hints->base_width) /
		wwin->normal_hints->width_inc);
	fw = wTextWidth(scr->info_text_font->font, num, strlen(num));
	
	/* left arrow */
	/* end bar */
	segment[0].x1 = x1; segment[0].y1 = y - (s + 6);
	segment[0].x2 = x1; segment[0].y2 = y - (s - 10);
	
	/* arrowhead */
	segment[1].x1 = x1 + 7; segment[1].y1 = y - (s + 1);
	segment[1].x2 = x1 + 1; segment[1].y2 = y - (s - 2);
	
	segment[2].x1 = x1 + 1; segment[2].y1 = y - (s - 2);
	segment[2].x2 = x1 + 7; segment[2].y2 = y - (s - 5);
	
	/* line */
	segment[3].x1 = x1 + 1; segment[3].y1 = y - (s - 2);
	segment[3].x2 = mx - fw/2 - 2; segment[3].y2 = y - (s - 2);
	
	XDrawSegments(dpy, root, gc, segment, 4);
	
	/* right arrow */
	/* end bar */
	segment[0].x1 = x2 + 1;
	segment[0].x2 = x2 + 1;
	
	/* arrowhead */
	segment[1].x1 = x2 - 6;
	segment[1].x2 = x2;
	
	segment[2].x1 = x2;
	segment[2].x2 = x2 - 6;
	
	/* line */
	segment[3].x1 = mx + fw/2 + 2;
	segment[3].x2 = x2;
	
	XDrawSegments(dpy, root, gc, segment, 4);
	
	/* XSetForeground(dpy, gc, scr->window_title_pixel[WS_UNFOCUSED]); */
	
	/* Display the width. */
	wDrawString(root, scr->info_text_font, gc,
		    mx - fw/2 + 1, y - s + fh/2 + 1, num, strlen(num));	
    } else {
	XClearWindow(dpy, scr->geometry_display);
	sprintf(num, "%i x %-i", (x2 - x1 - wwin->normal_hints->base_width)
		/ wwin->normal_hints->width_inc,
		(by - ty - wwin->normal_hints->base_height)
		/ wwin->normal_hints->height_inc);
	fw = wTextWidth(scr->info_text_font->font, num, strlen(num));
	
	XSetForeground(dpy, scr->info_text_gc,
		       scr->window_title_pixel[WS_UNFOCUSED]);
	
	/* Display the height. */
	wDrawString(scr->geometry_display, scr->info_text_font,
		    scr->info_text_gc,
		    (scr->geometry_display_width-fw)/2,
		    (scr->geometry_display_height-fh)/2 +scr->info_text_font->y,
		    num, strlen(num));
    }
}


static void
cycleGeometryDisplay(WWindow *wwin, int x, int y, int w, int h, int dir)
{
    WScreen *scr = wwin->screen_ptr;
    
    wPreferences.size_display++;
    wPreferences.size_display %= NUM_DISPLAYS;
    
    if (wPreferences.size_display == WDIS_NEW) {
	XUnmapWindow(dpy, scr->geometry_display);
    } else {
	if (wPreferences.size_display == WDIS_CENTER) {
	    moveGeometryDisplayCentered(scr,
					scr->scr_width / 2, scr->scr_height / 2);
	} else if (wPreferences.size_display == WDIS_TOPLEFT) {
	    moveGeometryDisplayCentered(scr, 1, 1);
	} else if (wPreferences.size_display == WDIS_FRAME_CENTER) {
	    moveGeometryDisplayCentered(scr, x + w/2, y + h/2);
	}
	XMapRaised(dpy, scr->geometry_display);
	showGeometry(wwin, x, y, x + w, y + h, dir);
    }
}


static void
mapGeometryDisplay(WWindow *wwin, int x, int y, int w, int h)
{
    WScreen *scr = wwin->screen_ptr;
    
    if (wPreferences.size_display == WDIS_NEW)
	return;
    
    if (wPreferences.size_display == WDIS_CENTER) {
	moveGeometryDisplayCentered(scr, scr->scr_width / 2,
				    scr->scr_height / 2);
    } else if (wPreferences.size_display == WDIS_TOPLEFT) {
	moveGeometryDisplayCentered(scr, 1, 1);
    } else if (wPreferences.size_display == WDIS_FRAME_CENTER) {
	moveGeometryDisplayCentered(scr, x + w/2, y + h/2);
    }
    XMapRaised(dpy, scr->geometry_display);
    showGeometry(wwin, x, y, x + w, y + h, 0);
}

#define unmapGeometryDisplay(w) \
XUnmapWindow(dpy, (w)->screen_ptr->geometry_display);

static void
checkEdgeResistance(WWindow *wwin, int *winx, int *winy, int off_x, int off_y)
{
    int scr_width = wwin->screen_ptr->scr_width;
    int scr_height = wwin->screen_ptr->scr_height;
    int x = *winx;
    int y = *winy;
    int edge_resistance = wPreferences.edge_resistance;

    x -= off_x;
    y -= off_y;

    if ((x + wwin->frame->core->width) >= (scr_width - 2)) {
      if ((x + wwin->frame->core->width) < ((scr_width - 2) 
          + edge_resistance)) {
        x = scr_width - wwin->frame->core->width - 2;
      } else {
        x -= edge_resistance;
      }
    }

    if (x <= 0) {
      if (x > -edge_resistance) {
        x = 0;
      } else {
        x += edge_resistance;
      }
    }

    if ((y + wwin->frame->core->height) >= (scr_height - 1)) {
      if ((y + wwin->frame->core->height) < ((scr_height - 1)
          + edge_resistance)) {
        y = scr_height - wwin->frame->core->height - 1;
      } else {
        y -= edge_resistance;
      }
    }

    if (y <=0) {
      if (y > -edge_resistance) {
        y = 0;
      } else {
        y += edge_resistance;
      }
    }


    *winx = x;
    *winy = y;
}

static void
doWindowMove(WWindow *wwin, int single_win_x, int single_win_y,
             LinkedList *list, int dx, int dy, int off_x, int off_y)
{
    WWindow *tmpw;
    int x, y;
    int scr_width = wwin->screen_ptr->scr_width;
    int scr_height = wwin->screen_ptr->scr_height;

    if (!list) {
        checkEdgeResistance(wwin, &single_win_x, &single_win_y, off_x, off_y);
        wWindowMove(wwin, single_win_x, single_win_y);
    } else {
	while (list) {
	    tmpw = list->head;
	    x = tmpw->frame_x + dx;
	    y = tmpw->frame_y + dy;
	    
	    /* don't let windows become unreachable */
	    
	    if (x + (int)tmpw->frame->core->width < 20)
		x = 20 - (int)tmpw->frame->core->width;
	    else if (x + 20 > scr_width)
		x = scr_width - 20;

	    if (y + (int)tmpw->frame->core->height < 20)
		y = 20 - (int)tmpw->frame->core->height;
	    else if (y + 20 > scr_height)
		y = scr_height - 20;
	    
 	    wWindowMove(tmpw, x, y);
	    list = list->tail;
	}
    }
}


static void
drawTransparentFrame(WWindow *wwin, int x, int y, int width, int height)
{
    Window root = wwin->screen_ptr->root_win;
    GC gc = wwin->screen_ptr->frame_gc;
    int h = 0;
    int bottom = 0;
    
    if (!wwin->window_flags.no_titlebar && !wwin->flags.shaded) {
	h = wwin->screen_ptr->title_font->height + TITLEBAR_EXTRA_HEIGHT;
    }
    if (!wwin->window_flags.no_resizebar && !wwin->flags.shaded) {
	/* Can't use wwin-frame->bottom_width because, in some cases 
	 (e.g. interactive placement), frame does not point to anything. */
	bottom = RESIZEBAR_HEIGHT - 1;
    } 
    XDrawRectangle(dpy, root, gc, x, y, width + 1, height + 1);
    
    if (h > 0) {
	XDrawLine(dpy, root, gc, x + 1, y + h, x + width + 1, y + h); 
    }
    if (bottom > 0) {
	XDrawLine(dpy, root, gc, x + 1,
		  y + height - bottom,
		  x + width + 1,
		  y + height - bottom);
    }
}


static void
drawFrames(WWindow *wwin, LinkedList *list, int dx, int dy, int off_x, int off_y)
{
    WWindow *tmpw;
    int scr_width = wwin->screen_ptr->scr_width;
    int scr_height = wwin->screen_ptr->scr_height;
    int x, y;
    
    if (!list) {

        x = wwin->frame_x + dx;
        y = wwin->frame_y + dy;

        checkEdgeResistance(wwin, &x, &y, off_x, off_y);
	drawTransparentFrame(wwin, x, y,
			     wwin->frame->core->width, 
			     wwin->frame->core->height);
	
    } else {
	while (list) {
	    tmpw = list->head;
	    x = tmpw->frame_x + dx;
	    y = tmpw->frame_y + dy;
	    
	    /* don't let windows become unreachable */
	    
	    if (x + (int)tmpw->frame->core->width < 20)
		x = 20 - (int)tmpw->frame->core->width;
	    else if (x + 20 > scr_width)
		x = scr_width - 20;
	    
	    if (y + (int)tmpw->frame->core->height < 20)
		y = 20 - (int)tmpw->frame->core->height;
	    else if (y + 20 > scr_height)
		y = scr_height - 20;
	    
	    drawTransparentFrame(tmpw, x, y, tmpw->frame->core->width,
				 tmpw->frame->core->height);
	    
	    list = list->tail;
	}
    }
}



static void
flushMotion()
{
    XEvent ev;
    
    XSync(dpy, 0);
    while (XCheckMaskEvent(dpy, ButtonMotionMask, &ev)) ;
}


static void
crossWorkspace(WScreen *scr, WWindow *wwin, int opaque_move,
               int new_workspace, int rewind)
{
    /* do not let window be unmapped */
    if (opaque_move) {
	wwin->flags.changing_workspace = 1;
	wWindowChangeWorkspace(wwin, new_workspace);
    }
    /* go to new workspace */
    wWorkspaceChange(scr, new_workspace);
    
    wwin->flags.changing_workspace = 0;
    
    if (rewind)
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, scr->scr_width - 20, 0);
    else
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, -(scr->scr_width - 20), 0);
    
    flushMotion();
    
    if (!opaque_move) {
	XGrabPointer(dpy, scr->root_win, True, PointerMotionMask
		     |ButtonReleaseMask|ButtonPressMask, GrabModeAsync,
		     GrabModeAsync, None, wCursor[WCUR_MOVE], CurrentTime);
    }
}


/*
 *---------------------------------------------------------------------- 
 * wMouseMoveWindow--
 * 	Move the named window and the other selected ones (if any),
 * interactively. Also shows the position of the window, if only one
 * window is being moved.
 * 	If the window is not on the selected window list, the selected
 * windows are deselected.
 * 	If shift is pressed during the operation, the position display
 * is changed to another type.
 * 
 * Returns:
 * 	True if the window was moved, False otherwise.
 * 
 * Side effects:
 * 	The window(s) position is changed, and the client(s) are 
 * notified about that.
 * 	The position display configuration may be changed.
 *---------------------------------------------------------------------- 
 */
int
wMouseMoveWindow(WWindow *wwin, XEvent *ev)
{
    WScreen *scr = wwin->screen_ptr;
    XEvent event;
    Window root = scr->root_win;
    KeyCode shiftl, shiftr;
    int w = wwin->frame->core->width;
    int h = wwin->frame->core->height;
    int x = wwin->frame_x;
    int y = wwin->frame_y;
    int ox, oy, orig_x, orig_y;
    int off_x, off_y;
    short count = 0;		/* for automatic workspace creation */
    int started = 0;
    int warped = 0;
    /* This needs not to change while moving, else bad things can happen */
    int opaque_move = wPreferences.opaque_move;
    int XOffset, YOffset, origDragX, origDragY;

    origDragX = wwin->frame_x;
    origDragY = wwin->frame_y;
    XOffset = origDragX - ev->xbutton.x_root;
    YOffset = origDragY - ev->xbutton.y_root;
    
    if (!wwin->flags.selected) {
	/* this window is not selected, unselect others and move only wwin */
	wUnselectWindows();
    }
    orig_x = ox = ev->xbutton.x_root;
    orig_y = oy = ev->xbutton.y_root;
    off_x = x; off_y = y;
    checkEdgeResistance(wwin, &off_x, &off_y, 0, 0);
    off_x = (off_x-x); off_y = (off_y-y);
#ifdef DEBUG
    puts("Moving window");
#endif
    shiftl = XKeysymToKeycode(dpy, XK_Shift_L);
    shiftr = XKeysymToKeycode(dpy, XK_Shift_R);
    while (1) {
	if (warped) {
            int junk;
            Window junkw;
    
	    /* XWarpPointer() doesn't seem to generate Motion events, so
	     we've got to simulate them */
	    XQueryPointer(dpy, root, &junkw, &junkw, &event.xmotion.x_root,
			  &event.xmotion.y_root, &junk, &junk,
			  (unsigned *) &junk);
	} else {
	    Window win;
	    
	    WMMaskEvent(dpy, KeyPressMask | ButtonMotionMask
		| ButtonReleaseMask | ButtonPressMask | ExposureMask, &event);
	    
	    if (event.type == MotionNotify) {
		/* compress MotionNotify events */
		win = event.xmotion.window;
		while (XCheckMaskEvent(dpy, ButtonMotionMask, &event)) ;
	    }
	}
	switch (event.type) {
	 case KeyPress:
	    if (wSelectedWindows)
		break;
	    if ((event.xkey.keycode == shiftl || event.xkey.keycode == shiftr)
		&& started) {
		if (!opaque_move)
                    drawFrames(wwin, wSelectedWindows,
                               ox - orig_x, oy - orig_y, off_x, off_y);
		
		cyclePositionDisplay(wwin, x, y, w, h);
		
		if (!opaque_move) {
		    drawFrames(wwin, wSelectedWindows,
			       ox - orig_x, oy - orig_y, off_x, off_y);
		} 
		showPosition(wwin, x, y);
	    }
	    break;
	    
	 case MotionNotify:
	    if (started) {
		showPosition(wwin, x, y);

                if (!opaque_move) {
                    drawFrames(wwin, wSelectedWindows,
                               ox-orig_x, oy-orig_y, off_x, off_y);
                } else {
                    doWindowMove(wwin, event.xmotion.x_root + XOffset,
                                 event.xmotion.y_root + YOffset,
                                 wSelectedWindows,
                                 event.xmotion.x_root - ox,
                                 event.xmotion.y_root - oy,
                                 off_x, off_y);
                }

                x = event.xmotion.x_root + XOffset;
                y = event.xmotion.y_root + YOffset;

                checkEdgeResistance(wwin, &x, &y, off_x, off_y);

		if (!wSelectedWindows) {
		    if (wPreferences.move_display == WDIS_FRAME_CENTER)
			moveGeometryDisplayCentered(scr, x + w/2, y + h/2);
		}
		if (!warped && !wPreferences.no_autowrap) {
		    if (event.xmotion.x_root <= 1) {
			if (scr->current_workspace > 0) {
			    crossWorkspace(scr, wwin, opaque_move, 
					   scr->current_workspace-1, True);
			    warped = 1;
			    count = 0;
			} else if (scr->current_workspace == 0
				   && wPreferences.ws_cycle) {
			    crossWorkspace(scr, wwin, opaque_move, 
					   scr->workspace_count-1, True);
			    warped = 1;
			    count = 0;			    
			}
		    } else if (event.xmotion.x_root >= scr->scr_width - 2) {

			if (scr->current_workspace == scr->workspace_count-1) {
			    if ((!wPreferences.ws_advance && wPreferences.ws_cycle)
				|| (scr->workspace_count == MAX_WORKSPACES)) {
				crossWorkspace(scr, wwin, opaque_move, 0, False);
				warped = 1;
				count = 0;
			    }
			    /* if user insists on trying to go to next
			     workspace even when it's already the last,
			     create a new one */
			    else if ((ox == event.xmotion.x_root)
				     && wPreferences.ws_advance) {
				
				/* detect user "rubbing" the window
				 against the edge */
				if (count > 0
				    && oy - event.xmotion.y_root > MOVE_THRESHOLD)
				    count = -(count + 1);
				else if (count <= 0
					 && event.xmotion.y_root - oy > MOVE_THRESHOLD)
				    count = -count + 1;
			    }
			    /* create a new workspace */
			    if (abs(count) > 2) {
				/* go to next workspace */
				wWorkspaceNew(scr);
				
				crossWorkspace(scr, wwin, opaque_move, 
					       scr->current_workspace+1, False);
				warped = 1;
				count = 0;
			    }
			} else if (scr->current_workspace < scr->workspace_count) {
			    
			    /* go to next workspace */
			    crossWorkspace(scr, wwin, opaque_move, 
					   scr->current_workspace+1, False);
			    warped = 1;
			    count = 0;
			}
		    } else {
			count = 0;
		    }
		} else {
		    warped = 0;
		}
	    } else if (abs(orig_x - event.xmotion.x_root) >= MOVE_THRESHOLD
		       || abs(orig_y - event.xmotion.y_root) >= MOVE_THRESHOLD) {
		XChangeActivePointerGrab(dpy, ButtonMotionMask
					 | ButtonReleaseMask | ButtonPressMask,
					 wCursor[WCUR_MOVE], CurrentTime);
		started = 1;
		XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, 
			      CurrentTime);
		event.xmotion.x_root = orig_x;
		event.xmotion.y_root = orig_y;
		
		if (!wSelectedWindows)
		    mapPositionDisplay(wwin, x, y, w, h);
		
		if (!opaque_move)
		    XGrabServer(dpy);
	    }
	    ox = event.xmotion.x_root;
	    oy = event.xmotion.y_root;
	    
	    if (started && !opaque_move)
		drawFrames(wwin, wSelectedWindows, ox - orig_x, oy - orig_y, off_x, off_y);
	    
	    showPosition(wwin, x, y);
	    break;

	 case ButtonPress:
	    break;
	    
	 case ButtonRelease:
	    if (event.xbutton.button != ev->xbutton.button)
		break;

	    if (started) {
		if (!opaque_move) {
		    drawFrames(wwin, wSelectedWindows,
			       ox - orig_x, oy - orig_y, off_x, off_y);
		    XSync(dpy, 0);
		    doWindowMove(wwin, event.xmotion.x_root + XOffset,
                                 event.xmotion.y_root + YOffset,
                                 wSelectedWindows,
				 ox - orig_x, oy - orig_y, 
				 off_x, off_y);
		}
#ifndef CONFIGURE_WINDOW_WHILE_MOVING
		wWindowSynthConfigureNotify(wwin);
#endif
		
		XUngrabKeyboard(dpy, CurrentTime);
		XUngrabServer(dpy);
		if (!opaque_move) {
		    wWindowChangeWorkspace(wwin, scr->current_workspace);
		    wSetFocusTo(scr, wwin);
		}
		showPosition(wwin, x, y);
		if (!wSelectedWindows) {
		    /* get rid of the geometry window */
		    unmapPositionDisplay(wwin);
		}
	    }
#ifdef DEBUG
	    puts("End move window");
#endif
	    return started;
	    
	 default:
	    if (started && !opaque_move) {
		drawFrames(wwin, wSelectedWindows, ox - orig_x, oy - orig_y, off_x, off_y);
		XUngrabServer(dpy);
		WMHandleEvent(&event);
		XSync(dpy, False);
		XGrabServer(dpy);
		drawFrames(wwin, wSelectedWindows, ox - orig_x, oy - orig_y, off_x, off_y);
	    } else {
		WMHandleEvent(&event);
	    }
	    break;
	}
    }
    return 0;
}


#define RESIZEBAR	1
#define HCONSTRAIN	2

static int
getResizeDirection(WWindow *wwin, int x, int y, int dx, int dy, 
		   int flags)
{
    int w = wwin->frame->core->width - 1;
    int cw = wwin->frame->resizebar_corner_width;
    int dir;
    
    /* if not resizing through the resizebar */
    if (!(flags & RESIZEBAR)) {
	int xdir = (abs(x) < (wwin->client.width/2)) ? LEFT : RIGHT;
	int ydir = (abs(y) < (wwin->client.height/2)) ? UP : DOWN;
	if (abs(dx) < 2 || abs(dy) < 2) {
	    if (abs(dy) > abs(dx))
		xdir = 0;
	    else
		ydir = 0;
	}
	return (xdir | ydir);
    }
    
    /* window is too narrow. allow diagonal resize */
    if (cw * 2 >= w) {
	int ydir;

	if (flags & HCONSTRAIN)
	    ydir = 0;
	else
	    ydir = DOWN;
	if (x < cw)
	    return (LEFT | ydir);
	else
	    return (RIGHT | ydir);
    }
    /* vertical resize */
    if ((x > cw) && (x < w - cw))
	return DOWN;
    
    if (x < cw)
	dir = LEFT;
    else 
	dir = RIGHT;
    
    if ((abs(dy) > 0) && !(flags & HCONSTRAIN))
	dir |= DOWN;
    
    return dir;
}


void
wMouseResizeWindow(WWindow *wwin, XEvent *ev)
{
    XEvent event;
    WScreen *scr = wwin->screen_ptr;
    Window root = scr->root_win;
    int vert_border = wwin->frame->top_width + wwin->frame->bottom_width;
    int fw = wwin->frame->core->width;
    int fh = wwin->frame->core->height;
    int fx = wwin->frame_x;
    int fy = wwin->frame_y;
    int is_resizebar = (wwin->frame->resizebar
			&& ev->xany.window==wwin->frame->resizebar->window);
    int orig_x, orig_y;
    int started;
    int dw, dh;
    int rw = fw, rh = fh;
    int rx1, ry1, rx2, ry2;
    int res = 0;
    KeyCode shiftl, shiftr;
    int h = 0;
    int orig_fx = fx;
    int orig_fy = fy;
    int orig_fw = fw;
    int orig_fh = fh;
    
    if (wwin->flags.shaded) {
	wwarning("internal error: tryein");
	return;
    }
    orig_x = ev->xbutton.x_root;
    orig_y = ev->xbutton.y_root;
    
    started = 0;
#ifdef DEBUG
    puts("Resizing window");
#endif
    
    wUnselectWindows();
    rx1 = fx;
    rx2 = fx + fw - 1;
    ry1 = fy;
    ry2 = fy + fh - 1;
    shiftl = XKeysymToKeycode(dpy, XK_Shift_L);
    shiftr = XKeysymToKeycode(dpy, XK_Shift_R);
    if (!wwin->window_flags.no_titlebar)
    	h = wwin->screen_ptr->title_font->height + TITLEBAR_EXTRA_HEIGHT;
    else
	h = 0;
    while (1) {
	WMMaskEvent(dpy, KeyPressMask | ButtonMotionMask | ButtonReleaseMask
		   | ButtonPressMask | ExposureMask, &event);
	switch (event.type) {
	 case KeyPress:
	    showGeometry(wwin, fx, fy, fx + fw, fy + fh, res);
	    if ((event.xkey.keycode == shiftl || event.xkey.keycode == shiftr)
		&& started) {
		drawTransparentFrame(wwin, fx, fy, fw, fh);
		cycleGeometryDisplay(wwin, fx, fy, fw, fh, res);
		drawTransparentFrame(wwin, fx, fy, fw, fh);
	    }
	    showGeometry(wwin, fx, fy, fx + fw, fy + fh, res);
	    break;
	    
	 case MotionNotify:
	    if (started) {
		dw = 0;
		dh = 0;
		
		orig_fx = fx;
		orig_fy = fy;
		orig_fw = fw;
		orig_fh = fh;
		
		if (res & LEFT)
		    dw = orig_x - event.xmotion.x_root;
		else if (res & RIGHT)
		    dw = event.xmotion.x_root - orig_x;
		if (res & UP)
		    dh = orig_y - event.xmotion.y_root;
		else if (res & DOWN)
		    dh = event.xmotion.y_root - orig_y;
		
		orig_x = event.xmotion.x_root;
		orig_y = event.xmotion.y_root;
		
		rw += dw;
		rh += dh;
		fw = rw;
		fh = rh - vert_border;
		wWindowConstrainSize(wwin, &fw, &fh);
		fh += vert_border;
		if (res & LEFT)
		    fx = rx2 - fw + 1;
		else if (res & RIGHT)
		    fx = rx1;
		if (res & UP)
		    fy = ry2 - fh + 1;
		else if (res & DOWN)
		    fy = ry1;
	    } else if (abs(orig_x - event.xmotion.x_root) >= MOVE_THRESHOLD
		       || abs(orig_y - event.xmotion.y_root) >= MOVE_THRESHOLD) {
		int tx, ty;
		Window junkw;
		int flags;
		
		XTranslateCoordinates(dpy, root, wwin->frame->core->window,
				      orig_x, orig_y, &tx, &ty, &junkw);
		
		/* check if resizing through resizebar */
		if (is_resizebar)
		    flags = RESIZEBAR;
		else
		    flags = 0;

		if (is_resizebar && ((ev->xbutton.state & ShiftMask)
		    || abs(orig_y - event.xmotion.y_root) < HRESIZE_THRESHOLD))
		    flags |= HCONSTRAIN;

		res = getResizeDirection(wwin, tx, ty,
					 orig_x - event.xmotion.x_root,
					 orig_y - event.xmotion.y_root, flags);

		XChangeActivePointerGrab(dpy, ButtonMotionMask
					 | ButtonReleaseMask | ButtonPressMask,
					 wCursor[WCUR_RESIZE], CurrentTime);
		XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, 
			      CurrentTime);

		XGrabServer(dpy);

		/* Draw the resize frame for the first time. */
		mapGeometryDisplay(wwin, fx, fy, fw, fh);
		
		drawTransparentFrame(wwin, fx, fy, fw, fh);
		
		showGeometry(wwin, fx, fy, fx + fw, fy + fh, res);
		
		started = 1;
	    }
	    if (started) {
		if (wPreferences.size_display == WDIS_FRAME_CENTER) {
		    drawTransparentFrame(wwin, orig_fx, orig_fy,
					 orig_fw, orig_fh);
		    moveGeometryDisplayCentered(scr, fx + fw / 2, fy + fh / 2);
		    drawTransparentFrame(wwin, fx, fy, fw, fh);
		} else {
		    drawTransparentFrame(wwin, orig_fx, orig_fy,
					 orig_fw, orig_fh);
		    drawTransparentFrame(wwin, fx, fy, fw, fh);
		}
		if (fh != orig_fh || fw != orig_fw) {
		    if (wPreferences.size_display == WDIS_NEW) {
			showGeometry(wwin, orig_fx, orig_fy, orig_fx + orig_fw, 
				     orig_fy + orig_fh, res);
		    }
		    showGeometry(wwin, fx, fy, fx + fw, fy + fh, res);
		}
	    }
	    break;

	 case ButtonPress:
	    break;

 	 case ButtonRelease:
	    if (event.xbutton.button != ev->xbutton.button)
		break;

	    if (started) {
		showGeometry(wwin, fx, fy, fx + fw, fy + fh, res);
		
		drawTransparentFrame(wwin, fx, fy, fw, fh);
		
		XUngrabKeyboard(dpy, CurrentTime);
		unmapGeometryDisplay(wwin);
		XUngrabServer(dpy);
		wWindowConfigure(wwin, fx, fy, fw, fh - vert_border);
	    }
#ifdef DEBUG
	    puts("End resize window");
#endif
	    return;
	    
	 default:
	    WMHandleEvent(&event);
	}
    }
}

#undef LEFT
#undef RIGHT
#undef HORIZONTAL
#undef UP
#undef DOWN
#undef VERTICAL
#undef HCONSTRAIN
#undef RESIZEBAR

void
wUnselectWindows()
{
    WWindow *wwin;
    
    while (wSelectedWindows) {
	wwin = wSelectedWindows->head;
        if (wwin->flags.miniaturized && wwin->icon && wwin->icon->selected)
            wIconSelect(wwin->icon);

        XSetWindowBorder(dpy, wwin->frame->core->window,
                         wwin->screen_ptr->frame_border_pixel);
	wwin->flags.selected = 0;
	list_remove_head(&wSelectedWindows);
    }
}


void
wSelectWindow(WWindow *wwin)
{
    if (!wwin->flags.selected) {
	wwin->flags.selected = 1;
	XSetWindowBorder(dpy, wwin->frame->core->window, 
			 wwin->screen_ptr->white_pixel);
	wSelectedWindows = list_cons(wwin, wSelectedWindows);
    } else {
	wwin->flags.selected = 0;
	XSetWindowBorder(dpy, wwin->frame->core->window, 
			 wwin->screen_ptr->frame_border_pixel);
	wSelectedWindows = list_remove_elem(wSelectedWindows, wwin);
    }
}


static void
selectWindowsInside(WScreen *scr, int x1, int y1, int x2, int y2)
{
    WWindow *tmpw;
    
    /* select the windows and put them in the selected window list */
    tmpw = scr->focused_window;
    while (tmpw != NULL) {
	if (!(tmpw->flags.miniaturized || tmpw->flags.hidden)) {
	    if ((tmpw->frame->workspace == scr->current_workspace
		 || tmpw->window_flags.omnipresent)
		&& (tmpw->frame_x >= x1) && (tmpw->frame_y >= y1) 
		&& (tmpw->frame->core->width + tmpw->frame_x <= x2)
		&& (tmpw->frame->core->height + tmpw->frame_y <= y2)) {
		XSetWindowBorder(dpy, tmpw->frame->core->window, 
				 tmpw->screen_ptr->white_pixel);
		tmpw->flags.selected = 1;
		wSelectedWindows = list_cons(tmpw, wSelectedWindows);
	    }
	}
	tmpw = tmpw->prev;
    }
}


void
wSelectWindows(WScreen *scr, XEvent *ev)
{
    XEvent event;
    Window root = scr->root_win;
    GC gc = scr->frame_gc;
    int xp = ev->xbutton.x_root;
    int yp = ev->xbutton.y_root;
    int w = 0, h = 0;
    int x = xp, y = yp;
    
#ifdef DEBUG
    puts("Selecting windows");
#endif
    if (XGrabPointer(dpy, scr->root_win, False, ButtonMotionMask
		     | ButtonReleaseMask | ButtonPressMask, GrabModeAsync,
		     GrabModeAsync, None, wCursor[WCUR_DEFAULT],
		     CurrentTime) != Success) {
	return;
    }
    XGrabServer(dpy);
    
    wUnselectWindows();
    
    XDrawRectangle(dpy, root, gc, xp, yp, w, h);
    while (1) {
	WMMaskEvent(dpy, ButtonReleaseMask | PointerMotionMask
		    | ButtonPressMask, &event);
	
	switch (event.type) {
	 case MotionNotify:
	    XDrawRectangle(dpy, root, gc, x, y, w, h);
	    x = event.xmotion.x_root;
	    if (x < xp) {
		w = xp - x;
	    } else {
		w = x - xp;
		x = xp;
	    }
	    y = event.xmotion.y_root;
	    if (y < yp) {
		h = yp - y;
	    } else {
		h = y - yp;
		y = yp;
	    }
	    XDrawRectangle(dpy, root, gc, x, y, w, h);
	    break;

	 case ButtonPress:
	    break;

	 case ButtonRelease:
	    if (event.xbutton.button != ev->xbutton.button)
		break;

	    XDrawRectangle(dpy, root, gc, x, y, w, h);
	    XUngrabServer(dpy);
	    XUngrabPointer(dpy, CurrentTime);
	    selectWindowsInside(scr, x, y, x + w, y + h);
#ifdef DEBUG
	    puts("End window selection");
#endif
	    return;
	    
	 default:
	    WMHandleEvent(&event);
	}
    }
}


void
InteractivePlaceWindow(WWindow *wwin, int *x_ret, int *y_ret)
{
    WScreen *scr = wwin->screen_ptr;
    Window root = scr->root_win;
    int x, y, h = 0;
    int width = wwin->client.width;
    int height = wwin->client.height;
    XEvent event;
    KeyCode shiftl, shiftr;
    Window junkw;
    int junk;
    
    if (XGrabPointer(dpy, root, True, PointerMotionMask | ButtonPressMask,
		     GrabModeAsync, GrabModeAsync, None, 
		     wCursor[WCUR_DEFAULT], CurrentTime) != Success) {
	*x_ret = 0;
	*y_ret = 0;
	return;
    }
    if (!wwin->window_flags.no_titlebar) {
	h = scr->title_font->height + TITLEBAR_EXTRA_HEIGHT;
	height += h;
    }
    if (!wwin->window_flags.no_resizebar) {
	height += RESIZEBAR_HEIGHT;
    }
    XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    XQueryPointer(dpy, root, &junkw, &junkw, &x, &y, &junk, &junk, 
		  (unsigned *) &junk);
    mapPositionDisplay(wwin, x - width/2, y - h/2, width, height);
    
    drawTransparentFrame(wwin, x - width/2, y - h/2, width, height);
    
    shiftl = XKeysymToKeycode(dpy, XK_Shift_L);
    shiftr = XKeysymToKeycode(dpy, XK_Shift_R);
    while (1) {
	WMMaskEvent(dpy, PointerMotionMask|ButtonPressMask|ExposureMask|KeyPressMask,
		   &event);
	switch (event.type) {
	 case KeyPress:
	    if ((event.xkey.keycode == shiftl) 
		|| (event.xkey.keycode == shiftr)) {
		drawTransparentFrame(wwin,
				     x - width/2, y - h/2, width, height);
		cyclePositionDisplay(wwin,
				     x - width/2, y - h/2, width, height);
		drawTransparentFrame(wwin,
				     x - width/2, y - h/2, width, height);
	    }
	    break;
	    
	 case MotionNotify:
	    drawTransparentFrame(wwin, x - width/2, y - h/2, width, height);
	    
	    x = event.xmotion.x_root;
	    y = event.xmotion.y_root;
	    
	    if (wPreferences.move_display == WDIS_FRAME_CENTER)
		moveGeometryDisplayCentered(scr, x, y + (height - h) / 2);
	    
	    showPosition(wwin, x - width/2, y - h/2);
	    
	    drawTransparentFrame(wwin, x - width/2, y - h/2, width, height);
	    
	    break;
	    
	 case ButtonPress:
	    drawTransparentFrame(wwin, x - width/2, y - h/2, width, height);
	    XSync(dpy, 0);
	    *x_ret = x - width/2;
	    *y_ret = y - h/2;
	    XUngrabPointer(dpy, CurrentTime);
	    XUngrabKeyboard(dpy, CurrentTime);
	    /* get rid of the geometry window */
	    unmapPositionDisplay(wwin);
	    return;
	    
	 default:
	    WMHandleEvent(&event);
	    break;
	}
    }
}
