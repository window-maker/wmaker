/*  wmspec.c-- support for the wm-spec Hints
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */


#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "WindowMaker.h"
#include "window.h"
#include "screen.h"
#include "framewin.h"
#include "actions.h"


static Atom net_supported;
static Atom net_client_list;
static Atom net_client_list_stacking;
static Atom net_number_of_desktops;
static Atom net_desktop_geometry;
static Atom net_desktop_viewport;
static Atom net_current_desktop;
static Atom net_active_window;
static Atom net_workarea;
static Atom net_supporting_wm_check;
/* net_virtual_roots N/A */
/* TODO
 * _NET_WM_NAME
 * _NET_WM_VISIBLE_NAME
 * _NET_WM_ICON_NAME
 * _NET_WM_VISIBLE_ICON_NAME
 * _NET_WM_WINDOW_TYPE
 */
static Atom net_wm_desktop;
static Atom net_wm_state;

static Atom net_wm_strut;
/*
 * _NET_WM_ICON_GEOMETRY
 * _NET_WM_ICON
 * _NET_WM_PID
 * _NET_WM_HANDLED_ICONS
 * 
 */

static Atom net_wm_state_modal;
static Atom net_wm_state_sticky;
static Atom net_wm_state_maximized_vert;
static Atom net_wm_state_maximized_horz;
static Atom net_wm_state_shaded;
static Atom net_wm_state_skip_taskbar;
static Atom net_wm_state_skip_pager;



/* states */


static char *atomNames[] = {
    "_NET_SUPPORTED",
	"_NET_CLIENT_LIST",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_DESKTOP_GEOMETRY",
	"_NET_DESKTOP_VIEWPORT",
	"_NET_CURRENT_DESKTOP",
	"_NET_ACTIVE_WINDOW",
	"_NET_WORKAREA",
	"_NET_SUPPORTING_WM_CHECK",
	"_NET_WM_DESKTOP",
	"_NET_WM_STATE",
	"_NET_WM_STRUT",
	
	
	
	"_NET_WM_STATE_MODAL",
	"_NET_WM_STATE_STICKY",
	"_NET_WM_STATE_MAXIMIZED_VERT",
	"_NET_WM_STATE_MAXIMIZED_HORZ",
	"_NET_WM_STATE_SHADED",
	"_NET_WM_STATE_SKIP_TASKBAR",
	"_NET_WM_STATE_SKIP_PAGER"
};

static void observer(void *self, WMNotification *notif);
static void wsobserver(void *self, WMNotification *notif);


typedef struct NetData {
    WScreen *scr;
    Window *client_windows;
    int client_count;
    int client_size;
} NetData;





static void setSupportedHints(WScreen *scr)
{
    Atom atom[32];
    int i = 0;
    unsigned int sizes[2];
    
    /* set supported hints list */
    
    atom[i++] = net_client_list;

    
    XChangeProperty(dpy, scr->root_win, 
		    net_supported, XA_ATOM, 32,
		    PropModeReplace, (unsigned char*)atom, i);
    
    /* set supporting wm hint */
    XChangeProperty(dpy, scr->root_win,
		    net_supporting_wm_check, XA_WINDOW, 32,
		    PropModeReplace, 
		    (unsigned char*)&scr->info_window, 1);

    XChangeProperty(dpy, scr->info_window, 
		    net_supporting_wm_check, XA_WINDOW, 32,
		    PropModeReplace, 
		    (unsigned char*)&scr->info_window, 1);
    
    
    /* set desktop geometry. dynamic change is not supported */
    sizes[0] = scr->scr_width;
    sizes[1] = scr->scr_height;

    XChangeProperty(dpy, scr->root_win,
		    net_desktop_geometry, XA_CARDINAL, 32,
		    PropModeReplace,
		    (unsigned char*)&sizes, 2);
    
    /* set desktop viewport. dynamic change is not supported */
    sizes[0] = 0;
    sizes[1] = 0;

    XChangeProperty(dpy, scr->root_win,
		    net_desktop_viewport, XA_CARDINAL, 32,
		    PropModeReplace,
		    (unsigned char*)&sizes, 2);

}


void netwmInitialize(WScreen *scr)
{
    NetData *data;
    Atom atom[32];

#ifdef HAVE_XINTERNATOMS
    XInternAtoms(dpy, atomNames, sizeof(atomNames)/sizeof(char*),
		 False, atom);
#else
    {
	int i;
	for (i = 0; i < sizeof(atomNames)/sizeof(char*); i++) {
	    atom[i] = XInternAtom(dpy, atomNames[i], False);
	}
    }
#endif
    net_supported = atom[0];
    net_client_list = atom[1];
    
    setSupportedHints(scr);
    
    data = wmalloc(sizeof(NetData));
    data->scr = scr;
    data->client_windows = NULL;
    data->client_count = 0;
    data->client_size = 0;

    WMAddNotificationObserver(observer, data, WMNManaged, NULL);
    WMAddNotificationObserver(observer, data, WMNUnmanaged, NULL);
    WMAddNotificationObserver(observer, data, WMNChangedWorkspace, NULL);
    WMAddNotificationObserver(observer, data, WMNChangedState, NULL);
    WMAddNotificationObserver(observer, data, WMNChangedFocus, NULL);
    WMAddNotificationObserver(observer, data, WMNChangedStacking, NULL);
    WMAddNotificationObserver(observer, data, WMNChangedName, NULL);

    WMAddNotificationObserver(wsobserver, data, WMNWorkspaceCreated, NULL);
    WMAddNotificationObserver(wsobserver, data, WMNWorkspaceDestroyed, NULL);
    WMAddNotificationObserver(wsobserver, data, WMNWorkspaceChanged, NULL);
    WMAddNotificationObserver(wsobserver, data, WMNWorkspaceNameChanged, NULL);
}


void
netwmUpdateWorkarea(WScreen *scr)
{
    unsigned int area[4];
    
    /* XXX */

    XChangeProperty(dpy, scr->root_win, net_workarea, XA_CARDINAL, 32,
		    PropModeReplace,
		    (unsigned char*)area, 4);
}



static void
updateClientList(WScreen *scr, WWindow *wwin, Bool added)
{
    NetData *data = scr->netdata;

    if (added) {
	if (data->client_count == data->client_size) {
	    data->client_size += 20;
	    data->client_windows = wrealloc(data->client_windows, 
					    sizeof(Window)*data->client_size);
	}

	data->client_windows[data->client_count++] = wwin->client_win;

	XChangeProperty(dpy, scr->root_win, net_client_list, XA_WINDOW, 32,
			PropModeAppend,	(unsigned char*)&wwin->client_win, 1);
    } else {
	int i;
	for (i = 0; i < data->client_count; i++) {
	    if (data->client_windows[i] == wwin->client_win) {
		if (data->client_count-1 > i) {
		    memmove(data->client_windows+i,
			    data->client_windows+i+1,
			    (data->client_count-i-1)*sizeof(Window));
		}
		data->client_count--;
		break;
	    }
	}

	/* update client list */
	XChangeProperty(dpy, scr->root_win, net_client_list, XA_WINDOW, 32,
			PropModeReplace,
			(unsigned char *)data->client_windows,
			data->client_count);
    }
    XFlush(dpy);
}



static void
updateClientListStacking(WScreen *scr)
{
    WWindow *wwin;
    Window *client_list;
    int client_count;
    WCoreWindow *tmp;
    WMBagIterator iter;

    /* update client list */
    
    client_list = (Window*)malloc(sizeof(Window)*scr->window_count);
    if (!client_list) {
	wwarning(_("out of memory while updating wm hints"));
	return;
    }

    WM_ETARETI_BAG(scr->stacking_list, tmp, iter) {
	while (tmp) {
	    client_list[client_count++] = tmp->window;
	    tmp = tmp->stacking->under;
	}
    }

    XChangeProperty(dpy, scr->root_win, 
		    net_client_list_stacking, XA_WINDOW, 32,
		    PropModeReplace, 
		    (unsigned char *)client_list, client_count);

    wfree(client_list);

    XFlush(dpy);
}



static void
updateWorkspaceCount(WScreen *scr) /* changeable */
{
    unsigned int count = scr->workspace_count;
    
    XChangeProperty(dpy, scr->root_win,
		    net_number_of_desktops, XA_CARDINAL, 32,
		    PropModeReplace,
		    (unsigned char*)&count, 1);
}


static void
updateCurrentWorkspace(WScreen *scr) /* changeable */
{
    unsigned int count = scr->current_workspace;
    
    XChangeProperty(dpy, scr->root_win,
		    net_current_desktop, XA_CARDINAL, 32,
		    PropModeReplace,
		    (unsigned char*)&count, 1);
}


static void
updateWorkspaceNames(WScreen *scr, int workspace)
{
    /* XXX UTF 8 */
}


static void
updateFocusHint(WScreen *scr, WWindow *wwin) /* changeable */
{
    Window window = None;
    
    if (wwin)
	window = wwin->client_win;
    
    XChangeProperty(dpy, scr->root_win,
		    net_active_window, XA_WINDOW, 32,
		    PropModeReplace,
		    (unsigned char*)&window, 1);
}


static void
updateWorkspaceHint(WWindow *wwin, Bool del)
{
    if (del) {
	XDeleteProperty(dpy, wwin->client_win, net_wm_desktop);
    } else {
	XChangeProperty(dpy, wwin->screen_ptr->root_win,
			net_wm_desktop, XA_CARDINAL, 32,
			PropModeReplace,
			(unsigned char*)&wwin->frame->workspace, 1);
    }
}


static void
updateStateHint(WWindow *wwin, Bool del) /* changeable */
{
    if (del) {
	if (!wwin->flags.net_state_from_client)
	    XDeleteProperty(dpy, wwin->client_win, net_wm_state);
    } else {
	Atom state[8];
	int i = 0;
	
	if (wwin->flags.omnipresent)
	    state[i++] = net_wm_state_sticky;
	if (wwin->flags.maximized & MAX_HORIZONTAL)
	    state[i++] = net_wm_state_maximized_horz;
	if (wwin->flags.maximized & MAX_VERTICAL)
	    state[i++] = net_wm_state_maximized_vert;
	if (wwin->flags.shaded)
	    state[i++] = net_wm_state_shaded;
	if (WFLAGP(wwin, skip_window_list) || wwin->flags.net_skip_taskbar)
	    state[i++] = net_wm_state_skip_taskbar;
	if (wwin->flags.net_skip_pager)
	    state[i++] = net_wm_state_skip_pager;
    }
}


static void
updateStrut(WWindow *wwin, Bool adding)
{
    if (adding) { 
//	XGetWindowProperty(dpy, wwin->client_win, 
			   
	/* XXX add property observer */
    } else {
    }
}


static void
observer(void *self, WMNotification *notif)
{
    WWindow *wwin = (WWindow*)WMGetNotificationObject(notif);
    const char *name = WMGetNotificationName(notif);
    void *data = WMGetNotificationClientData(notif);
    NetData *ndata = (NetData*)self;
    
    
    if (strcmp(name, WMNManaged) == 0 && wwin) {
	updateClientList(wwin->screen_ptr, wwin, True);
	
	updateStrut(wwin, True);

    } else if (strcmp(name, WMNUnmanaged) == 0 && wwin) {
	updateClientList(wwin->screen_ptr, wwin, False);
	updateWorkspaceHint(wwin, True);
	updateStateHint(wwin, True);
	
	updateStrut(wwin, False);
	
    } else if (strcmp(name, WMNResetStacking) == 0 && wwin) {
	updateClientListStacking(wwin->screen_ptr);
	
    } else if (strcmp(name, WMNChangedStacking) == 0 && wwin) {
	updateClientListStacking(wwin->screen_ptr);
	
    } else if (strcmp(name, WMNChangedFocus) == 0) {
	updateFocusHint(ndata->scr, wwin);
	
    } else if (strcmp(name, WMNChangedWorkspace) == 0 && wwin) {
	updateWorkspaceHint(wwin, False);
	
    } else if (strcmp(name, WMNChangedState) == 0 && wwin) {
	updateStateHint(wwin, False);
    }
}


static void
wsobserver(void *self, WMNotification *notif)
{
    WScreen *scr = (WScreen*)WMGetNotificationObject(notif);
    const char *name = WMGetNotificationName(notif);
    void *data = WMGetNotificationClientData(notif);

    if (strcmp(name, WMNWorkspaceCreated) == 0) {
	updateWorkspaceCount(scr);
    } else if (strcmp(name, WMNWorkspaceDestroyed) == 0) {
	updateWorkspaceCount(scr);
    } else if (strcmp(name, WMNWorkspaceChanged) == 0) {
	updateCurrentWorkspace(scr);
    } else if (strcmp(name, WMNWorkspaceNameChanged) == 0) {
	updateWorkspaceNames(scr, (int)data);
    }
}



