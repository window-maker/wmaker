/* placement.c - window and icon placement on screen
 * 
 *  Window Maker window manager
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "icon.h"
#include "actions.h"
#include "funcs.h"
#include "application.h"
#include "appicon.h"
#include "dock.h"

extern WPreferences wPreferences;


#define X_ORIGIN     wPreferences.window_place_origin.x
#define Y_ORIGIN     wPreferences.window_place_origin.y


/*
 * interactive window placement is in moveres.c
 */

extern void
InteractivePlaceWindow(WWindow *wwin, int *x_ret, int *y_ret);

#if 0
void
PlaceIcon(WScreen *scr, int *x_ret, int *y_ret)
{
    int x1, y1, x2, y2;
    int icon_x, icon_y;
    int left_margin, right_margin;
    WCoreWindow *wcore;

    left_margin = 0;
    right_margin = scr->scr_width;
    if (scr->dock) {
	if (scr->dock->on_right_side)
            right_margin -= wPreferences.icon_size + DOCK_EXTRA_SPACE;
	else
	    left_margin += wPreferences.icon_size + DOCK_EXTRA_SPACE;
    }

    x1 = left_margin;
    y2 = scr->scr_height;
    y1 = y2-wPreferences.icon_size*2;
    x2 = left_margin+wPreferences.icon_size;

    while (1) {
	wcore = scr->stacking_list[0];
	
	while (wcore) {
            void *parent;

	    if (x2>=right_margin+wPreferences.icon_size) {
		x1 = left_margin;
		x2 = left_margin+wPreferences.icon_size*2;
		y1 -= wPreferences.icon_size;
		y2 -= wPreferences.icon_size;
		if (y2<wPreferences.icon_size) {
		    /* what's this guy doing!? */
		    *x_ret = 0;
		    *y_ret = 0;
		    return;
		}
		break;
	    }

            parent = (void*) wcore->descriptor.parent;

	    /* if it is an application icon */
	    if (wcore->descriptor.parent_type == WCLASS_APPICON) {
		icon_x = ((WAppIcon*)parent)->x_pos;
		icon_y = ((WAppIcon*)parent)->y_pos;
            } else if (wcore->descriptor.parent_type == WCLASS_MINIWINDOW &&
                       (((WIcon*)parent)->owner->frame->workspace==scr->current_workspace
                        || ((WIcon*)parent)->owner->window_flags.omnipresent
                        || wPreferences.sticky_icons)) {
		icon_x = ((WIcon*)parent)->owner->icon_x;
		icon_y = ((WIcon*)parent)->owner->icon_y;
	    } else {
		wcore = wcore->stacking->under;
		continue;
	    }
	    wcore = wcore->stacking->under;
	    
	    /* test if place is taken */
	    if (icon_y>y1 && icon_y<y2) {
		if (icon_x<x2 && icon_x>=x1) { 
		    x2 = icon_x+wPreferences.icon_size*2;
		    x1 = icon_x+wPreferences.icon_size;
		    /* this place can't be used */
		    wcore = scr->stacking_list[0];
		    break;
		}
	    }
	}
	if (!wcore) {
	    /* found spot */
	    break;
	}
    }
    *x_ret = x1;
    *y_ret = y2-wPreferences.icon_size;
}
#endif

/*
 * Returns True if it is an icon and is in this workspace.
 */
static Bool
iconPosition(WCoreWindow *wcore, int sx1, int sy1, int sx2, int sy2, 
	     int workspace, int *retX, int *retY)
{
    void *parent;
    int ok = 0;
    
    parent = wcore->descriptor.parent;

    /* if it is an application icon */
    if (wcore->descriptor.parent_type == WCLASS_APPICON) {
	*retX = ((WAppIcon*)parent)->x_pos;
	*retY = ((WAppIcon*)parent)->y_pos;

	ok = 1;
    } else if (wcore->descriptor.parent_type == WCLASS_MINIWINDOW &&
	       (((WIcon*)parent)->owner->frame->workspace==workspace
		|| ((WIcon*)parent)->owner->window_flags.omnipresent
		|| wPreferences.sticky_icons)) {

	*retX = ((WIcon*)parent)->owner->icon_x;
	*retY = ((WIcon*)parent)->owner->icon_y;

	ok = 1;
    }
    
    /*
     * Check if it is inside the screen.
     */
    if (ok) {
	if (*retX < sx1-wPreferences.icon_size)
	    ok = 0;
	else if (*retX > sx2)
	    ok = 0;

	if (*retY < sy1-wPreferences.icon_size)
	    ok = 0;
	else if (*retY > sy2)
	    ok = 0;
    }

    return ok;
}




void
PlaceIcon(WScreen *scr, int *x_ret, int *y_ret)
{
    int pf;			       /* primary axis */
    int sf;			       /* secondary axis */
    int fullW;
    int fullH;
    char *map;
    int pi, si;
    WCoreWindow *obj;
    int sx1, sx2, sy1, sy2;	       /* screen boundary */
    int sw, sh;
    int xo, yo;
    int xs, ys;
    int x, y;
    int isize = wPreferences.icon_size;
    int done = 0;

    /*
     * Find out screen boundaries.
     */
    sx1 = 0;
    sy1 = 0;
    sx2 = scr->scr_width;
    sy2 = scr->scr_height;
    if (scr->dock) {
	if (scr->dock->on_right_side)
            sx2 -= isize + DOCK_EXTRA_SPACE;
	else
	    sx1 += isize + DOCK_EXTRA_SPACE;
    }

    sw = isize * (scr->scr_width/isize);
    sh = isize * (scr->scr_height/isize);
    fullW = (sx2-sx1)/isize;
    fullH = (sy2-sy1)/isize;

    /* icon yard boundaries */    
    if (wPreferences.icon_yard & IY_VERT) {
	pf = fullH;
	sf = fullW;
    } else {
	pf = fullW;
	sf = fullH;
    }
    if (wPreferences.icon_yard & IY_RIGHT) {
	xo = sx2 - isize;
	xs = -1;
    } else {
	xo = sx1;
	xs = 1;
    }
    if (wPreferences.icon_yard & IY_TOP) {
	yo = sy1;
	ys = 1;
    } else {
	yo = sy2 - isize;
	ys = -1;
    }

    /*
     * Create a map with the occupied slots. 1 means the slot is used
     * or at least partially used. 
     * The slot usage can be optimized by only marking fully used slots
     * or slots that have most of it covered. 
     * Space usage is worse than the fvwm algorithm (used in the old version)
     * but complexity is much better (faster) than it.
     */
    map = wmalloc((sw+2) * (sh+2));
    memset(map, 0, (sw+2) * (sh+2));

#define INDEX(x,y)	(((y)+1)*(sw+2) + (x) + 1)

    obj = scr->stacking_list[0];
    while (obj) {
	int x, y;

	if (iconPosition(obj, sx1, sy1, sx2, sy2, scr->current_workspace, 
			 &x, &y)) {
	    int xdi, ydi;	       /* rounded down */
	    int xui, yui;	       /* rounded up */

	    xdi = x/isize;
	    ydi = y/isize;
	    xui = (x+isize/2)/isize;
	    yui = (y+isize/2)/isize;
	    map[INDEX(xdi,ydi)] = 1;
	    map[INDEX(xdi,yui)] = 1;
	    map[INDEX(xui,ydi)] = 1;
	    map[INDEX(xui,yui)] = 1;
	}
	obj = obj->stacking->under;
    }

    /*
     * Default position
     */
    *x_ret = 0;
    *y_ret = 0;

    /*
     * Look for an empty slot
     */
    for (si=0; si<sf; si++) {
	for (pi=0; pi<pf; pi++) {
	    if (wPreferences.icon_yard & IY_VERT) {
		x = xo + xs*(si*isize);
		y = yo + ys*(pi*isize);
	    } else {
		x = xo + xs*(pi*isize);
		y = yo + ys*(si*isize);
	    }
	    if (!map[INDEX(x/isize, y/isize)]) {
		*x_ret = x;
		*y_ret = y;
		done = 1;
		break;
	    }
	}
	if (done)
	    break;
    }

    free(map);
}




static int
smartPlaceWindow(WWindow *wwin, int *x_ret, int *y_ret,
                 unsigned int width, unsigned int height)
{
    WScreen *scr = wwin->screen_ptr;
    int test_x = 0, test_y = Y_ORIGIN;
    int loc_ok = False, tw,tx,ty,th;
    int swidth, sx;
    WWindow *test_window;
    int extra_height;

    if (wwin->frame)
	extra_height = wwin->frame->top_width + wwin->frame->bottom_width + 2;
    else
	extra_height = 24; /* random value */
    
    swidth = scr->scr_width;
    sx = X_ORIGIN;
    if (scr->dock && !scr->dock->lowered) {
        if (scr->dock->on_right_side)
            swidth -= wPreferences.icon_size + DOCK_EXTRA_SPACE;
        else if (X_ORIGIN < wPreferences.icon_size + DOCK_EXTRA_SPACE)
            sx += wPreferences.icon_size + DOCK_EXTRA_SPACE - X_ORIGIN;
    }

    /* this was based on fvwm2's smart placement */

    height += extra_height;

    while (((test_y + height) < (scr->scr_height)) && (!loc_ok)) {
	
	test_x = sx;
	
	while (((test_x + width) < swidth) && (!loc_ok)) {
	    
	    loc_ok = True;
	    test_window = scr->focused_window;
            
	    while ((test_window != (WWindow *) 0) && (loc_ok == True)) {
		
                tw = test_window->client.width;
                if (test_window->flags.shaded)
                    th = test_window->frame->top_width;
                else
                    th = test_window->client.height+extra_height;
		tx = test_window->frame_x;
		ty = test_window->frame_y;
                
		if((tx<(test_x+width))&&((tx + tw) > test_x)&&
		   (ty<(test_y+height))&&((ty + th)>test_y)&&
                   (test_window->flags.mapped ||
                    (test_window->flags.shaded &&
                     !(test_window->flags.miniaturized ||
                       test_window->flags.hidden)))) {

                    loc_ok = False;
		}
		test_window = test_window->next;
	    }
	    
	    test_window = scr->focused_window;
	    
	    while ((test_window != (WWindow *) 0) && (loc_ok == True))  {
		
                tw = test_window->client.width;
                if (test_window->flags.shaded)
                    th = test_window->frame->top_width;
                else
                    th = test_window->client.height+extra_height;
		tx = test_window->frame_x;
		ty = test_window->frame_y;
		
		if((tx<(test_x+width))&&((tx + tw) > test_x)&&
		   (ty<(test_y+height))&&((ty + th)>test_y)&&
                   (test_window->flags.mapped ||
                    (test_window->flags.shaded &&
                     !(test_window->flags.miniaturized ||
                       test_window->flags.hidden)))) {

                    loc_ok = False;
		}
		test_window = test_window->prev;
	    }
	    if (loc_ok == True) {
		*x_ret = test_x;
		*y_ret = test_y;
		break;
	    }
	    test_x += PLACETEST_HSTEP;
	}
	test_y += PLACETEST_VSTEP;
    }
    return loc_ok;
}


static void 
cascadeWindow(WScreen *scr, WWindow *wwin, int *x_ret, int *y_ret,
              unsigned int width, unsigned int height, int h)
{
    unsigned int extra_height;
    unsigned int scr_width;
    int xoffset = 0;

    scr_width = scr->scr_width;
    if (scr->dock && !scr->dock->lowered) {
	if (scr->dock->on_right_side) {
	    scr_width -= wPreferences.icon_size;
	} else {
	    xoffset = wPreferences.icon_size;
	}
    }

    if (wwin->frame)
	extra_height = wwin->frame->top_width + wwin->frame->bottom_width;
    else
	extra_height = 24; /* random value */
    
    *x_ret = h * scr->cascade_index + X_ORIGIN;
    *y_ret = h * scr->cascade_index + Y_ORIGIN;
    height += extra_height;
    
    if (width + *x_ret > scr_width || height + *y_ret > scr->scr_height) {
	scr->cascade_index = 0;
	*x_ret = h*scr->cascade_index + X_ORIGIN;
	*y_ret = h*scr->cascade_index + Y_ORIGIN;
    }
}


void
PlaceWindow(WWindow *wwin, int *x_ret, int *y_ret,
            unsigned width, unsigned height)
{
    WScreen *scr = wwin->screen_ptr;
    int h = scr->title_font->height+TITLEBAR_EXTRA_HEIGHT;

    switch (wPreferences.window_placement) {
     case WPM_MANUAL:
	InteractivePlaceWindow(wwin, x_ret, y_ret);
	break;

     case WPM_SMART:
	if (smartPlaceWindow(wwin, x_ret, y_ret, width, height))
	  break;
	/* there isn't a break here, because if we fail, it should fall
	   through to cascade placement, as people who want tiling want
	   automagicness aren't going to want to place their window */

     case WPM_CASCADE:
        if (wPreferences.window_placement == WPM_SMART)
            scr->cascade_index++;

	cascadeWindow(scr, wwin, x_ret, y_ret, width, height, h);

        if (wPreferences.window_placement == WPM_CASCADE)
            scr->cascade_index++;

        if (scr->dock && !scr->dock->lowered) {
            int x2;

            x2 = *x_ret + width;
            if (scr->dock->on_right_side
                && x2 > scr->scr_width - wPreferences.icon_size -
                        DOCK_EXTRA_SPACE)
                *x_ret = scr->scr_width - width
                         - wPreferences.icon_size - DOCK_EXTRA_SPACE;
            else if (!scr->dock->on_right_side &&
                     X_ORIGIN < wPreferences.icon_size + DOCK_EXTRA_SPACE)
                *x_ret += wPreferences.icon_size + DOCK_EXTRA_SPACE - X_ORIGIN;
        }
	break;

     case WPM_RANDOM:
	{
	    int w, h;
	    
	    w = (scr->scr_width - width);
	    h = (scr->scr_height - height);
	    if (w<1) w = 1;
	    if (h<1) h = 1;
	    *x_ret = rand()%w;
	    *y_ret = rand()%h;
	    if (scr->dock && !scr->dock->lowered) {
		int x2;
		    
		x2 = *x_ret + width;
		if (scr->dock->on_right_side 
                    && x2 > scr->scr_width - wPreferences.icon_size -
                            DOCK_EXTRA_SPACE)
                    *x_ret = scr->scr_width - width
                             - wPreferences.icon_size - DOCK_EXTRA_SPACE;
		else if (!scr->dock->on_right_side 
			 && *x_ret < wPreferences.icon_size)
		    *x_ret = wPreferences.icon_size + DOCK_EXTRA_SPACE;
	    }
	}
	break;

#ifdef DEBUG	
     default:
	puts("Invalid window placement!!!");
	*x_ret = 0;
	*y_ret = 0;
#endif
    }
    
    if (*x_ret < 0)
	*x_ret = 0;
    else if (*x_ret + width > scr->scr_width)
	*x_ret = scr->scr_width - width;
    
    if (*y_ret < 0)
	*y_ret = 0;
    else if (*y_ret + height > scr->scr_height)
	*y_ret = scr->scr_height - height;
}


