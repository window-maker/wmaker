/* gnome.c-- support for the GNOME Hints
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1998-2002 Alfredo K. Kojima
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

/*
 * According to the author of this thing, it should not be taken seriously.
 * IMHO, there are lot's of weirdnesses and it's quite unelegant. I'd
 * rather not support it, but here it goes anyway. 
 */

#include "wconfig.h"

#ifdef GNOME_STUFF

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "WindowMaker.h"
#include "screen.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "workspace.h"
#include "funcs.h"
#include "actions.h"
#include "stacking.h"
#include "client.h"

#include "gnome.h"






#define WIN_HINTS_SKIP_FOCUS      (1<<0) /*"alt-tab" skips this win*/
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /*do not show in window list*/
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /*do not show on taskbar*/
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /*Reserved - definition is unclear*/
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /*app only accepts focus if clicked*/
#define WIN_HINTS_DO_NOT_COVER    (1<<5) /* attempt to not cover this window */


#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
/* these are bogus states defined in "the spec" */
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/


#define WIN_LAYER_DESKTOP                0
#define WIN_LAYER_BELOW                  2
#define WIN_LAYER_NORMAL                 4
#define WIN_LAYER_ONTOP                  6
#define WIN_LAYER_DOCK                   8
#define WIN_LAYER_ABOVE_DOCK             10
#define WIN_LAYER_MENU                   12



static Atom _XA_WIN_SUPPORTING_WM_CHECK = 0;
static Atom _XA_WIN_PROTOCOLS;
static Atom _XA_WIN_LAYER;
static Atom _XA_WIN_STATE;
static Atom _XA_WIN_HINTS;
static Atom _XA_WIN_APP_STATE;
static Atom _XA_WIN_EXPANDED_SIZE;
static Atom _XA_WIN_ICONS;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_WORKSPACE_NAMES;
static Atom _XA_WIN_CLIENT_LIST;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;


static void observer(void *self, WMNotification *notif);
static void wsobserver(void *self, WMNotification *notif);


void
wGNOMEInitStuff(WScreen *scr)
{
    Atom supportedStuff[10];
    int count;

    if (!_XA_WIN_SUPPORTING_WM_CHECK) {

	_XA_WIN_SUPPORTING_WM_CHECK = 
	    XInternAtom(dpy, "_WIN_SUPPORTING_WM_CHECK", False);

	_XA_WIN_PROTOCOLS = XInternAtom(dpy, "_WIN_PROTOCOLS", False);

	_XA_WIN_LAYER = XInternAtom(dpy, "_WIN_LAYER", False);

	_XA_WIN_STATE = XInternAtom(dpy, "_WIN_STATE", False);

	_XA_WIN_HINTS = XInternAtom(dpy, "_WIN_HINTS", False);

	_XA_WIN_APP_STATE = XInternAtom(dpy, "_WIN_APP_STATE", False);

	_XA_WIN_EXPANDED_SIZE = XInternAtom(dpy, "_WIN_EXPANDED_SIZE", False);

	_XA_WIN_ICONS = XInternAtom(dpy, "_WIN_ICONS", False);

	_XA_WIN_WORKSPACE = XInternAtom(dpy, "_WIN_WORKSPACE", False);

	_XA_WIN_WORKSPACE_COUNT = 
	    XInternAtom(dpy, "_WIN_WORKSPACE_COUNT", False);

	_XA_WIN_WORKSPACE_NAMES = 
	    XInternAtom(dpy, "_WIN_WORKSPACE_NAMES", False);

	_XA_WIN_CLIENT_LIST = XInternAtom(dpy, "_WIN_CLIENT_LIST", False);

	_XA_WIN_DESKTOP_BUTTON_PROXY = 
	    XInternAtom(dpy, "_WIN_DESKTOP_BUTTON_PROXY", False);
    }

    /* I'd rather use the ICCCM 2.0 mechanisms, but
     * since some people prefer to reinvent the wheel instead of
     * conforming to standards... */

    /* setup the "We're compliant, you idiot!" hint */

    /* why XA_CARDINAL instead of XA_WINDOW? */
    XChangeProperty(dpy, scr->root_win, _XA_WIN_SUPPORTING_WM_CHECK, 
		    XA_CARDINAL, 32, PropModeReplace, 
		    (unsigned char*)&scr->no_focus_win, 1);

    XChangeProperty(dpy, scr->no_focus_win, _XA_WIN_SUPPORTING_WM_CHECK, 
		    XA_CARDINAL, 32, PropModeReplace, 
		    (unsigned char*)&scr->no_focus_win, 1);

    
    /* setup the "desktop button proxy" thing */
    XChangeProperty(dpy, scr->root_win, _XA_WIN_DESKTOP_BUTTON_PROXY,
		    XA_CARDINAL, 32, PropModeReplace,
		    (unsigned char*)&scr->no_focus_win, 1);
    XChangeProperty(dpy, scr->no_focus_win, _XA_WIN_DESKTOP_BUTTON_PROXY,
		    XA_CARDINAL, 32, PropModeReplace,
		    (unsigned char*)&scr->no_focus_win, 1);
    

    /* setup the list of supported protocols */
    count = 0;
    supportedStuff[count++] = _XA_WIN_LAYER;
    supportedStuff[count++] = _XA_WIN_STATE;
    supportedStuff[count++] = _XA_WIN_HINTS;
    supportedStuff[count++] = _XA_WIN_APP_STATE;
    supportedStuff[count++] = _XA_WIN_EXPANDED_SIZE;
    supportedStuff[count++] = _XA_WIN_ICONS;
    supportedStuff[count++] = _XA_WIN_WORKSPACE;
    supportedStuff[count++] = _XA_WIN_WORKSPACE_COUNT;
    supportedStuff[count++] = _XA_WIN_WORKSPACE_NAMES;
    supportedStuff[count++] = _XA_WIN_CLIENT_LIST;

    XChangeProperty(dpy, scr->root_win, _XA_WIN_PROTOCOLS, XA_ATOM, 32,
		    PropModeReplace, (unsigned char*)supportedStuff, count);

    XFlush(dpy);

    WMAddNotificationObserver(observer, NULL, WMNManaged, NULL);
    WMAddNotificationObserver(observer, NULL, WMNUnmanaged, NULL);
    WMAddNotificationObserver(observer, NULL, WMNChangedWorkspace, NULL);
    WMAddNotificationObserver(observer, NULL, WMNChangedState, NULL);
    WMAddNotificationObserver(observer, NULL, WMNChangedFocus, NULL);
    WMAddNotificationObserver(observer, NULL, WMNChangedStacking, NULL);
    WMAddNotificationObserver(observer, NULL, WMNChangedName, NULL);

    WMAddNotificationObserver(wsobserver, NULL, WMNWorkspaceCreated, NULL);
    WMAddNotificationObserver(wsobserver, NULL, WMNWorkspaceDestroyed, NULL);
    WMAddNotificationObserver(wsobserver, NULL, WMNWorkspaceChanged, NULL);
    WMAddNotificationObserver(wsobserver, NULL, WMNWorkspaceNameChanged, NULL);
}


void
wGNOMEUpdateClientListHint(WScreen *scr)
{
    WWindow *wwin;
    Window *windows;
    int count;

    windows = malloc(sizeof(Window)*scr->window_count);
    if (!windows) {
	wwarning(_("out of memory while updating GNOME hints"));
	return;
    }

    count = 0;
    wwin = scr->focused_window;
    while (wwin) {
        if (!wwin->flags.internal_window &&
            !wwin->client_flags.skip_window_list) {

	    windows[count++] = wwin->client_win;
	}

	wwin = wwin->prev;
    }

    XChangeProperty(dpy, scr->root_win, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32,
		    PropModeReplace, (unsigned char *)windows, count);

    wfree(windows);
    XFlush(dpy);
}


void
wGNOMEUpdateWorkspaceHints(WScreen *scr)
{
    long val;

    val = scr->workspace_count;

    XChangeProperty(dpy, scr->root_win, _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL,
		    32, PropModeReplace, (unsigned char*)&val, 1);

    wGNOMEUpdateWorkspaceNamesHint(scr);
}


void
wGNOMEUpdateWorkspaceNamesHint(WScreen *scr)
{
    char *wsNames[MAX_WORKSPACES];
    XTextProperty textProp;
    int i;

    for (i = 0; i < scr->workspace_count; i++) {
	wsNames[i] = scr->workspaces[i]->name;
    }

    if (XStringListToTextProperty(wsNames, scr->workspace_count, &textProp)) {
	XSetTextProperty(dpy, scr->root_win, &textProp,
			 _XA_WIN_WORKSPACE_NAMES);
	XFree(textProp.value);
    }
}


void
wGNOMEUpdateCurrentWorkspaceHint(WScreen *scr)
{
    long val;

    val = scr->current_workspace;

    XChangeProperty(dpy, scr->root_win, _XA_WIN_WORKSPACE, XA_CARDINAL,
		    32, PropModeReplace, (unsigned char*)&val, 1);
}


static int
getWindowLevel(int layer)
{
    int level;

    if (layer <= WIN_LAYER_DESKTOP)
	level = WMDesktopLevel;
    else if (layer <= WIN_LAYER_BELOW)
	level = WMSunkenLevel;
    else if (layer <= WIN_LAYER_NORMAL)
	level = WMNormalLevel;
    else if (layer <= WIN_LAYER_ONTOP)
	level = WMFloatingLevel;
    else if (layer <= WIN_LAYER_DOCK)
	level = WMDockLevel;
    else if (layer <= WIN_LAYER_ABOVE_DOCK)
	level = WMSubmenuLevel;
    else if (layer <= WIN_LAYER_MENU)
	level = WMMainMenuLevel;
    else 
	level = WMOuterSpaceLevel;
    
    return level;
}


Bool
wGNOMECheckClientHints(WWindow *wwin, int *layer, int *workspace)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    long flags, val, *data = 0;
    Bool hasHints = False;

    /* hints */
    
    if (XGetWindowProperty(dpy, wwin->client_win, _XA_WIN_HINTS, 0, 1, False,
			   /* should be XA_INTEGER, but spec is broken */
			   XA_CARDINAL, &type_ret, &fmt_ret, &nitems_ret, 
			   &bytes_after_ret,
			   (unsigned char**)&data)==Success && data) {
	flags = *data;
	
	XFree(data);
	
	if (flags & (WIN_HINTS_SKIP_FOCUS|WIN_HINTS_SKIP_WINLIST
				|WIN_HINTS_SKIP_TASKBAR)) {
	    wwin->client_flags.skip_window_list = 1;
	}

	/* client reserved area, for the panel */
	if (flags & (WIN_HINTS_DO_NOT_COVER)) {
		WReservedArea *area;

		area = malloc(sizeof(WReservedArea));
		if (!area) {
			wwarning(_("out of memory while updating GNOME hints"));
		} else {
			XWindowAttributes wattribs;

			XGetWindowAttributes(dpy, wwin->client_win, &wattribs);
			wClientGetNormalHints(wwin, &wattribs, False,
					      &area->area.x1, &area->area.y1,
					      &area->area.x2, &area->area.y2);
			area->area.x2 = area->area.x2 + area->area.x1;
			area->area.y2 = area->area.y2 + area->area.y1;

			area->window = wwin->client_win;
		}
		area->next = wwin->screen_ptr->reservedAreas;
		wwin->screen_ptr->reservedAreas = area;
		
		wScreenUpdateUsableArea(wwin->screen_ptr);
	}

	hasHints = True;
    }

    /* layer */
    if (XGetWindowProperty(dpy, wwin->client_win, _XA_WIN_LAYER, 0, 1, False,
			   XA_CARDINAL, &type_ret, &fmt_ret, &nitems_ret, 
			   &bytes_after_ret,
			   (unsigned char**)&data)==Success  && data) {
	val = *data;

	XFree(data);

	*layer = getWindowLevel(val);
	hasHints = True;
    }
    
    /* workspace */
    if (XGetWindowProperty(dpy, wwin->client_win, _XA_WIN_WORKSPACE, 0, 1, 
			   False, XA_CARDINAL, &type_ret, &fmt_ret, 
			   &nitems_ret, &bytes_after_ret,
			   (unsigned char**)&data)==Success && data) {
	val = *data;

	XFree(data);

	if (val > 0)
	    *workspace = val;
	hasHints = True;
    }

    /* reserved area */
    if (XGetWindowProperty(dpy, wwin->client_win, _XA_WIN_EXPANDED_SIZE, 0, 1, 
			   False, XA_CARDINAL, &type_ret, &fmt_ret, 
			   &nitems_ret, &bytes_after_ret,
			   (unsigned char**)&data)==Success && data) {
	WReservedArea *area;

	area = malloc(sizeof(WReservedArea));
	if (!area) {
	    wwarning(_("out of memory while updating GNOME hints"));
	} else {
	    area->area.x1 = data[0];
	    area->area.y1 = data[1];
	    area->area.x2 = data[2] - data[0];
	    area->area.y2 = data[3] - data[1];
	    XFree(data);

	    area->window = wwin->client_win;
	}

	area->next = wwin->screen_ptr->reservedAreas;
	wwin->screen_ptr->reservedAreas = area;

        wScreenUpdateUsableArea(wwin->screen_ptr);
	hasHints = True;
    }
    
    return hasHints;
}


Bool
wGNOMECheckInitialClientState(WWindow *wwin)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    long flags, *data = 0;

    if (XGetWindowProperty(dpy, wwin->client_win, _XA_WIN_STATE, 0, 1, False,
			   XA_CARDINAL, &type_ret, &fmt_ret, &nitems_ret, 
			   &bytes_after_ret,
			   (unsigned char**)&data)!=Success || !data)
	return False;

    flags = *data;

    XFree(data);

    if (flags & WIN_STATE_STICKY)
	wwin->client_flags.omnipresent = 1;

    if (flags & (WIN_STATE_MAXIMIZED_VERT|WIN_STATE_MAXIMIZED_HORIZ)) {

	if (flags & WIN_STATE_MAXIMIZED_VERT)
	    wwin->flags.maximized |= MAX_VERTICAL;

	if (flags & WIN_STATE_MAXIMIZED_HORIZ)
	    wwin->flags.maximized |= MAX_HORIZONTAL;
    }

    if (flags & WIN_STATE_SHADED)
	wwin->flags.shaded = 1;
    
    return True;
}


void
wGNOMEUpdateClientStateHint(WWindow *wwin, Bool changedWorkspace)
{
    long val;
    long flags = 0;

    if (changedWorkspace) {
	val = wwin->frame->workspace;

	XChangeProperty(dpy, wwin->client_win, _XA_WIN_WORKSPACE, XA_CARDINAL,
			32, PropModeReplace, (unsigned char*)&val, 1);

	if (val != wwin->screen_ptr->current_workspace)
	    flags |= WIN_STATE_HID_WORKSPACE;
    }

    if (IS_OMNIPRESENT(wwin))
	flags |= WIN_STATE_STICKY;

    if (wwin->flags.miniaturized)
	flags |= WIN_STATE_MINIMIZED;

    if (wwin->flags.maximized & MAX_VERTICAL)
	flags |= WIN_STATE_MAXIMIZED_VERT;

    if (wwin->flags.maximized & MAX_HORIZONTAL)
	flags |= WIN_STATE_MAXIMIZED_HORIZ;

    if (wwin->flags.shaded)
	flags |= WIN_STATE_SHADED;

    if (wwin->transient_for != None) {
	WWindow *owner = wWindowFor(wwin->transient_for);

	if (owner && !owner->flags.mapped)
	    flags |= WIN_STATE_HID_TRANSIENT;
    }

    /* ? */
    if (wwin->flags.hidden)
	flags |= WIN_STATE_HIDDEN;

    XChangeProperty(dpy, wwin->client_win, _XA_WIN_STATE, XA_CARDINAL,
		    32, PropModeReplace, (unsigned char*)&flags, 1);
}


Bool
wGNOMEProcessClientMessage(XClientMessageEvent *event)
{
    WScreen *scr;
    WWindow *wwin;
    Bool done = True;

    scr = wScreenForWindow(event->window);
    if (scr) {
	/* generic client messages */
	if (event->message_type == _XA_WIN_WORKSPACE) {
	    wWorkspaceChange(scr, event->data.l[0]);
	} else {
	    done = False;
	}

	if (done)
	    return True;
    }

    /* window specific client messages */    

    wwin = wWindowFor(event->window);
    if (!wwin)
	return False;
    
    if (event->message_type == _XA_WIN_LAYER) {
	int level = getWindowLevel(event->data.l[0]);

	if (WINDOW_LEVEL(wwin) != level) {
	    ChangeStackingLevel(wwin->frame->core, level);
	}
    } else if (event->message_type == _XA_WIN_STATE) {
	int flags, mask;
	int maximize = 0;

	mask = event->data.l[0];
	flags = event->data.l[1]; 

	if (mask & WIN_STATE_STICKY) {
	    if ((flags & WIN_STATE_STICKY) != WFLAGP(wwin, omnipresent)) {
		wWindowSetOmnipresent(wwin, (flags & WIN_STATE_STICKY)!=0);
	    }
	}

	if (mask & WIN_STATE_MAXIMIZED_VERT) {
	    if (flags & WIN_STATE_MAXIMIZED_VERT)
		maximize = MAX_VERTICAL;
	    else
		maximize = 0;
	} else {
	    maximize = wwin->flags.maximized & MAX_VERTICAL;
	}

	if (mask & WIN_STATE_MAXIMIZED_HORIZ) {
	    if (flags & WIN_STATE_MAXIMIZED_HORIZ)
		maximize |= MAX_HORIZONTAL;
	    else
		maximize |= 0;
	} else {
	    maximize |= wwin->flags.maximized & MAX_HORIZONTAL;
	}

	if (maximize != wwin->flags.maximized) {
#define both (MAX_HORIZONTAL|MAX_VERTICAL)
	    if (!(maximize & both) && (wwin->flags.maximized & both)) {
		wUnmaximizeWindow(wwin);
	    }
	    if ((maximize & both) && !(wwin->flags.maximized & both)) {
		wMaximizeWindow(wwin, maximize);
	    }
#undef both
	}

	if (mask & WIN_STATE_SHADED) {
	    if ((flags & WIN_STATE_SHADED) != wwin->flags.shaded) {
		if (wwin->flags.shaded)
		    wUnshadeWindow(wwin);
		else
		    wShadeWindow(wwin);
	    }
	}
    } else if (event->message_type == _XA_WIN_WORKSPACE) {

	if (event->data.l[0] != wwin->frame->workspace) {
	    wWindowChangeWorkspace(wwin, event->data.l[0]);
	}
    } else {
	done = False;
    }
    
    return done;
}


Bool
wGNOMEProxyizeButtonEvent(WScreen *scr, XEvent *event)
{
    if (event->type == ButtonPress)
	XUngrabPointer(dpy, CurrentTime);
    XSendEvent(dpy, scr->no_focus_win, False, SubstructureNotifyMask, event);

    return True;
}


void
wGNOMERemoveClient(WWindow *wwin)
{
    int flag = 0;
    WReservedArea *area;

    wGNOMEUpdateClientListHint(wwin->screen_ptr);
    
    area = wwin->screen_ptr->reservedAreas;

    if (area) {
	if (area->window == wwin->client_win) {
	    wwin->screen_ptr->reservedAreas = area->next;
	    wfree(area);
	    flag = 1;
	} else {
	    while (area->next && area->next->window != wwin->client_win) 
		area = area->next;
	
	    if (area->next) {
		WReservedArea *next;
		
		next = area->next->next;
		wfree(area->next);
		area->next = next;
		
		flag = 1;
	    }
	}
    }

    if (flag) {
	wScreenUpdateUsableArea(wwin->screen_ptr);
    }
}




static void observer(void *self, WMNotification *notif)
{
    WWindow *wwin = (WWindow*)WMGetNotificationObject(notif);
    const char *name = WMGetNotificationName(notif);

    if (strcmp(name, WMNManaged) == 0 && wwin) {
	wGNOMEUpdateClientStateHint(wwin, True);
	
	wGNOMEUpdateClientListHint(wwin->screen_ptr);
    } else if (strcmp(name, WMNUnmanaged) == 0 && wwin) {
	wGNOMERemoveClient(wwin);
    } else if (strcmp(name, WMNChangedWorkspace) == 0 && wwin) {
	wGNOMEUpdateClientStateHint(wwin, True);
    } else if (strcmp(name, WMNChangedState) == 0 && wwin) {
	wGNOMEUpdateClientStateHint(wwin, False);
    } 
}

static void wsobserver(void *self, WMNotification *notif)
{
    WScreen *scr = (WScreen*)WMGetNotificationObject(notif);
    const char *name = WMGetNotificationName(notif);
    
    if (strcmp(name, WMNWorkspaceCreated) == 0) {
	wGNOMEUpdateWorkspaceHints(scr);
    } else if (strcmp(name, WMNWorkspaceDestroyed) == 0) {
	wGNOMEUpdateWorkspaceHints(scr);
    } else if (strcmp(name, WMNWorkspaceNameChanged) == 0) {
	wGNOMEUpdateWorkspaceNamesHint(scr);
    } else if (strcmp(name, WMNWorkspaceChanged) == 0) {
	wGNOMEUpdateCurrentWorkspaceHint(scr);
	

    } else if (strcmp(name, WMNResetStacking) == 0) {
	
    }
}


#endif /* GNOME_STUFF */
