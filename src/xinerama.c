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


#include "xinerama.h"

#include "screen.h"
#include "window.h"
#include "framewin.h"
#include "wcore.h"
#include "funcs.h"

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

void wInitXinerama(WScreen *scr)
{
    scr->xine_screens = 0;
    scr->xine_count = 0;
#ifdef XINERAMA
    scr->xine_screens = XineramaQueryScreens(dpy, &scr->xine_count);
#endif
    scr->xine_primary_head = 0;
}


int wGetRectPlacementInfo(WScreen *scr, WMRect rect, int *flags)
{
    int best;
    unsigned long area, totalArea;
    int i;
    int rx = rect.pos.x;
    int ry = rect.pos.y;
    int rw = rect.size.width;
    int rh = rect.size.height;

    wassertrv(flags!=NULL, 0);
    
    best = -1;
    area = 0;
    totalArea = 0;

    *flags = XFLAG_NONE;

    if (scr->xine_count <= 1) {
	unsigned long a;

	a = calcIntersectionArea(rx, ry, rw, rh,
				 0, 0, scr->scr_width, scr->scr_height);

	if (a == 0) {
	    *flags |= XFLAG_DEAD;
	} else if (a != rw*rh) {
	    *flags |= XFLAG_PARTIAL;
	}
	
	return scr->xine_primary_head;
    }

#ifdef XINERAMA
    for (i = 0; i < scr->xine_count; i++) {
	unsigned long a;
	
	a = calcIntersectionArea(rx, ry, rw, rh,
				 scr->xine_screens[i].x_org,
				 scr->xine_screens[i].y_org,
				 scr->xine_screens[i].width,
				 scr->xine_screens[i].height);
	    
	totalArea += a;
	if (a > area) {
	    if ( best != -1)
		*flags |= XFLAG_MULTIPLE;
	    area = a;
	    best = i;
	}
    }

    if ( best == -1) {
	*flags |= XFLAG_DEAD;
	best = wGetHeadForPointerLocation(scr);
    } else if ( totalArea != rw*rh) 
	*flags |= XFLAG_PARTIAL;

    return best;
#endif
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

    if (!scr->xine_count)
	return scr->xine_primary_head;

    best = -1;
    area = 0;
    
#ifdef XINERAMA    
    for (i = 0; i < scr->xine_count; i++) {
	unsigned long a;
	
	a = calcIntersectionArea(rx, ry, rw, rh,
				 scr->xine_screens[i].x_org,
				 scr->xine_screens[i].y_org,
				 scr->xine_screens[i].width,
				 scr->xine_screens[i].height);
	
	if (a > area) {
	    area = a;
	    best = i;
	}
    }

    /*
     * in case rect is in dead space, return valid head
     */
    if (best == -1)
	best = wGetHeadForPointerLocation(scr);

    return best;
#else /* !XINERAMA */
    return scr->xine_primary_head;
#endif /* !XINERAMA */
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


/*
int wGetHeadForPoint(WScreen *scr, WMPoint point, int *flags)
{
    int i;	

    // paranoia
    if ( flags == NULL) {
	static int tmp;
	flags = &tmp;
    }
    *flags = XFLAG_NONE;
    
    for (i = 0; i < scr->xine_count; i++) {
#if 0
	int yy, xx;
	
	xx = scr->xine_screens[i].x_org + scr->xine_screens[i].width;
	yy = scr->xine_screens[i].y_org + scr->xine_screens[i].height;
	if (point.x >= scr->xine_screens[i].x_org &&
	    point.y >= scr->xine_screens[i].y_org &&
	    point.x < xx && point.y < yy) {
	    return i;
	}
#else
	XineramaScreenInfo *xsi = &scr->xine_screens[i];

	if ((unsigned)(point.x - xsi->x_org) < xsi->width &&
	    (unsigned)(point.y - xsi->y_org) < xsi->height)
	    return i;
#endif
    }

    *flags |= XFLAG_DEAD;
    
    return scr->xine_primary_head;
}
*/



int wGetHeadForPoint(WScreen *scr, WMPoint point)
{
#ifdef XINERAMA
    int i;	

    for (i = 0; i < scr->xine_count; i++) {
	XineramaScreenInfo *xsi = &scr->xine_screens[i];

	if ((unsigned)(point.x - xsi->x_org) < xsi->width &&
	    (unsigned)(point.y - xsi->y_org) < xsi->height)
	    return i;
    }
#endif /* XINERAMA */
    return scr->xine_primary_head;
}


int wGetHeadForPointerLocation(WScreen *scr)
{
    WMPoint point;
    Window bla;
    int ble;
    unsigned int blo;

    if (!scr->xine_count)
	return scr->xine_primary_head;
    
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

#ifdef XINERAMA
    if (head < scr->xine_count) {
	rect.pos.x = scr->xine_screens[head].x_org;
	rect.pos.y = scr->xine_screens[head].y_org;
	rect.size.width = scr->xine_screens[head].width;
	rect.size.height = scr->xine_screens[head].height;	
    } else
#endif /* XINERAMA */
    {
	rect.pos.x = 0;
	rect.pos.y = 0;
	rect.size.width = scr->scr_width;
	rect.size.height = scr->scr_height;
    }

    return rect;
}



WArea wGetUsableAreaForHead(WScreen *scr, int head, WArea *totalAreaPtr)
{
    WArea totalArea, usableArea = scr->totalUsableArea;
    WMRect rect = wGetRectForHead(scr, head);

    totalArea.x1 = rect.pos.x;
    totalArea.y1 = rect.pos.y;
    totalArea.x2 = totalArea.x1 + rect.size.width;
    totalArea.y2 = totalArea.y1 + rect.size.height;

    if (totalAreaPtr != NULL) *totalAreaPtr = totalArea;

    usableArea.x1 = WMAX(totalArea.x1, usableArea.x1);
    usableArea.y1 = WMAX(totalArea.y1, usableArea.y1);
    usableArea.x2 = WMIN(totalArea.x2, usableArea.x2);
    usableArea.y2 = WMIN(totalArea.y2, usableArea.y2);

    return usableArea;
}


WMPoint wGetPointToCenterRectInHead(WScreen *scr, int head, int width, int height)
{
    WMPoint p;
    WMRect rect = wGetRectForHead(scr, head);

    p.x = rect.pos.x + (rect.size.width - width)/2;
    p.y = rect.pos.y + (rect.size.height - height)/2;

    return p;
}

