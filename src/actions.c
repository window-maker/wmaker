/* action.c- misc. window commands (miniaturize, hide etc.)
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998, 1999 Alfredo K. Kojima
 *  Copyright (c) 1998       Dan Pascu
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
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "client.h"
#include "icon.h"
#include "funcs.h"
#include "application.h"
#include "actions.h"
#include "stacking.h"
#include "appicon.h"
#include "dock.h"
#include "appmenu.h"
#include "winspector.h"
#include "workspace.h"
#include "wsound.h"

#ifdef GNOME_STUFF
# include "gnome.h"
#endif
#ifdef KWM_HINTS
# include "kwm.h"
#endif



/****** Global Variables ******/
extern Time LastTimestamp;
extern Time LastFocusChange;

extern Cursor wCursor[WCUR_LAST];

extern WPreferences wPreferences;

extern Atom _XA_WM_TAKE_FOCUS;


/******* Local Variables *******/
static struct {
    int steps;
    int delay;
} shadePars[5] = {
    {SHADE_STEPS_UF, SHADE_DELAY_UF},
    {SHADE_STEPS_F, SHADE_DELAY_F},
    {SHADE_STEPS_M, SHADE_DELAY_M},
    {SHADE_STEPS_S, SHADE_DELAY_S},
    {SHADE_STEPS_U, SHADE_DELAY_U}};

#define SHADE_STEPS	shadePars[(int)wPreferences.shade_speed].steps
#define SHADE_DELAY	shadePars[(int)wPreferences.shade_speed].delay


static int ignoreTimestamp=0;


#ifdef ANIMATIONS
static void
processEvents(int event_count)
{
    XEvent event;

    /*
     * This is a hack. When animations are enabled, processing of 
     * events ocurred during a animation are delayed until it's end.
     * Calls that consider the TimeStamp, like XSetInputFocus(), will
     * fail because the TimeStamp is too old. Then, for example, if
     * the user changes window focus while a miniaturize animation is
     * in course, the window will not get focus properly after the end
     * of the animation. This tries to workaround it by passing CurrentTime
     * as the TimeStamp for XSetInputFocus() for all events ocurred during
     * the animation.
     */
    ignoreTimestamp=1;
    while (XPending(dpy) && event_count--) {
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
    ignoreTimestamp=0;
}
#endif /* ANIMATIONS */



/*
 *---------------------------------------------------------------------- 
 * wSetFocusTo--
 * 	Changes the window focus to the one passed as argument.
 * If the window to focus is not already focused, it will be brought
 * to the head of the list of windows. Previously focused window is
 * unfocused. 
 * 
 * Side effects:
 * 	Window list may be reordered and the window focus is changed.
 * 
 *---------------------------------------------------------------------- 
 */
void
wSetFocusTo(WScreen *scr, WWindow  *wwin)
{
    static WScreen *old_scr=NULL;

    WWindow *old_focused;
    WWindow *focused=scr->focused_window;
    int timestamp=LastTimestamp;
    WApplication *oapp=NULL, *napp=NULL;
    int wasfocused;

    if (!old_scr)
      old_scr=scr;
    old_focused=old_scr->focused_window;

    LastFocusChange = timestamp;

/*
 * This is a hack, because XSetInputFocus() should have a proper
 * timestamp instead of CurrentTime but it seems that some times
 * clients will not receive focus properly that way.
    if (ignoreTimestamp)
*/
      timestamp = CurrentTime;

    if (old_focused)
	oapp = wApplicationOf(old_focused->main_window);

    if (wwin == NULL) {
	XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
	if (old_focused) {
	    wWindowUnfocus(old_focused);
	}
	if (oapp) {
	    wAppMenuUnmap(oapp->menu);
#ifdef NEWAPPICON
	    wApplicationDeactivate(oapp);
#endif
	}
#ifdef KWM_HINTS
	wKWMUpdateActiveWindowHint(scr);
	wKWMSendEventMessage(NULL, WKWMFocusWindow);
#endif
	return;
    } else if (old_scr != scr && old_focused) {
        wWindowUnfocus(old_focused);
    }

    wasfocused = wwin->flags.focused;
    napp = wApplicationOf(wwin->main_window);

    /* remember last workspace where the app has been */
    if (napp) 
	napp->last_workspace = wwin->screen_ptr->current_workspace;

    if (wwin->flags.mapped && !WFLAGP(wwin, no_focusable)) {
	/* install colormap if colormap mode is lock mode */
	if (wPreferences.colormap_mode==WKF_CLICK)
	    wColormapInstallForWindow(scr, wwin);

	/* set input focus */
	switch (wwin->focus_mode) {
	 case WFM_NO_INPUT:
	    XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
	    break;

	 case WFM_PASSIVE:
	 case WFM_LOCALLY_ACTIVE:
	    XSetInputFocus(dpy, wwin->client_win, RevertToParent, timestamp);
	    break;

	 case WFM_GLOBALLY_ACTIVE:
	    break;
	}
	XFlush(dpy);
	if (wwin->protocols.TAKE_FOCUS) {
	    wClientSendProtocol(wwin, _XA_WM_TAKE_FOCUS, timestamp);
	}
	XSync(dpy, False);
    } else {
	XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
    }
    if (WFLAGP(wwin, no_focusable))
	return;

    /* if this is not the focused window focus it */
    if (focused!=wwin) {
	/* change the focus window list order */
	if (wwin->prev)
	  wwin->prev->next = wwin->next;

	if (wwin->next)
	  wwin->next->prev = wwin->prev;

	wwin->prev = focused;
	focused->next = wwin;
	wwin->next = NULL;
	scr->focused_window = wwin;

	if (oapp && oapp != napp) {
	    wAppMenuUnmap(oapp->menu);
#ifdef NEWAPPICON
	    wApplicationDeactivate(oapp);
#endif
	}
    }

    wWindowFocus(wwin, focused);

    if (napp && !wasfocused) {
#ifdef USER_MENU
	wUserMenuRefreshInstances(napp->menu, wwin);
#endif /* USER_MENU */

    if (wwin->flags.mapped)
    	wAppMenuMap(napp->menu, wwin);
#ifdef NEWAPPICON
	wApplicationActivate(napp);
#endif
    }
#ifdef KWM_HINTS
    wKWMUpdateActiveWindowHint(scr);
    wKWMSendEventMessage(wwin, WKWMFocusWindow);
#endif
    XFlush(dpy);
    old_scr=scr;
}


void
wShadeWindow(WWindow  *wwin)
{
    time_t time0 = time(NULL);
#ifdef ANIMATIONS
    int y, s, w, h;
#endif

    if (wwin->flags.shaded)
	return;

    XLowerWindow(dpy, wwin->client_win);    

    wSoundPlay(WSOUND_SHADE);

#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wwin->flags.skip_next_animation
	&& !wPreferences.no_animations) {
	/* do the shading animation */
	h = wwin->frame->core->height;
        s = h/SHADE_STEPS;
        if (s < 1) s=1;
	w = wwin->frame->core->width;
	y = wwin->frame->top_width;
	while (h>wwin->frame->top_width+1) {
	    XMoveWindow(dpy, wwin->client_win, 0, y);
	    XResizeWindow(dpy, wwin->frame->core->window, w, h);
	    XFlush(dpy);

	    if (time(NULL)-time0 > MAX_ANIMATION_TIME)
		break;

            if (SHADE_DELAY > 0)
                wusleep(SHADE_DELAY*1000L);
	    h-=s;
	    y-=s;
	}
	XMoveWindow(dpy, wwin->client_win, 0, wwin->frame->top_width);
    }
#endif  /* ANIMATIONS */

    wwin->flags.skip_next_animation = 0;
    wwin->flags.shaded = 1;
    wwin->flags.mapped = 0;
    /* prevent window withdrawal when getting UnmapNotify */
    XSelectInput(dpy, wwin->client_win, 
                 wwin->event_mask & ~StructureNotifyMask);
    XUnmapWindow(dpy, wwin->client_win);
    XSelectInput(dpy, wwin->client_win, wwin->event_mask);

    /* for the client it's just like iconification */
    wFrameWindowResize(wwin->frame, wwin->frame->core->width,
		       wwin->frame->top_width - 1);
    
    wwin->client.y = wwin->frame_y - wwin->client.height 
	+ wwin->frame->top_width;
    wWindowSynthConfigureNotify(wwin);

    /*
    wClientSetState(wwin, IconicState, None);
     */

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMIconifiedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif
    /* update window list to reflect shaded state */
    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);

#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup) {
	/* Look at processEvents() for reason of this code. */
    	XSync(dpy, 0);
    	processEvents(XPending(dpy));
    }
#endif
}


void
wUnshadeWindow(WWindow  *wwin)
{
    time_t time0 = time(NULL);
#ifdef ANIMATIONS
    int y, s, w, h;    
#endif /* ANIMATIONS */


    if (!wwin->flags.shaded)
	return;
    
    wwin->flags.shaded = 0;
    wwin->flags.mapped = 1;
    XMapWindow(dpy, wwin->client_win);

    wSoundPlay(WSOUND_UNSHADE);

#ifdef ANIMATIONS
    if (!wPreferences.no_animations && !wwin->flags.skip_next_animation) {
	/* do the shading animation */
	h = wwin->frame->top_width + wwin->frame->bottom_width;
	y = wwin->frame->top_width - wwin->client.height;
        s = abs(y)/SHADE_STEPS;
        if (s<1) s=1;
	w = wwin->frame->core->width;
	XMoveWindow(dpy, wwin->client_win, 0, y);
	if (s>0) {
	    while (h < wwin->client.height + wwin->frame->top_width
		   + wwin->frame->bottom_width) {
		XResizeWindow(dpy, wwin->frame->core->window, w, h);
		XMoveWindow(dpy, wwin->client_win, 0, y);
		XFlush(dpy);
                if (SHADE_DELAY > 0)
                    wusleep(SHADE_DELAY*2000L/3);
		h+=s;
		y+=s;
		
		if (time(NULL)-time0 > MAX_ANIMATION_TIME)
		    break;
	    }
	}
	XMoveWindow(dpy, wwin->client_win, 0, wwin->frame->top_width);
    }
#endif /* ANIMATIONS */

    wwin->flags.skip_next_animation = 0;
    wFrameWindowResize(wwin->frame, wwin->frame->core->width, 
		       wwin->frame->top_width + wwin->client.height
		       + wwin->frame->bottom_width);

    wwin->client.y = wwin->frame_y + wwin->frame->top_width;
    wWindowSynthConfigureNotify(wwin);
    
    /*
    wClientSetState(wwin, NormalState, None);
     */
    /* if the window is focused, set the focus again as it was disabled during
     * shading */
    if (wwin->flags.focused)
	wSetFocusTo(wwin->screen_ptr, wwin);

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMIconifiedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif

    /* update window list to reflect unshaded state */
    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);

}


void
wMaximizeWindow(WWindow *wwin, int directions)
{
    int new_width, new_height, new_x, new_y;
    WArea usableArea = wwin->screen_ptr->totalUsableArea;
    WArea totalArea;


    if (WFLAGP(wwin, no_resizable))
	return;
    
    totalArea.x1 = 0;
    totalArea.y1 = 0;
    totalArea.x2 = wwin->screen_ptr->scr_width;
    totalArea.y2 = wwin->screen_ptr->scr_height;

#ifdef XINERAMA
    if (wwin->screen_ptr->xine_count > 0
	&& !(directions & MAX_IGNORE_XINERAMA)) {
	WScreen *scr = wwin->screen_ptr;
	WMRect rect;
	
	rect = wGetRectForHead(scr, wGetHeadForWindow(wwin));
	totalArea.x1 = rect.pos.x;
	totalArea.y1 = rect.pos.y;
	totalArea.x2 = totalArea.x1 + rect.size.width;
	totalArea.y2 = totalArea.y1 + rect.size.height;
	
	usableArea.x1 = WMAX(totalArea.x1, usableArea.x1);
	usableArea.y1 = WMAX(totalArea.y1, usableArea.y1);
	usableArea.x2 = WMIN(totalArea.x2, usableArea.x2);
	usableArea.y2 = WMIN(totalArea.y2, usableArea.y2);
    }
#endif /* XINERAMA */

    if (WFLAGP(wwin, full_maximize)) {
	usableArea = totalArea;
    }

    if (wwin->flags.shaded) {
	wwin->flags.skip_next_animation = 1;
	wUnshadeWindow(wwin);
    }
    wwin->flags.maximized = directions;
    wwin->old_geometry.width = wwin->client.width;
    wwin->old_geometry.height = wwin->client.height;
    wwin->old_geometry.x = wwin->frame_x;
    wwin->old_geometry.y = wwin->frame_y;

#ifdef KWM_HINTS
    wKWMUpdateClientGeometryRestore(wwin);
#endif

    if (directions & MAX_HORIZONTAL) {

	new_width = (usableArea.x2-usableArea.x1)-FRAME_BORDER_WIDTH*2;	
	new_x = usableArea.x1;

    } else {

	new_x = wwin->frame_x;
	new_width = wwin->frame->core->width;

    }

    if (directions & MAX_VERTICAL) {

	new_height = (usableArea.y2-usableArea.y1)-FRAME_BORDER_WIDTH*2;
	new_y = usableArea.y1;
	if (WFLAGP(wwin, full_maximize)) {
	    new_y -= wwin->frame->top_width;
	    new_height += wwin->frame->bottom_width - 1;
	}
    } else {
	new_y = wwin->frame_y;
	new_height = wwin->frame->core->height;
    }

    if (!WFLAGP(wwin, full_maximize)) {
	new_height -= wwin->frame->top_width+wwin->frame->bottom_width;
    }

    wWindowConstrainSize(wwin, &new_width, &new_height);
    wWindowConfigure(wwin, new_x, new_y, new_width, new_height);

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMMaximizedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif

    wSoundPlay(WSOUND_MAXIMIZE);
}


void
wUnmaximizeWindow(WWindow *wwin)
{
    int restore_x, restore_y;

    if (!wwin->flags.maximized)
	return;
    
    if (wwin->flags.shaded) {
	wwin->flags.skip_next_animation = 1;
	wUnshadeWindow(wwin);
    }
    restore_x = (wwin->flags.maximized & MAX_HORIZONTAL) ?
        wwin->old_geometry.x : wwin->frame_x;
    restore_y = (wwin->flags.maximized & MAX_VERTICAL) ?
        wwin->old_geometry.y : wwin->frame_y;
    wwin->flags.maximized = 0;
    wWindowConfigure(wwin, restore_x, restore_y,
		     wwin->old_geometry.width, wwin->old_geometry.height);

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMMaximizedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif

    wSoundPlay(WSOUND_UNMAXIMIZE);
}

#ifdef ANIMATIONS
static void
animateResizeFlip(WScreen *scr, int x, int y, int w, int h,
                  int fx, int fy, int fw, int fh, int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES_F)
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    XPoint points[5]; 
    float dx, dch, midy;
    float angle, final_angle, delta;

    xstep = (float)(fx-x)/steps;
    ystep = (float)(fy-y)/steps;
    wstep = (float)(fw-w)/steps;
    hstep = (float)(fh-h)/steps;
     
    cx = (float)x;
    cy = (float)y;
    cw = (float)w;
    ch = (float)h;

    final_angle = 2*WM_PI*MINIATURIZE_ANIMATION_TWIST_F;
    delta = (float)(final_angle/FRAMES);
    for (angle=0;; angle+=delta) {
        if (angle > final_angle)
            angle = final_angle;

        dx = (cw/10) - ((cw/5) * sin(angle));
        dch = (ch/2) * cos(angle);
        midy = cy + (ch/2);

        points[0].x = cx + dx;      points[0].y = midy - dch;
        points[1].x = cx + cw - dx; points[1].y = points[0].y;
        points[2].x = cx + cw + dx; points[2].y = midy + dch;
        points[3].x = cx - dx;      points[3].y = points[2].y;
        points[4].x = points[0].x;  points[4].y = points[0].y;

        XGrabServer(dpy);
        XDrawLines(dpy,scr->root_win,scr->frame_gc,points, 5, CoordModeOrigin);
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_F > 0)
        wusleep(MINIATURIZE_ANIMATION_DELAY_F);
#endif

	XDrawLines(dpy,scr->root_win,scr->frame_gc,points, 5, CoordModeOrigin);
	XUngrabServer(dpy);
	cx+=xstep;
	cy+=ystep;
	cw+=wstep;
	ch+=hstep;
	if (angle >= final_angle)
	  break;

    }
    XFlush(dpy);
}
#undef FRAMES


static void 
animateResizeTwist(WScreen *scr, int x, int y, int w, int h,
                   int fx, int fy, int fw, int fh, int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES_T)
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    XPoint points[5]; 
    float angle, final_angle, a, d, delta;

    x += w/2;
    y += h/2;
    fx += fw/2;
    fy += fh/2;

    xstep = (float)(fx-x)/steps;
    ystep = (float)(fy-y)/steps;
    wstep = (float)(fw-w)/steps;
    hstep = (float)(fh-h)/steps;
     
    cx = (float)x;
    cy = (float)y;
    cw = (float)w;
    ch = (float)h;

    final_angle = 2*WM_PI*MINIATURIZE_ANIMATION_TWIST_T;
    delta =  (float)(final_angle/FRAMES);
    for (angle=0;; angle+=delta) {
        if (angle > final_angle)
            angle = final_angle;

        a = atan(ch/cw);
        d = sqrt((cw/2)*(cw/2)+(ch/2)*(ch/2));

        points[0].x = cx+cos(angle-a)*d;
        points[0].y = cy+sin(angle-a)*d;
        points[1].x = cx+cos(angle+a)*d;
        points[1].y = cy+sin(angle+a)*d;
        points[2].x = cx+cos(angle-a+WM_PI)*d;
        points[2].y = cy+sin(angle-a+WM_PI)*d;
        points[3].x = cx+cos(angle+a+WM_PI)*d;
        points[3].y = cy+sin(angle+a+WM_PI)*d;
        points[4].x = cx+cos(angle-a)*d;
        points[4].y = cy+sin(angle-a)*d;
        XGrabServer(dpy);
        XDrawLines(dpy, scr->root_win, scr->frame_gc, points, 5, CoordModeOrigin);
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_T > 0)
        wusleep(MINIATURIZE_ANIMATION_DELAY_T);
#endif

        XDrawLines(dpy, scr->root_win, scr->frame_gc, points, 5, CoordModeOrigin);
	XUngrabServer(dpy);
	cx+=xstep;
	cy+=ystep;
	cw+=wstep;
	ch+=hstep;
	if (angle >= final_angle)
	  break;
 
    }
    XFlush(dpy);
}
#undef FRAMES


static void 
animateResizeZoom(WScreen *scr, int x, int y, int w, int h,
                  int fx, int fy, int fw, int fh, int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES_Z)
    float cx[FRAMES], cy[FRAMES], cw[FRAMES], ch[FRAMES];
    float xstep, ystep, wstep, hstep;
    int i, j;

    xstep = (float)(fx-x)/steps;
    ystep = (float)(fy-y)/steps;
    wstep = (float)(fw-w)/steps;
    hstep = (float)(fh-h)/steps;
    
    for (j=0; j<FRAMES; j++) {
	cx[j] = (float)x;
	cy[j] = (float)y;
	cw[j] = (float)w;
	ch[j] = (float)h;
    }
    XGrabServer(dpy);
    for (i=0; i<steps; i++) {
	for (j=0; j<FRAMES; j++) {
	    XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
			   (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
	}
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_Z > 0)
        wusleep(MINIATURIZE_ANIMATION_DELAY_Z);
#endif
	for (j=0; j<FRAMES; j++) {
	    XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
			   (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
	    if (j<FRAMES-1) {
		cx[j]=cx[j+1];
		cy[j]=cy[j+1];
		cw[j]=cw[j+1];
		ch[j]=ch[j+1];
	    } else {
		cx[j]+=xstep;
		cy[j]+=ystep;
		cw[j]+=wstep;
		ch[j]+=hstep;
	    }
	}
    }

    for (j=0; j<FRAMES; j++) {
	XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
		       (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
    }
    XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY_Z > 0)
    wusleep(MINIATURIZE_ANIMATION_DELAY_Z);
#endif
    for (j=0; j<FRAMES; j++) {
	XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
		       (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
    }
    
    XUngrabServer(dpy);
}
#undef FRAMES


static void 
animateResize(WScreen *scr, int x, int y, int w, int h, 
	      int fx, int fy, int fw, int fh, int hiding)
{
    int style = wPreferences.iconification_style; /* Catch the value */
    int steps, k;

    if (style == WIS_NONE)
        return;

    if (style == WIS_RANDOM) {
	style = rand()%3;
    }

    k = (hiding ? 2 : 3);
    switch(style) {
    case WIS_TWIST:
        steps = (MINIATURIZE_ANIMATION_STEPS_T * k)/3;
        if (steps>0)
            animateResizeTwist(scr, x, y, w, h, fx, fy, fw, fh, steps);
        break;
    case WIS_FLIP:
        steps = (MINIATURIZE_ANIMATION_STEPS_F * k)/3;
        if (steps>0)
            animateResizeFlip(scr, x, y, w, h, fx, fy, fw, fh, steps);
        break;
    case WIS_ZOOM:
    default:
        steps = (MINIATURIZE_ANIMATION_STEPS_Z * k)/3;
        if (steps>0)
            animateResizeZoom(scr, x, y, w, h, fx, fy, fw, fh, steps);
        break;
    }
}
#endif /* ANIMATIONS */


static void
flushExpose()
{
    XEvent tmpev;
    
    while (XCheckTypedEvent(dpy, Expose, &tmpev))
	WMHandleEvent(&tmpev);
    XSync(dpy, 0);
}

static void
unmapTransientsFor(WWindow *wwin)
{
    WWindow *tmp;
    

    tmp = wwin->screen_ptr->focused_window;
    while (tmp) {
	/* unmap the transients for this transient */
	if (tmp!=wwin && tmp->transient_for == wwin->client_win
	    && (tmp->flags.mapped || wwin->screen_ptr->flags.startup
		|| tmp->flags.shaded)) {
	    unmapTransientsFor(tmp);
	    tmp->flags.miniaturized = 1;
	    if (!tmp->flags.shaded) {
		wWindowUnmap(tmp);
	    } else {
		XUnmapWindow(dpy, tmp->frame->core->window);
	    }
	    /*
	    if (!tmp->flags.shaded)
	     */
		wClientSetState(tmp, IconicState, None);
#ifdef KWM_HINTS
	    wKWMUpdateClientStateHint(tmp, KWMIconifiedFlag);
	    wKWMSendEventMessage(tmp, WKWMRemoveWindow);
	    tmp->flags.kwm_hidden_for_modules = 1;
#endif

	    UpdateSwitchMenu(wwin->screen_ptr, tmp, ACTION_CHANGE_STATE);

	}
	tmp = tmp->prev;
    }
}


static void
mapTransientsFor(WWindow *wwin)
{
    WWindow *tmp;

    tmp = wwin->screen_ptr->focused_window;
    while (tmp) {
	/* recursively map the transients for this transient */
	if (tmp!=wwin && tmp->transient_for == wwin->client_win
	    && /*!tmp->flags.mapped*/ tmp->flags.miniaturized 
	    && tmp->icon==NULL) {
	    mapTransientsFor(tmp);
	    tmp->flags.miniaturized = 0;
	    if (!tmp->flags.shaded) {
		wWindowMap(tmp);
	    } else {
		XMapWindow(dpy, tmp->frame->core->window);
	    }
 	    tmp->flags.semi_focused = 0;
	    /*
	    if (!tmp->flags.shaded)
	     */
		wClientSetState(tmp, NormalState, None);
#ifdef KWM_HINTS
	    wKWMUpdateClientStateHint(tmp, KWMIconifiedFlag);
	    if (tmp->flags.kwm_hidden_for_modules) {
		wKWMSendEventMessage(tmp, WKWMAddWindow);
		tmp->flags.kwm_hidden_for_modules = 0;
	    }
#endif

	    UpdateSwitchMenu(wwin->screen_ptr, tmp, ACTION_CHANGE_STATE);

	}
	tmp = tmp->prev;
    }
}

#if 0
static void
setupIconGrabs(WIcon *icon)
{
    /* setup passive grabs on the icon */
    XGrabButton(dpy, Button1, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button2, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button3, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XSync(dpy, 0);
}
#endif

static WWindow*
recursiveTransientFor(WWindow *wwin)
{
    int i;

    if (!wwin)
	return None;

    /* hackish way to detect transient_for cycle */ 
    i = wwin->screen_ptr->window_count+1;

    while (wwin && wwin->transient_for != None && i>0) {
	wwin = wWindowFor(wwin->transient_for);
	i--;
    }
    if (i==0 && wwin) {
	wwarning("%s has a severely broken WM_TRANSIENT_FOR hint.",
		 wwin->frame->title);
	return NULL;
    }

    return wwin;
}

#if 0
static void
removeIconGrabs(WIcon *icon)
{
    /* remove passive grabs on the icon */
    XUngrabButton(dpy, Button1, AnyModifier, icon->core->window);
    XUngrabButton(dpy, Button2, AnyModifier, icon->core->window);
    XUngrabButton(dpy, Button3, AnyModifier, icon->core->window);    
    XSync(dpy, 0);
}
#endif


void
wIconifyWindow(WWindow *wwin)
{
    XWindowAttributes attribs;
    int present;


    if (!XGetWindowAttributes(dpy, wwin->client_win, &attribs)) {
	/* the window doesn't exist anymore */
	return;
    }

    if (wwin->flags.miniaturized) {
	return;
    }

    
    if (wwin->transient_for!=None) {
	WWindow *owner = wWindowFor(wwin->transient_for);
	
	if (owner && owner->flags.miniaturized)
	    return;
    }
    
    present = wwin->frame->workspace==wwin->screen_ptr->current_workspace;
 
    /* if the window is in another workspace, simplify process */
    if (present) {
	/* icon creation may take a while */
	XGrabPointer(dpy, wwin->screen_ptr->root_win, False, 
		     ButtonMotionMask|ButtonReleaseMask, GrabModeAsync, 
		     GrabModeAsync, None, None, CurrentTime);
    }

    if (!wPreferences.disable_miniwindows) {
	if (!wwin->flags.icon_moved) {
	    PlaceIcon(wwin->screen_ptr, &wwin->icon_x, &wwin->icon_y);
	}
	wwin->icon = wIconCreate(wwin);

	wwin->icon->mapped = 1;
    }

    wwin->flags.miniaturized = 1;
    wwin->flags.mapped = 0;

    /* unmap transients */
    
    unmapTransientsFor(wwin);

    if (present) {
	wSoundPlay(WSOUND_ICONIFY);

	XUngrabPointer(dpy, CurrentTime);
	wWindowUnmap(wwin);
	/* let all Expose events arrive so that we can repaint
	 * something before the animation starts (and the server is grabbed) */
	XSync(dpy, 0);

	if (wPreferences.disable_miniwindows)
	    wClientSetState(wwin, IconicState, None);
	else
	    wClientSetState(wwin, IconicState, wwin->icon->icon_win);

	flushExpose();
#ifdef ANIMATIONS
	if (!wwin->screen_ptr->flags.startup && !wwin->flags.skip_next_animation
	    && !wPreferences.no_animations) {
	    int ix, iy, iw, ih;

	    if (!wPreferences.disable_miniwindows) {
		ix = wwin->icon_x;
		iy = wwin->icon_y;
		iw = wwin->icon->core->width;
		ih = wwin->icon->core->height;
	    } else {
#ifdef KWM_HINTS
		WArea area;

		if (wKWMGetIconGeometry(wwin, &area)) {
		    ix = area.x1;
		    iy = area.y1;
		    iw = area.x2 - ix;
		    ih = area.y2 - iy;
		} else
#endif /* KWM_HINTS */
		{
		    ix = 0;
		    iy = 0;
		    iw = wwin->screen_ptr->scr_width;
		    ih = wwin->screen_ptr->scr_height;
		}
	    }
	    animateResize(wwin->screen_ptr, wwin->frame_x, wwin->frame_y, 
			  wwin->frame->core->width, wwin->frame->core->height,
			  ix, iy, iw, ih, False);
	}
#endif
    }
    
    wwin->flags.skip_next_animation = 0;

    if (!wPreferences.disable_miniwindows) {

	if (wwin->screen_ptr->current_workspace==wwin->frame->workspace ||
	    IS_OMNIPRESENT(wwin) || wPreferences.sticky_icons)

	    XMapWindow(dpy, wwin->icon->core->window);

	AddToStackList(wwin->icon->core);

	wLowerFrame(wwin->icon->core);
    }

    if (present) {
	WWindow *owner = recursiveTransientFor(wwin->screen_ptr->focused_window);

/*
 * It doesn't seem to be working and causes button event hangup 
 * when deiconifying a transient window.
 setupIconGrabs(wwin->icon);
 */
	if ((wwin->flags.focused 
	     || (owner && wwin->client_win == owner->client_win))
	    && wPreferences.focus_mode==WKF_CLICK) {
	    WWindow *tmp;
	    
	    tmp = wwin->prev;
	    while (tmp) {
		if (!WFLAGP(tmp, no_focusable)
		    && !(tmp->flags.hidden||tmp->flags.miniaturized)
		    && (wwin->frame->workspace == tmp->frame->workspace))
		    break;
		tmp = tmp->prev;
	    }
	    wSetFocusTo(wwin->screen_ptr, tmp);
	} else if (wPreferences.focus_mode!=WKF_CLICK) {
	    wSetFocusTo(wwin->screen_ptr, NULL);
	}

#ifdef ANIMATIONS
	if (!wwin->screen_ptr->flags.startup) {
	    Window clientwin = wwin->client_win;

	    XSync(dpy, 0);
	    processEvents(XPending(dpy));
	    
	    /* the window can disappear while doing the processEvents() */
	    if (!wWindowFor(clientwin))
		return;
	}
#endif
    }


    if (wwin->flags.selected && !wPreferences.disable_miniwindows)
        wIconSelect(wwin->icon);

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMIconifiedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);
}




void
wDeiconifyWindow(WWindow  *wwin)
{
    wWindowChangeWorkspace(wwin, wwin->screen_ptr->current_workspace);

    if (!wwin->flags.miniaturized)
	return;

    if (wwin->transient_for != None 
	&& wwin->transient_for != wwin->screen_ptr->root_win) {
	WWindow *owner = recursiveTransientFor(wwin);

	if (owner && owner->flags.miniaturized) {
	    wDeiconifyWindow(owner);
	    wSetFocusTo(wwin->screen_ptr, wwin);
	    wRaiseFrame(wwin->frame->core);
	    return;
	}
    }

    wwin->flags.miniaturized = 0;
    if (!wwin->flags.shaded)
	wwin->flags.mapped = 1;

    if (!wPreferences.disable_miniwindows) {
	if (wwin->icon->selected)
	    wIconSelect(wwin->icon);

	XUnmapWindow(dpy, wwin->icon->core->window);
    }

    wSoundPlay(WSOUND_DEICONIFY);

    /* if the window is in another workspace, do it silently */
#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations
	&& !wwin->flags.skip_next_animation) {
	int ix, iy, iw, ih;

	if (!wPreferences.disable_miniwindows) {
	    ix = wwin->icon_x;
	    iy = wwin->icon_y;
	    iw = wwin->icon->core->width;
	    ih = wwin->icon->core->height;
	} else {
#ifdef KWM_HINTS
	    WArea area;

	    if (wKWMGetIconGeometry(wwin, &area)) {
		ix = area.x1;
		iy = area.y1;
		iw = area.x2 - ix;
		ih = area.y2 - iy;
	    } else
#endif /* KWM_HINTS */
	    {
		ix = 0;
		iy = 0;
		iw = wwin->screen_ptr->scr_width;
		ih = wwin->screen_ptr->scr_height;
	    }
	}
	animateResize(wwin->screen_ptr, ix, iy, iw, ih,
		      wwin->frame_x, wwin->frame_y,
		      wwin->frame->core->width, wwin->frame->core->height,
		      False);
    }
#endif /* ANIMATIONS */
    wwin->flags.skip_next_animation = 0;
    XGrabServer(dpy);
    if (!wwin->flags.shaded) {
	XMapWindow(dpy, wwin->client_win);
    }
    XMapWindow(dpy, wwin->frame->core->window);
    wRaiseFrame(wwin->frame->core);
    if (!wwin->flags.shaded) {
	wClientSetState(wwin, NormalState, None);
    }
    mapTransientsFor(wwin);

    if (!wPreferences.disable_miniwindows) {
	RemoveFromStackList(wwin->icon->core);
	/*    removeIconGrabs(wwin->icon);*/
	wIconDestroy(wwin->icon);
	wwin->icon = NULL;
    }
    XUngrabServer(dpy);

    if (wPreferences.focus_mode==WKF_CLICK)
	wSetFocusTo(wwin->screen_ptr, wwin);

#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup) {
	Window clientwin = wwin->client_win;
	
    	XSync(dpy, 0);
    	processEvents(XPending(dpy));
	
	if (!wWindowFor(clientwin))
	    return;
    }
#endif
    
    if (wPreferences.auto_arrange_icons) {
        wArrangeIcons(wwin->screen_ptr, True);
    }

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif
#ifdef KWM_HINTS
    wKWMUpdateClientStateHint(wwin, KWMIconifiedFlag);
    wKWMSendEventMessage(wwin, WKWMChangedClient);
#endif

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);
}



static void
hideWindow(WIcon *icon, int icon_x, int icon_y, WWindow *wwin, int animate)
{
    if (wwin->flags.miniaturized) {
	if (wwin->icon) { 
	    XUnmapWindow(dpy, wwin->icon->core->window);
	    wwin->icon->mapped = 0;
	}
	wwin->flags.hidden = 1;
#ifdef GNOME_STUFF
	wGNOMEUpdateClientStateHint(wwin, False);
#endif

	UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);

	return;
    }

    if (wwin->flags.inspector_open) {
	wHideInspectorForWindow(wwin);
    }

    wwin->flags.hidden = 1;
    wWindowUnmap(wwin);

    wClientSetState(wwin, IconicState, icon->icon_win);
    flushExpose();
    wSoundPlay(WSOUND_HIDE);
#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations &&
	!wwin->flags.skip_next_animation && animate) {
	animateResize(wwin->screen_ptr, wwin->frame_x, wwin->frame_y, 
		      wwin->frame->core->width, wwin->frame->core->height,
		      icon_x, icon_y, icon->core->width, icon->core->height,
		      True);
    }
#endif
    wwin->flags.skip_next_animation = 0;

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);
}



void
wHideOtherApplications(WWindow *awin)
{
    WWindow *wwin;
    WApplication *tapp;

    if (!awin)
	return;
    wwin = awin->screen_ptr->focused_window;


    while (wwin) {
	if (wwin!=awin 
	    && wwin->frame->workspace == awin->screen_ptr->current_workspace
            && !(wwin->flags.miniaturized||wwin->flags.hidden)
            && !wwin->flags.internal_window
	    && wGetWindowOfInspectorForWindow(wwin) != awin
	    && !WFLAGP(wwin, no_hide_others)) {
	    
	    if (wwin->main_window==None || WFLAGP(wwin, no_appicon)) {
		if (!WFLAGP(wwin, no_miniaturizable)) {
		    wwin->flags.skip_next_animation = 1;
		    wIconifyWindow(wwin);
		}
	    } else if (wwin->main_window!=None
		       && awin->main_window != wwin->main_window) {
		tapp = wApplicationOf(wwin->main_window);
		if (tapp) {
		    tapp->flags.skip_next_animation = 1;
		    wHideApplication(tapp);
		} else {
		    if (!WFLAGP(wwin, no_miniaturizable)) {
		    	wwin->flags.skip_next_animation = 1;
		   	 wIconifyWindow(wwin);
		    }
		}
	    }
	}
	wwin = wwin->prev;
    }
    /*
    wSetFocusTo(awin->screen_ptr, awin);
     */
}



void
wHideApplication(WApplication *wapp)
{
    WScreen *scr;
    WWindow *wlist;
    int hadfocus;
    
    if (!wapp) {
	wwarning("trying to hide a non grouped window");
	return;
    }
    if (!wapp->main_window_desc) {
	wwarning("group leader not found for window group");
	return;
    }
    scr = wapp->main_window_desc->screen_ptr;
    hadfocus = 0;
    wlist = scr->focused_window;
    if (!wlist)
	return;

    if (wlist->main_window == wapp->main_window)
	wapp->last_focused = wlist;
    else
	wapp->last_focused = NULL;
    while (wlist) {
	if (wlist->main_window == wapp->main_window) {
	    if (wlist->flags.focused) {
		hadfocus = 1;
            }
            if (wapp->app_icon)
                hideWindow(wapp->app_icon->icon, wapp->app_icon->x_pos,
                           wapp->app_icon->y_pos, wlist,
                           !wapp->flags.skip_next_animation);
	}
	wlist = wlist->prev;
    }
    
    wapp->flags.skip_next_animation = 0;    

    if (hadfocus) {
	if (wPreferences.focus_mode==WKF_CLICK) {
	    wlist = scr->focused_window;
	    while (wlist) {
		if (!WFLAGP(wlist, no_focusable) && !wlist->flags.hidden
		    && (wlist->flags.mapped || wlist->flags.shaded))
		  break;
		wlist = wlist->prev;
	    }
	    wSetFocusTo(scr, wlist);
	} else {
	    wSetFocusTo(scr, NULL);
	}
    }
    
    wapp->flags.hidden = 1;
    
    if(wPreferences.auto_arrange_icons) {
	wArrangeIcons(scr, True);
    }
#ifdef HIDDENDOT
    if (wapp->app_icon)
    	wAppIconPaint(wapp->app_icon);
#endif
}




static void
unhideWindow(WIcon *icon, int icon_x, int icon_y, WWindow *wwin, int animate,
	     int bringToCurrentWS)
{
    if (bringToCurrentWS)
	wWindowChangeWorkspace(wwin, wwin->screen_ptr->current_workspace);

    wwin->flags.hidden=0;
    wwin->flags.mapped=1;
    
    wSoundPlay(WSOUND_UNHIDE);
#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations 
	&& animate) {
	animateResize(wwin->screen_ptr, icon_x, icon_y,
		      icon->core->width, icon->core->height,
		      wwin->frame_x, wwin->frame_y, 
		      wwin->frame->core->width, wwin->frame->core->height,
                      True);
    }
#endif
    wwin->flags.skip_next_animation = 0;
    XMapWindow(dpy, wwin->client_win);
    XMapWindow(dpy, wwin->frame->core->window);
    wClientSetState(wwin, NormalState, None);
    wRaiseFrame(wwin->frame->core);
    if (wwin->flags.inspector_open) {
	wUnhideInspectorForWindow(wwin);
    }

#ifdef GNOME_STUFF
    wGNOMEUpdateClientStateHint(wwin, False);
#endif

    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_STATE);

}


void
wUnhideApplication(WApplication *wapp, Bool miniwindows, Bool bringToCurrentWS)
{
    WScreen *scr;
    WWindow *wlist, *next;
    WWindow *focused=NULL;

    if (!wapp) {
	return;
    }
    scr = wapp->main_window_desc->screen_ptr;
    wlist = scr->focused_window;
    if (!wlist) return;
    /* goto beginning of list */
    while (wlist->prev)
      wlist = wlist->prev;
    
    while (wlist) {
	next = wlist->next;

	if (wlist->main_window == wapp->main_window) {
	    if (wlist->flags.focused)
	      focused = wlist;
	    else if (!focused || !focused->flags.focused)
	      focused = wlist;
	    
	    if (wlist->flags.miniaturized && wlist->icon) {
                if (bringToCurrentWS || wPreferences.sticky_icons
		    || wlist->frame->workspace == scr->current_workspace) {
		    if (!wlist->icon->mapped) {
			XMapWindow(dpy, wlist->icon->core->window);
			wlist->icon->mapped = 1;
		    }
		    wlist->flags.hidden = 0;

		    UpdateSwitchMenu(scr, wlist, ACTION_CHANGE_STATE);

		    if (wlist->frame->workspace != scr->current_workspace)
			wWindowChangeWorkspace(wlist, scr->current_workspace);
		}
		if (miniwindows) {
		    wDeiconifyWindow(wlist);
		}
	    } else if (wlist->flags.hidden) {
		unhideWindow(wapp->app_icon->icon, wapp->app_icon->x_pos,
			     wapp->app_icon->y_pos, wlist, 
			     !wapp->flags.skip_next_animation,
			     bringToCurrentWS);
	    } else {
		if (bringToCurrentWS
		    && wlist->frame->workspace != scr->current_workspace) {
		    wWindowChangeWorkspace(wlist, scr->current_workspace);
		}
		wRaiseFrame(wlist->frame->core);
	    }
	}
	wlist = next;
    }
    
    wapp->flags.skip_next_animation = 0;
    wapp->flags.hidden = 0;
    
    if (focused)
	wSetFocusTo(scr, focused);
    else if (wapp->last_focused && wapp->last_focused->flags.mapped)
	wSetFocusTo(scr, wapp->last_focused);
    wapp->last_focused = NULL;
    if (wPreferences.auto_arrange_icons) {
        wArrangeIcons(scr, True);
    }
#ifdef HIDDENDOT
    wAppIconPaint(wapp->app_icon);
#endif 
}



void
wShowAllWindows(WScreen *scr)
{
    WWindow *wwin, *old_foc;
    WApplication *wapp;

    old_foc = wwin = scr->focused_window;
    while (wwin) {
        if (!wwin->flags.internal_window &&
            (scr->current_workspace == wwin->frame->workspace
	    || IS_OMNIPRESENT(wwin))) {
	    if (wwin->flags.miniaturized) {
		wwin->flags.skip_next_animation = 1;
		wDeiconifyWindow(wwin);
	    } else if (wwin->flags.hidden) {
		wapp = wApplicationOf(wwin->main_window);
		if (wapp) {
		    wUnhideApplication(wapp, False, False);
		} else {
		    wwin->flags.skip_next_animation = 1;
		    wDeiconifyWindow(wwin);
		}
	    }
	}
	wwin = wwin->prev;
    }
    wSetFocusTo(scr, old_foc);
    /*wRaiseFrame(old_foc->frame->core);*/
}


void
wRefreshDesktop(WScreen *scr)
{
    Window win;
    XSetWindowAttributes attr;

    attr.backing_store = NotUseful;
    attr.save_under = False;
    win = XCreateWindow(dpy, scr->root_win, 0, 0, scr->scr_width, 
			scr->scr_height, 0, CopyFromParent, CopyFromParent,
			(Visual *)CopyFromParent, CWBackingStore|CWSaveUnder,
			&attr);
    XMapRaised(dpy, win);
    XDestroyWindow(dpy, win);
    XFlush(dpy);
}


void
wArrangeIcons(WScreen *scr, Bool arrangeAll)
{
    WWindow *wwin;
    WAppIcon *aicon;
    int pf;			       /* primary axis */
    int sf;			       /* secondary axis */
    int fullW;
    int fullH;
    int pi, si;
    int sx1, sx2, sy1, sy2;	       /* screen boundary */
    int sw, sh;
    int xo, yo;
    int xs, ys;
    int isize = wPreferences.icon_size;

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

    /* arrange icons putting the most recently focused window
     * as the last icon */
#define X ((wPreferences.icon_yard & IY_VERT) ? xo + xs*(si*isize)\
		    : xo + xs*(pi*isize))
#define Y ((wPreferences.icon_yard & IY_VERT) ? yo + ys*(pi*isize)\
		    : yo + ys*(si*isize))

    /* arrange application icons */
    aicon = scr->app_icon_list;
    /* reverse them to avoid unnecessarily sliding of icons */
    while (aicon && aicon->next)
        aicon = aicon->next;

    pi = 0;
    si = 0;
    while (aicon) {	
        if (!aicon->docked && wAppIconIsFirstInstance(aicon)) {
            if (aicon->x_pos != X || aicon->y_pos != Y) {
#ifdef ANIMATIONS
                if (!wPreferences.no_animations) {
                    SlideWindow(aicon->icon->core->window, aicon->x_pos, aicon->y_pos,
                                X, Y);
                }
#endif /* ANIMATIONS */
            }	    
	    wAppIconMove(aicon, X, Y);
            pi++;
        }
        /* we reversed the order so we use prev */
        aicon = aicon->prev;
        if (pi >= pf) {
            pi=0;
            si++;
        }
    }
    
    /* arrange miniwindows */

    wwin = scr->focused_window;
    /* reverse them to avoid unnecessarily shuffling */
    while (wwin && wwin->prev)
        wwin = wwin->prev;

    while (wwin) {
        if (wwin->icon && wwin->flags.miniaturized && !wwin->flags.hidden &&
            (wwin->frame->workspace==scr->current_workspace ||
             IS_OMNIPRESENT(wwin) || wPreferences.sticky_icons)) {
	    
	    if (arrangeAll || !wwin->flags.icon_moved) {
		if (wwin->icon_x != X || wwin->icon_y != Y) {
#ifdef ANIMATIONS
		    if (wPreferences.no_animations) {
			XMoveWindow(dpy, wwin->icon->core->window, X, Y);
		    } else {
			SlideWindow(wwin->icon->core->window, wwin->icon_x, 
				    wwin->icon_y, X, Y);
		    }
#else
		    XMoveWindow(dpy, wwin->icon->core->window, X, Y);
#endif /* ANIMATIONS */
		}
		wwin->icon_x = X;
		wwin->icon_y = Y;
		pi++;
	    }
	}
	if (arrangeAll) {
	    wwin->flags.icon_moved = 0;
	}
        /* we reversed the order, so we use next */
        wwin = wwin->next;
	if (pi >= pf) {
	    pi=0;
	    si++;
	}
    }
}


void
wSelectWindow(WWindow *wwin, Bool flag)
{
    WScreen *scr = wwin->screen_ptr;
    
    if (flag) {
	wwin->flags.selected = 1;
	XSetWindowBorder(dpy, wwin->frame->core->window, scr->white_pixel);
	
	if (WFLAGP(wwin, no_border)) {	
	    XSetWindowBorderWidth(dpy, wwin->frame->core->window, 
				  FRAME_BORDER_WIDTH);
	}

	if (!scr->selected_windows)
	    scr->selected_windows = WMCreateArray(4);
	WMAddToArray(scr->selected_windows, wwin);
    } else {
	wwin->flags.selected = 0;
	XSetWindowBorder(dpy, wwin->frame->core->window,
			 scr->frame_border_pixel);

	if (WFLAGP(wwin, no_border)) {	
	    XSetWindowBorderWidth(dpy, wwin->frame->core->window, 0);
	}

	if (scr->selected_windows) {
	    WMRemoveFromArray(scr->selected_windows, wwin);
	}
    }
}


void
wMakeWindowVisible(WWindow *wwin)
{
    if (wwin->frame->workspace != wwin->screen_ptr->current_workspace)
	wWorkspaceChange(wwin->screen_ptr, wwin->frame->workspace);

    if (wwin->flags.shaded) {
	wUnshadeWindow(wwin);
    }
    if (wwin->flags.hidden) {
	WApplication *app;

	app = wApplicationOf(wwin->main_window);
	if (app)
	    wUnhideApplication(app, False, False);
    } else if (wwin->flags.miniaturized) {
	wDeiconifyWindow(wwin);
    } else {
	if (!WFLAGP(wwin, no_focusable))
	    wSetFocusTo(wwin->screen_ptr, wwin);
	wRaiseFrame(wwin->frame->core);
    }
}
