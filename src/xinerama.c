/*
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997-2001 Alfredo K. Kojima
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


#ifdef XINERAMA


#include "xinerama.h"

#include "screen.h"
#include "window.h"
#include "framewin.h"
#include "wcore.h"

#include <X11/extensions/Xinerama.h>


void wInitXinerama(WScreen *scr)
{
    scr->xine_screens = XineramaQueryScreens(dpy, &scr->xine_count);

    scr->xine_primary_head = 0;
}


/*
 * intersect_rectangles-
 *     Calculate the rectangle that results from the intersection of
 *     2 rectangles.
 * 
 * Returns:
 *     0 if the rectangles do not intersect, 1 otherwise.
 */

typedef struct {
    int x1, y1, x2, y2;
} _Rectangle;

int intersect_rectangles(_Rectangle *rect1,
			 _Rectangle *rect2,
			 _Rectangle *result)
{
    if (rect1->x1 < rect2->x1)
	result->x1 = rect2->x1;
    else
	result->x1 = rect1->x1;
    
    if (rect1->x2 > rect2->x2)
	result->x2 = rect2->x2;
    else
	result->x2 = rect1->x2;
    
    if (rect1->y1 < rect2->y1)
	result->y1 = rect2->y1;
    else
	result->y1 = rect1->y1;
    
    if (rect1->y2 > rect2->y2)
	result->y2 = rect2->y2;
    else
	result->y2 = rect1->y2;
    
    if (result->x2 < result->x1)
	return 0;
    if (result->y2 < result->y1)
	return 0;

    return 1;
}

static int intersectArea(int x1, int y1, int w1, int h1,
			 int x2, int y2, int w2, int h2)
{
    _Rectangle rect1, rect2, result;
    
    rect1.x1 = x1;
    rect1.y1 = y1;
    rect1.x2 = x1+w1;
    rect1.y2 = y1+h1;
    
    rect2.x1 = x2;
    rect2.y1 = y2;
    rect2.x2 = x2+w2;
    rect2.y2 = y2+h2;
    
    if (intersect_rectangles(&rect1, &rect2, &result))
	return (result.x2-result.x1)*(result.y2-result.y1);
    else
	return 0;
}


/* get the head that covers most of the rectangle */
int wGetHeadForRect(WScreen *scr, WMRect rect)
{
    int best;
    unsigned long area;
    int i;
    int rx = rect.pos.x;
    int ry = rect.pos.y;
    int rw = rect.size.width;
    int rh = rect.size.height;

    best = -1;
    area = 0;
    
    for (i = 0; i < scr->xine_count; i++) {
	unsigned long a;
	
	a = intersectArea(rx, ry, rw, rh,
			  scr->xine_screens[i].x_org,
			  scr->xine_screens[i].y_org,
			  scr->xine_screens[i].width,
			  scr->xine_screens[i].height);
	
	if (a > area) {
	    area = a;
	    best = i;
	}
    }

    return best;
}


int wGetHeadForWindow(WWindow *wwin)
{
    WMRect rect;

    rect.pos.x = wwin->frame_x;
    rect.pos.y = wwin->frame_y;
    rect.size.width = wwin->frame->core->width;
    rect.size.height = wwin->frame->core->height;

    return wGetHeadForRect(wwin->screen_ptr, rect);
}


int wGetHeadForPoint(WScreen *scr, WMPoint point)
{
    int i;	

    for (i = 0; i < scr->xine_count; i++) {
	int yy, xx;
	
	xx = scr->xine_screens[i].x_org + scr->xine_screens[i].width;
	yy = scr->xine_screens[i].y_org + scr->xine_screens[i].height;
	if (point.x >= scr->xine_screens[i].x_org &&
	    point.y >= scr->xine_screens[i].y_org &&
	    point.x < xx && point.y < yy) {
	    return i;
	}
    }
    
    return scr->xine_primary_head;
}


int wGetHeadForPointerLocation(WScreen *scr)
{
    WMPoint point;
    Window bla;
    int ble;
    unsigned int blo;

    
    if (!XQueryPointer(dpy, scr->root_win, &bla, &bla,
		       &point.x, &point.y,
		       &ble, &ble,
		       &blo))
	return scr->xine_primary_head;

    return wGetHeadForPoint(scr, point);
}

/* get the dimensions of the head */
WMRect
wGetRectForHead(WScreen *scr, int head)
{
    WMRect rect;

    if (head < scr->xine_count) {
	rect.pos.x = scr->xine_screens[head].x_org;
	rect.pos.y = scr->xine_screens[head].y_org;
	rect.size.width = scr->xine_screens[head].width;
	rect.size.height = scr->xine_screens[head].height;	
    } else {
	rect.pos.x = 0;
	rect.pos.y = 0;
	rect.size.width = scr->scr_width;
	rect.size.height = scr->scr_height;
    }

    return rect;
}


WMRect
wGetUsableRectForHead(WScreen *scr, int head)
{
    WMRect rect;

    if (head < scr->xine_count) {
	rect.pos.x = scr->xine_screens[head].x_org;
	rect.pos.y = scr->xine_screens[head].y_org;
	rect.size.width = scr->xine_screens[head].width;
	rect.size.height = scr->xine_screens[head].height;	
    } else {
	rect.pos.x = 0;
	rect.pos.y = 0;
	rect.size.width = scr->scr_width;
	rect.size.height = scr->scr_height;
    }

    return rect;
}

#endif 
