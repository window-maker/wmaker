/* kwm.c-- stuff for support for kwm hints
 *
 *  Window Maker window manager
 * 
 *  Copyright (c) 1998, 1999 Alfredo K. Kojima
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
 * 
 * Supported stuff:
 * ================
 * kwm.h function/method	Notes
 *----------------------------------------------------------------------------
 * setUnsavedDataHint() 	currently, only gives visual clue that 
 *				there is unsaved data (broken X close button)
 * setSticky()			
 * setIcon()			std X thing...
 * setDecoration()		
 * logout()
 * refreshScreen()
 * setWmCommand()		std X thing
 * currentDesktop()
 * setKWMModule()
 *
 * isKWMInitialized()		not trustworthy
 * activeWindow()		dunno it's use, but since it's easy to 
 * 				implement it won't hurt to support
 * switchToDesktop()
 * (set/get)WindowRegion()
 * (set)numberOfDesktops()	KDE limits to 32, but wmaker is virtually
 * 				unlimited. May raise some incompatibility
 * 				in badly written KDE modules?
 * (set/get)DesktopName()
 * sendKWMCommand()		also does the message relay thing
 * desktop()
 * geometryRestore()
 * isIconified()		
 * isMaximized()		
 * isSticky()
 * moveToDesktop()		WARNING: evil mechanism
 * setGeometryRestore()		WARNING: evil mechanism
 * setMaximize()		woo hoo! wanna race?
 * setIconify()			BAH!: why reinvent the f'ing wheel!? 
 * 				its even broken!!!
 * move()			std X thing
 * setGeometry()		std X thing
 * close()			std X thing
 * activate()
 * activateInternal()
 * raise()			std X thing
 * lower()			std X thing
 * prepareForSwallowing()	std X thing
 * doNotManage()		klugy thing...
 * getBlablablaString()
 * setKWMDockModule()		maybe we can make the Dock behave as the KDE
 * 				dock, but must figure where to show the windows
 * setDockWindow()
 * 
 * Unsupported stuff (superfluous/not-essential/nonsense):
 * =======================================================
 * 
 * darkenScreen()
 * setMiniIcon()
 * configureWm()
 * raiseSoundEvent()
 * registerSoundEvent()
 * unregisterSoundEvent()
 * title()
 * titleWithState()
 * geometry()			kde lib code makes it unnecessary
 * 
 * 
 * Extensions to KDE:
 * ==================
 * 
 * These are clientmessage-type messages specific to Window Maker:
 * wmaker:info - show info panel
 * wmaker:legal - show legal panel
 * wmaker:arrangeIcons - arrange icons
 * wmaker:showAll - show all windows
 * wmaker:hideOthers - hide others
 * wmaker:restart - restart wmaker
 * wmaker:exit - exit wmaker
 */

/*
 * TODO
 * different WORKAREA for each workspace
 */


#include "wconfig.h"

#ifdef KWM_HINTS

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
#include "properties.h"
#include "icon.h"
#include "client.h"
#include "funcs.h"
#include "actions.h"
#include "workspace.h"
#include "dialog.h"

#include "kwm.h"

/*#define DEBUG1
*/
/******* Global ******/

extern Time LastFocusChange;
extern Time LastTimestamp;


extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;
extern Atom _XA_WM_DELETE_WINDOW;

/** Local **/

static Atom _XA_KWM_COMMAND = 0;
static Atom _XA_KWM_ACTIVATE_WINDOW = 0;
static Atom _XA_KWM_ACTIVE_WINDOW = 0;
static Atom _XA_KWM_DO_NOT_MANAGE = 0;
static Atom _XA_KWM_DOCKWINDOW = 0;
static Atom _XA_KWM_RUNNING = 0;

static Atom _XA_KWM_MODULE = 0;

static Atom _XA_KWM_MODULE_INIT = 0;
static Atom _XA_KWM_MODULE_INITIALIZED = 0;
static Atom _XA_KWM_MODULE_DESKTOP_CHANGE = 0;
static Atom _XA_KWM_MODULE_DESKTOP_NAME_CHANGE = 0;
static Atom _XA_KWM_MODULE_DESKTOP_NUMBER_CHANGE = 0;
static Atom _XA_KWM_MODULE_WIN_ADD = 0;
static Atom _XA_KWM_MODULE_WIN_REMOVE = 0;
static Atom _XA_KWM_MODULE_WIN_CHANGE = 0;
static Atom _XA_KWM_MODULE_WIN_RAISE = 0;
static Atom _XA_KWM_MODULE_WIN_LOWER = 0;
static Atom _XA_KWM_MODULE_WIN_ACTIVATE = 0;
static Atom _XA_KWM_MODULE_WIN_ICON_CHANGE = 0;
static Atom _XA_KWM_MODULE_DOCKWIN_ADD = 0;
static Atom _XA_KWM_MODULE_DOCKWIN_REMOVE = 0;
  
static Atom _XA_KWM_WIN_UNSAVED_DATA = 0;
static Atom _XA_KWM_WIN_DECORATION = 0;
static Atom _XA_KWM_WIN_DESKTOP = 0;
static Atom _XA_KWM_WIN_GEOMETRY_RESTORE = 0;
static Atom _XA_KWM_WIN_ICONIFIED = 0;
static Atom _XA_KWM_WIN_MAXIMIZED = 0;
static Atom _XA_KWM_WIN_STICKY = 0;

static Atom _XA_KWM_CURRENT_DESKTOP = 0;
static Atom _XA_KWM_NUMBER_OF_DESKTOPS = 0;
static Atom _XA_KWM_DESKTOP_NAME_[MAX_WORKSPACES];
static Atom _XA_KWM_WINDOW_REGION_[MAX_WORKSPACES];



/* list of window titles that must not be managed */
typedef struct KWMDoNotManageList {
    char title[20];
    struct KWMDoNotManageList *next;
} KWMDoNotManageList;

static KWMDoNotManageList *KWMDoNotManageCrap = NULL;


/* list of KWM modules */
typedef struct KWMModuleList {
    Window window;
    struct KWMModuleList *next;
#ifdef DEBUG1
    char *title;
#endif
} KWMModuleList;

static KWMModuleList *KWMModules = NULL;

static KWMModuleList *KWMDockWindows = NULL;

/* window decoration types */
enum {
    KWMnoDecoration = 0, 
	KWMnormalDecoration = 1, 
	KWMtinyDecoration = 2,
	KWMnoFocus = 256
};



static Bool
getSimpleHint(Window win, Atom atom, long *retval)
{
    long *data = 0;

    assert(atom!=0);

    data = (long*)PropGetCheckProperty(win, atom, atom, 32, 1, NULL);

    if (!data)
	return False;

    *retval = *data;

    XFree(data);

    return True;
}



static void
setSimpleHint(Window win, Atom atom, long value)
{
    assert(atom!=0);
    XChangeProperty(dpy, win, atom, atom,
		    32, PropModeReplace, (unsigned char*)&value, 1);
}


static void
sendClientMessage(WScreen *scr, Window window, Atom atom, long value)
{
    XEvent event;
    long mask = 0;

    assert(atom!=0);

    memset(&event, 0, sizeof(XEvent));
    event.xclient.type = ClientMessage;
    event.xclient.message_type = atom;
    event.xclient.window = window;
    event.xclient.format = 32;
    event.xclient.data.l[0] = value;
    event.xclient.data.l[1] = LastTimestamp;

    if (scr && scr->root_win == window)
	mask = SubstructureRedirectMask;

    XSendEvent(dpy, window, False, mask, &event);
}


static void
sendTextMessage(WScreen *scr, Window window, Atom atom, char *text)
{
    XEvent event;
    long mask = 0;
    int i;

    assert(atom!=0);

    memset(&event, 0, sizeof(XEvent));
    event.xclient.type = ClientMessage;
    event.xclient.message_type = atom;
    event.xclient.window = window;
    event.xclient.format = 8;

    for (i=0; i<20 && text[i]; i++)
	event.xclient.data.b[i] = text[i];

    if (scr && scr->root_win == window)
	mask = SubstructureRedirectMask;

    XSendEvent(dpy, window, False, mask, &event);
}


static Bool
getAreaHint(Window win, Atom atom, WArea *area)
{
    long *data = 0;

    data = (long*)PropGetCheckProperty(win, atom, atom, 32, 4, NULL);

    if (!data)
	return False;

    area->x1 = data[0];
    area->y1 = data[1];
    area->x2 = data[2] + area->x1;
    area->y2 = data[3] + area->y1;

    XFree(data);

    return True;
}


static void
setAreaHint(Window win, Atom atom, WArea area)
{
    long value[4];

    assert(atom!=0);
    value[0] = area.x1;
    value[1] = area.y1;
    value[2] = area.x2 - area.x1;
    value[3] = area.y2 - area.y1;
    XChangeProperty(dpy, win, atom, atom, 32, PropModeReplace, 
		    (unsigned char*)&value, 4);
}


static void
addModule(WScreen *scr, Window window)
{
    KWMModuleList *node;
    long val;
    WWindow *ptr;

    node = malloc(sizeof(KWMModuleList));
    if (!node) {
	wwarning("out of memory while registering KDE module");
	return;
    }

    node->next = KWMModules;
    node->window = window;
    KWMModules = node;

    sendClientMessage(scr, window, _XA_KWM_MODULE_INIT, 0);

    if (getSimpleHint(window, _XA_KWM_MODULE, &val) && val==2) {
	if (scr->kwm_dock != None) {
	    setSimpleHint(window, _XA_KWM_MODULE, 1);
	} else {
	    KWMModuleList *ptr;
	    
	    scr->kwm_dock = window;

	    for (ptr = KWMDockWindows; ptr!=NULL; ptr = ptr->next) {
		sendClientMessage(scr, scr->kwm_dock, _XA_KWM_MODULE_DOCKWIN_ADD,
				  ptr->window);
	    }
	}
    }

    /* send list of windows */
    for (ptr = scr->focused_window; ptr!=NULL; ptr = ptr->prev) {
	if (!ptr->flags.kwm_hidden_for_modules
	    && !WFLAGP(ptr, skip_window_list)) {
	    sendClientMessage(scr, window, _XA_KWM_MODULE_WIN_ADD,
			      ptr->client_win);
	}
    }

    /* send window stacking order */
    wKWMSendStacking(scr, window);

    /* send focused window */
    if (scr->focused_window && scr->focused_window->flags.focused) {
	sendClientMessage(scr, window, _XA_KWM_MODULE_WIN_ACTIVATE,
			  scr->focused_window->client_win);
    }

    /* tell who we are */
    sendTextMessage(scr, window, _XA_KWM_COMMAND, "wm:wmaker");


    sendClientMessage(scr, window, _XA_KWM_MODULE_INITIALIZED, 0);
#ifdef DEBUG1
    KWMModules->title = NULL;
    XFetchName(dpy, window, &KWMModules->title);
    printf("NEW MODULE %s\n", KWMModules->title);
#endif
}


static void
removeModule(WScreen *scr, Window window)
{
    KWMModuleList *next;

    if (!KWMModules) {
	return;
    }

    if (KWMModules->window == window) {
	next = KWMModules->next;
#ifdef DEBUG1
	printf("REMOVING MODULE %s\n", KWMModules->title);
	if (KWMModules->title)
	    XFree(KWMModules->title);
#endif
	free(KWMModules);
	KWMModules = next;
    } else {
	KWMModuleList *ptr;

	ptr = KWMModules;
	while (ptr->next) {
	    if (ptr->next->window == window) {
		next = ptr->next->next;
#ifdef DEBUG1
		printf("REMOVING MODULE %s\n", ptr->next->title);
		if (ptr->next->title)
		    XFree(ptr->next->title);
#endif
		free(ptr->next);
		ptr->next->next = next;
		break;
	    }
	    ptr->next = ptr->next->next;
	}
    }

    if (scr->kwm_dock == window)
	scr->kwm_dock = None;
}



static void
addDockWindow(WScreen *scr, Window window)
{
    KWMModuleList *ptr;

    for (ptr = KWMDockWindows; ptr != NULL; ptr = ptr->next) {
	if (ptr->window == window)
	    break;
    }
    if (!ptr) {
	KWMModuleList *node;

	node = malloc(sizeof(KWMModuleList));
	if (!node) {
	    wwarning("out of memory while processing KDE dock window");
	    return;
	}
	node->next = KWMDockWindows;
	KWMDockWindows = node;
	node->window = window;
	XSelectInput(dpy, window, StructureNotifyMask);

	sendClientMessage(scr, scr->kwm_dock, _XA_KWM_MODULE_DOCKWIN_ADD,
			  window);
    }
}


static void
removeDockWindow(WScreen *scr, Window window)
{
    if (!KWMDockWindows)
	return;

    if (KWMDockWindows->window == window) {
	KWMModuleList *next;

	sendClientMessage(scr, scr->kwm_dock, _XA_KWM_MODULE_DOCKWIN_REMOVE,
			  window);

	next = KWMDockWindows->next;
	free(KWMDockWindows);
	KWMDockWindows = next;

    } else {
	KWMModuleList *ptr, *next;

	ptr = KWMDockWindows;
	while (ptr->next) {
	    if (ptr->next->window == window) {
		sendClientMessage(scr, scr->kwm_dock,
				  _XA_KWM_MODULE_DOCKWIN_REMOVE, window);
		next = ptr->next->next;
		free(ptr->next);
		ptr->next = next;
		return;
	    }
	    ptr = ptr->next;
	}
    }
}


static void
sendToModules(WScreen *scr, Atom atom, WWindow *wwin, long data)
{
    KWMModuleList *ptr;
    XEvent event;
    long mask;

    if (wwin) {
	if (wwin->flags.kwm_hidden_for_modules
	    || WFLAGP(wwin, skip_window_list))
	    return;
	data = wwin->client_win;
    }
#ifdef DEBUG1    
    printf("notifying %s\n",XGetAtomName(dpy, atom));
#endif
    memset(&event, 0, sizeof(XEvent));
    event.xclient.type = ClientMessage;
    event.xclient.message_type = atom;
    event.xclient.format = 32;
    event.xclient.data.l[1] = LastTimestamp;

    mask = 0;
    if (scr && scr->root_win == data)
	mask = SubstructureRedirectMask;

    for (ptr = KWMModules; ptr!=NULL; ptr = ptr->next) {
	event.xclient.window = ptr->window;
	event.xclient.data.l[0] = data;
	XSendEvent(dpy, ptr->window, False, mask, &event);
    }
}


void
wKWMInitStuff(WScreen *scr)
{
    if (!_XA_KWM_WIN_STICKY) {
	_XA_KWM_WIN_UNSAVED_DATA = XInternAtom(dpy, "KWM_WIN_UNSAVED_DATA",
					       False);

	_XA_KWM_WIN_DECORATION = XInternAtom(dpy, "KWM_WIN_DECORATION", False);

	_XA_KWM_WIN_DESKTOP = XInternAtom(dpy, "KWM_WIN_DESKTOP", False);

	_XA_KWM_WIN_GEOMETRY_RESTORE = XInternAtom(dpy, 
						   "KWM_WIN_GEOMETRY_RESTORE",
						   False);

	_XA_KWM_WIN_STICKY = XInternAtom(dpy, "KWM_WIN_STICKY", False);

	_XA_KWM_WIN_ICONIFIED = XInternAtom(dpy, "KWM_WIN_ICONIFIED", False);

	_XA_KWM_WIN_MAXIMIZED = XInternAtom(dpy, "KWM_WIN_MAXIMIZED", False);

	_XA_KWM_COMMAND = XInternAtom(dpy, "KWM_COMMAND", False);

	_XA_KWM_ACTIVE_WINDOW = XInternAtom(dpy, "KWM_ACTIVE_WINDOW", False);

	_XA_KWM_ACTIVATE_WINDOW = XInternAtom(dpy, "KWM_ACTIVATE_WINDOW",
					      False);

	_XA_KWM_DO_NOT_MANAGE = XInternAtom(dpy, "KWM_DO_NOT_MANAGE", False);
	
	_XA_KWM_CURRENT_DESKTOP = XInternAtom(dpy, "KWM_CURRENT_DESKTOP", 
					      False);

	_XA_KWM_NUMBER_OF_DESKTOPS = XInternAtom(dpy, "KWM_NUMBER_OF_DESKTOPS",
						 False);

	_XA_KWM_DOCKWINDOW = XInternAtom(dpy, "KWM_DOCKWINDOW", False);

	_XA_KWM_RUNNING = XInternAtom(dpy, "KWM_RUNNING", False);

	_XA_KWM_MODULE = XInternAtom(dpy, "KWM_MODULE", False);

	_XA_KWM_MODULE_INIT = XInternAtom(dpy, "KWM_MODULE_INIT", False);
	_XA_KWM_MODULE_INITIALIZED = XInternAtom(dpy, "KWM_MODULE_INITIALIZED", False);

	/* dunno what these do, but Matthias' patch contains it... */
	_XA_KWM_MODULE_DESKTOP_CHANGE = XInternAtom(dpy, "KWM_MODULE_DESKTOP_CHANGE", False);
	_XA_KWM_MODULE_DESKTOP_NAME_CHANGE = XInternAtom(dpy, "KWM_MODULE_DESKTOP_NAME_CHANGE", False);
	_XA_KWM_MODULE_DESKTOP_NUMBER_CHANGE = XInternAtom(dpy, "KWM_MODULE_DESKTOP_NUMBER_CHANGE", False);

	_XA_KWM_MODULE_WIN_ADD = XInternAtom(dpy, "KWM_MODULE_WIN_ADD", False);
	_XA_KWM_MODULE_WIN_REMOVE = XInternAtom(dpy, "KWM_MODULE_WIN_REMOVE", False);
	_XA_KWM_MODULE_WIN_CHANGE = XInternAtom(dpy, "KWM_MODULE_WIN_CHANGE", False);
	_XA_KWM_MODULE_WIN_RAISE = XInternAtom(dpy, "KWM_MODULE_WIN_RAISE", False);
	_XA_KWM_MODULE_WIN_LOWER = XInternAtom(dpy, "KWM_MODULE_WIN_LOWER", False);
	_XA_KWM_MODULE_WIN_ACTIVATE = XInternAtom(dpy, "KWM_MODULE_WIN_ACTIVATE", False);
	_XA_KWM_MODULE_WIN_ICON_CHANGE = XInternAtom(dpy, "KWM_MODULE_WIN_ICON_CHANGE", False);
	_XA_KWM_MODULE_DOCKWIN_ADD = XInternAtom(dpy, "KWM_MODULE_DOCKWIN_ADD", False);
	_XA_KWM_MODULE_DOCKWIN_REMOVE = XInternAtom(dpy, "KWM_MODULE_DOCKWIN_REMOVE", False);
	
	memset(_XA_KWM_WINDOW_REGION_, 0, sizeof(_XA_KWM_WINDOW_REGION_));

	memset(_XA_KWM_DESKTOP_NAME_, 0, sizeof(_XA_KWM_DESKTOP_NAME_));
    }

#define SETSTR(hint, str) {\
	static Atom a = 0; if (!a) a = XInternAtom(dpy, #hint, False);\
	XChangeProperty(dpy, scr->root_win, a, XA_STRING, 8, PropModeReplace,\
	    	    (unsigned char*)str, strlen(str));}

    SETSTR(KWM_STRING_MAXIMIZE, _("Maximize"));
    SETSTR(KWM_STRING_UNMAXIMIZE, _("Unmaximize"));
    SETSTR(KWM_STRING_ICONIFY, _("Miniaturize"));
    SETSTR(KWM_STRING_UNICONIFY, _("Deminiaturize"));
    SETSTR(KWM_STRING_STICKY, _("Omnipresent"));
    SETSTR(KWM_STRING_UNSTICKY, _("Not Omnipresent"));
    SETSTR(KWM_STRING_STRING_MOVE, _("Move"));
    SETSTR(KWM_STRING_STRING_RESIZE, _("Resize"));
    SETSTR(KWM_STRING_CLOSE, _("Close"));
    SETSTR(KWM_STRING_TODESKTOP, _("Move To"));
    SETSTR(KWM_STRING_ONTOCURRENTDESKTOP, _("Bring Here"));
#undef SETSTR
}


void
wKWMSendStacking(WScreen *scr, Window module)
{
    int i;
    WCoreWindow *core;

    for (i = 0; i < MAX_WINDOW_LEVELS; i++) {
	for (core = scr->stacking_list[i]; core != NULL; 
	     core = core->stacking->under) {
	    WWindow *wwin;

	    wwin = wWindowFor(core->window);
	    if (wwin)
		sendClientMessage(scr, module, _XA_KWM_MODULE_WIN_RAISE,
				  wwin->client_win);
	}
    }
}


void
wKWMBroadcastStacking(WScreen *scr)
{
    KWMModuleList *ptr = KWMModules;

    while (ptr) {
	wKWMSendStacking(scr, ptr->window);

	ptr = ptr->next;
    }
}


char*
wKWMGetWorkspaceName(WScreen *scr, int workspace)
{
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    char *data = NULL, *tmp;
    char buffer[64];

    assert(workspace >= 0 && workspace < MAX_WORKSPACES);

    if (_XA_KWM_DESKTOP_NAME_[workspace]==0) {
	sprintf(buffer, "KWM_DESKTOP_NAME_%d", workspace + 1);

	_XA_KWM_DESKTOP_NAME_[workspace] = XInternAtom(dpy, buffer, False);
    }
    
    /* What do these people have against correctly using property types? */
    if (XGetWindowProperty(dpy, scr->root_win, 
			   _XA_KWM_DESKTOP_NAME_[workspace], 0, 128, False, 
			   XA_STRING,
			   &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
			   (unsigned char**)&data)!=Success || !data)
	return NULL;

    tmp = wstrdup(data);
    XFree(data);

    return tmp;
}


void
wKWMSetInitializedHint(WScreen *scr)
{
    setSimpleHint(scr->root_win, _XA_KWM_RUNNING, 1);
}


void
wKWMShutdown(WScreen *scr, Bool closeModules)
{
    KWMModuleList *ptr;

    XDeleteProperty(dpy, scr->root_win, _XA_KWM_RUNNING);

    if (closeModules) {
	for (ptr = KWMModules; ptr != NULL; ptr = ptr->next) {
	    XKillClient(dpy, ptr->window);
	}
    }
}


void
wKWMCheckClientHints(WWindow *wwin, int *workspace)
{
    long val;

    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_UNSAVED_DATA, &val)	
	&& val) {

	wwin->client_flags.broken_close = 1;
    }
    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_DECORATION, &val)) {
	if (val & KWMnoFocus) {
	    wwin->client_flags.no_focusable = 1;
	}
	switch (val & ~KWMnoFocus) {
	 case KWMnoDecoration:
	    wwin->client_flags.no_titlebar = 1;
	    wwin->client_flags.no_resizebar = 1;
	    break;
	 case KWMtinyDecoration:
	    wwin->client_flags.no_resizebar = 1;
	    break;
	 case KWMnormalDecoration:
	 default:
	    break;
	}
    }
    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_DESKTOP, &val)) {
	*workspace = val - 1;
    }
}


void
wKWMCheckClientInitialState(WWindow *wwin)
{
    long val;

    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_STICKY, &val) && val) {

	wwin->client_flags.omnipresent = 1;
    }
    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_ICONIFIED, &val) 
	&& val) {

	wwin->flags.miniaturized = 1;
    }
    if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_MAXIMIZED, &val) 
	&& val) {

	wwin->flags.maximized = MAX_VERTICAL|MAX_HORIZONTAL;
    }
}


Bool
wKWMCheckClientHintChange(WWindow *wwin, XPropertyEvent *event)
{
    Bool processed = True;
    Bool flag;
    long value;


    if (!wwin->frame) {
	return False;
    }
    
    if (event->atom == _XA_KWM_WIN_UNSAVED_DATA) {
#ifdef DEBUG1
	printf("got KDE unsaved data change\n");
#endif

	flag = getSimpleHint(wwin->client_win, _XA_KWM_WIN_UNSAVED_DATA,
			     &value) && value;

	if (flag != wwin->client_flags.broken_close) {
	    wwin->client_flags.broken_close = flag;
	    if (wwin->frame)
		wWindowUpdateButtonImages(wwin);
	}
    } else if (event->atom == _XA_KWM_WIN_STICKY) {

#ifdef DEBUG1
	printf("got KDE sticky change\n");
#endif
	flag = !getSimpleHint(wwin->client_win, _XA_KWM_WIN_STICKY, 
			     &value) || value;

	if (flag != wwin->client_flags.omnipresent) {

	    wwin->client_flags.omnipresent = flag;

	    UpdateSwitchMenu(wwin->screen_ptr, wwin, ACTION_CHANGE_WORKSPACE);

	}
    } else if (event->atom == _XA_KWM_WIN_MAXIMIZED) {

#ifdef DEBUG1
	printf("got KDE maximize change\n");
#endif
	flag = !getSimpleHint(wwin->client_win, _XA_KWM_WIN_MAXIMIZED,
			     &value) || value;

	if (flag != (wwin->flags.maximized!=0)) {

	    if (flag)
		wMaximizeWindow(wwin, flag*(MAX_VERTICAL|MAX_HORIZONTAL));
	    else
		wUnmaximizeWindow(wwin);
	} 
    } else if (event->atom == _XA_KWM_WIN_ICONIFIED) {

#ifdef DEBUG1
	printf("got KDE iconify change\n");
#endif
	flag = !getSimpleHint(wwin->client_win, _XA_KWM_WIN_ICONIFIED,
			     &value) || value;

	if (flag != wwin->flags.miniaturized) {

	    if (flag)
		wIconifyWindow(wwin);
	    else
		wDeiconifyWindow(wwin);
	} 

    } else if (event->atom == _XA_KWM_WIN_DECORATION) {
	Bool refresh = False;
	int oldnofocus;

#ifdef DEBUG1
	printf("got KDE decoration change\n");
#endif

	oldnofocus = wwin->client_flags.no_focusable;

	if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_DECORATION, &value)) {
	    wwin->client_flags.no_focusable = (value & KWMnoFocus)!=0;

	    switch (value & ~KWMnoFocus) {
	     case KWMnoDecoration:
		if (!WFLAGP(wwin, no_titlebar) || !WFLAGP(wwin, no_resizebar))
		    refresh = True;

		wwin->client_flags.no_titlebar = 1;
		wwin->client_flags.no_resizebar = 1;
		break;

	     case KWMtinyDecoration:
		if (WFLAGP(wwin, no_titlebar) || !WFLAGP(wwin, no_resizebar))
		    refresh = True;

		wwin->client_flags.no_titlebar = 0;
		wwin->client_flags.no_resizebar = 1;
		break;

	     case KWMnormalDecoration:
	     default:
		if (WFLAGP(wwin, no_titlebar) || WFLAGP(wwin, no_resizebar))
		    refresh = True;

		wwin->client_flags.no_titlebar = 0;
		wwin->client_flags.no_resizebar = 0;
		break;
	    }
	} else {
	    if (WFLAGP(wwin, no_titlebar) || WFLAGP(wwin, no_resizebar))
		refresh = True;
	    wwin->client_flags.no_focusable = (value & KWMnoFocus)!=0;
	    wwin->client_flags.no_titlebar = 0;
	    wwin->client_flags.no_resizebar = 0;
	}

	if (refresh)
	    wWindowConfigureBorders(wwin);

	if (wwin->client_flags.no_focusable && !oldnofocus) {

	    sendToModules(wwin->screen_ptr, _XA_KWM_MODULE_WIN_REMOVE, 
			  wwin, 0);
	    wwin->flags.kwm_hidden_for_modules = 1;

	} else if (!wwin->client_flags.no_focusable && oldnofocus) {

	    if (wwin->flags.kwm_hidden_for_modules) {
		sendToModules(wwin->screen_ptr, _XA_KWM_MODULE_WIN_ADD,
			      wwin, 0);
		wwin->flags.kwm_hidden_for_modules = 0;
	    }
	}
    } else if (event->atom == _XA_KWM_WIN_DESKTOP && wwin->frame) {
#ifdef DEBUG1
	printf("got KDE workspace change for %s\n", wwin->frame->title);
#endif
	if (getSimpleHint(wwin->client_win, _XA_KWM_WIN_DESKTOP, &value)
	    && value-1 != wwin->frame->workspace) {
	    wWindowChangeWorkspace(wwin, value-1);
	}

    } else if (event->atom == _XA_KWM_WIN_GEOMETRY_RESTORE) {
	WArea area;

#ifdef DEBUG1
	printf("got KDE geometry restore change\n");
#endif
	if (getAreaHint(wwin->client_win, _XA_KWM_WIN_GEOMETRY_RESTORE,	&area)
	    && (wwin->old_geometry.x != area.x1
		|| wwin->old_geometry.y != area.y1
		|| wwin->old_geometry.width != area.x2 - area.x1
		|| wwin->old_geometry.height != area.y2 - area.y1)) {

		wwin->old_geometry.x = area.x1;
		wwin->old_geometry.y = area.y1;
		wwin->old_geometry.width = area.x2 - area.x1;
		wwin->old_geometry.height = area.y2 - area.y1;
	    }
    } else {
	processed = False;
    }

    return processed;
}


static Bool
performWindowCommand(WScreen *scr, char *command)
{
    WWindow *wwin = NULL;


    wwin = scr->focused_window;
    if (!wwin || !wwin->flags.focused || !wwin->flags.mapped) {
	wwin = NULL;
    }

    CloseWindowMenu(scr);


    if (strcmp(command, "winMove")==0 || strcmp(command, "winResize")==0) {

	if (wwin)
	    wKeyboardMoveResizeWindow(wwin);

    } else if (strcmp(command, "winMaximize")==0) {

	if (wwin)
	    wMaximizeWindow(wwin, MAX_VERTICAL|MAX_HORIZONTAL);
	
    } else if (strcmp(command, "winRestore")==0) {

	if (wwin && wwin->flags.maximized)
	    wUnmaximizeWindow(wwin);

    } else if (strcmp(command, "winIconify")==0) {


	if (wwin && !WFLAGP(wwin, no_miniaturizable)) {
	    if (wwin->protocols.MINIATURIZE_WINDOW)
		wClientSendProtocol(wwin, _XA_GNUSTEP_WM_MINIATURIZE_WINDOW,
				    LastTimestamp);
	    else {
		wIconifyWindow(wwin);
	    }
	}

    } else if (strcmp(command, "winClose")==0) {

	if (wwin && !WFLAGP(wwin, no_closable)) {
	    if (wwin->protocols.DELETE_WINDOW)
		wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW, LastTimestamp);
	}

    } else if (strcmp(command, "winSticky")==0) {

	if (wwin) {
	    wwin->client_flags.omnipresent ^= 1;
	    UpdateSwitchMenu(scr, wwin, ACTION_CHANGE_WORKSPACE);
	}

    } else if (strcmp(command, "winShade")==0) {

	if (wwin && !WFLAGP(wwin, no_shadeable)) {
	    if (wwin->flags.shaded)
		wUnshadeWindow(wwin);
	    else
		wShadeWindow(wwin);
	}

    } else if (strcmp(command, "winOperation")==0) {

	if (wwin)
	    OpenWindowMenu(wwin, wwin->frame_x, 
			   wwin->frame_y+wwin->frame->top_width, True);
	
    } else {
	return False;
    }

    return True;
}


static void
performCommand(WScreen *scr, char *command, XClientMessageEvent *event)
{
    assert(scr != NULL);
    
    if (strcmp(command, "commandLine")==0
       || strcmp(command, "execute")==0) {
	char *cmd;

	cmd = ExpandOptions(scr, _("%a(Run Command,Type the command to run:)"));
	if (cmd) {
	    ExecuteShellCommand(scr, cmd);
	    free(cmd);
	}
    } else if (strcmp(command, "logout")==0) {

	Shutdown(WSLogoutMode);

    } else if (strcmp(command, "refreshScreen")==0) {

	wRefreshDesktop(scr);

    } else if (strncmp(command, "go:", 3)==0) {

	Shutdown(WSRestartPreparationMode);
	Restart(&command[3]);

    } else if (strcmp(command, "desktop+1")==0) {

	wWorkspaceRelativeChange(scr, 1);

    } else if (strcmp(command, "desktop-1")==0) {

	wWorkspaceRelativeChange(scr, -1);

    } else if (strcmp(command, "desktop+2")==0) {

	wWorkspaceRelativeChange(scr, 2);

    } else if (strcmp(command, "desktop-2")==0) {

	wWorkspaceRelativeChange(scr, -2);

    } else if (strcmp(command, "desktop%%2")==0) {

	if (scr->current_workspace % 2 == 1)
	    wWorkspaceRelativeChange(scr, 1);
	else
	    wWorkspaceRelativeChange(scr, -1);
    } else if (strncmp(command, "desktop", 7)==0) {
	int ws;

	ws = atoi(&command[7]);
	wWorkspaceChange(scr, ws);

	/* wmaker specific stuff */
    } else if (strcmp(command, "wmaker:info")==0) {

	wShowInfoPanel(scr);

    } else if (strcmp(command, "wmaker:legal")==0) {

	wShowLegalPanel(scr);

    } else if (strcmp(command, "wmaker:arrangeIcons")==0) {

	wArrangeIcons(scr, True);

    } else if (strcmp(command, "wmaker:showAll")==0) {

	wShowAllWindows(scr);

    } else if (strcmp(command, "wmaker:hideOthers")==0) {

	wHideOtherApplications(scr->focused_window);

    } else if (strcmp(command, "wmaker:restart")==0) {

	Shutdown(WSRestartPreparationMode);
	Restart(NULL);

    } else if (strcmp(command, "wmaker:exit")==0) {

	Shutdown(WSExitMode);

#ifdef UNSUPPORTED_STUFF
    } else if (strcmp(command, "moduleRaised")==0) { /* useless */
    } else if (strcmp(command, "deskUnclutter")==0) {
    } else if (strcmp(command, "deskCascade")==0) {
    } else if (strcmp(command, "configure")==0) {
    } else if (strcmp(command, "taskManager")==0) {
    } else if (strcmp(command, "darkenScreen")==0) { /* breaks consistency */
#endif
    } else if (!performWindowCommand(scr, command)) {
	KWMModuleList *module;
	long mask = 0;
	XEvent ev;
	/* do message relay thing */

	ev.xclient = *event;
	for (module = KWMModules; module != NULL; module = module->next) {

	    ev.xclient.window = module->window;
	    if (module->window == scr->root_win)
		mask = SubstructureRedirectMask;
	    else
		mask = 0;

	    XSendEvent(dpy, module->window, False, mask, &ev);
	}
    }
}


Bool
wKWMProcessClientMessage(XClientMessageEvent *event)
{
    Bool processed = True;
    WScreen *scr;
#ifdef DEBUG1
    printf("CLIENT MESS %s\n", XGetAtomName(dpy, event->message_type));
#endif
    if (event->message_type == _XA_KWM_COMMAND && event->format==8) {
	char buffer[24];
	int i;

	scr = wScreenForRootWindow(event->window);

	for (i=0; i<20; i++) {
	    buffer[i] = event->data.b[i];
	}
	buffer[i] = 0;

#ifdef DEBUG1
	printf("got KDE command %s\n", buffer);
#endif
	performCommand(scr, buffer, event);

    } else if (event->message_type == _XA_KWM_ACTIVATE_WINDOW) {
	WWindow *wwin;

#ifdef DEBUG1
	printf("got KDE activate internal\n");
#endif
	wwin = wWindowFor(event->data.l[0]);

	if (wwin)
	    wSetFocusTo(wwin->screen_ptr, wwin);

    } else if (event->message_type == _XA_KWM_DO_NOT_MANAGE 
	       && event->format == 8) {
	KWMDoNotManageList *node;
	int i;

#ifdef DEBUG1
	printf("got KDE dont manage\n");
#endif

	node = malloc(sizeof(KWMDoNotManageList));
	if (!node) {
	    wwarning("out of memory processing KWM_DO_NOT_MANAGE message");
	}
	for (i=0; i<20 && event->data.b[i]; i++)
	    node->title[i] = event->data.b[i];
	node->title[i] = 0;

	node->next = KWMDoNotManageCrap;
	KWMDoNotManageCrap = node;

    } else if (event->message_type == _XA_KWM_MODULE) {
	long val;
	Window modwin = event->data.l[0];

	scr = wScreenForRootWindow(event->window);

	if (getSimpleHint(modwin, _XA_KWM_MODULE, &val) && val) {
#ifdef DEBUG1
	    puts("got KDE module startup");
#endif
	    addModule(scr, modwin);
	} else {
#ifdef DEBUG1
	    puts("got KDE module finish");
#endif
	    removeModule(scr, modwin);
	}
    } else {
	processed = False;
    }
    
    return processed;
}


void
wKWMCheckModule(WScreen *scr, Window window)
{
    long val;

    if (getSimpleHint(window, _XA_KWM_MODULE, &val) && val) {
#ifdef DEBUG1
	puts("got KDE module startup");
#endif
	addModule(scr, window);
    }
}


Bool
wKWMCheckRootHintChange(WScreen *scr, XPropertyEvent *event)
{
    Bool processed = True;
    long value;

    if (event->atom == _XA_KWM_CURRENT_DESKTOP) {
	if (getSimpleHint(scr->root_win, _XA_KWM_CURRENT_DESKTOP, &value)) {
#ifdef DEBUG1
	    printf("got KDE workspace switch to %li\n", value);
#endif
	    if (value-1 != scr->current_workspace) {
		wWorkspaceChange(scr, value-1);
	    }
	}
    } else if (event->atom == _XA_KWM_NUMBER_OF_DESKTOPS) {
#ifdef DEBUG1
	    printf("got KDE workspace number change\n");
#endif

	if (getSimpleHint(scr->root_win, _XA_KWM_NUMBER_OF_DESKTOPS, &value)) {

	    /* increasing is easy... */
	    if (value > scr->workspace_count) {
		scr->flags.kwm_syncing_count = 1;

		wWorkspaceMake(scr, value - scr->workspace_count);

		scr->flags.kwm_syncing_count = 0;

	    } else if (value < scr->workspace_count) {
		int i;
		Bool rebuild = False;

		scr->flags.kwm_syncing_count = 1;

		/* decrease all we can do */
		for (i = scr->workspace_count; i >= value; i--) {
		    if (!wWorkspaceDelete(scr, i)) {
			rebuild = True;
			break;
		    }
		}

		scr->flags.kwm_syncing_count = 0;

		/* someone destroyed a workspace that can't be destroyed. 
		 * Reset the hints to reflect our internal state.
		 */
		if (rebuild) {
		    wKWMUpdateWorkspaceCountHint(scr);
		}
	    }
	}
    } else {
	int i;

	processed = False;

	for (i = 0; i < MAX_WORKSPACES; i++) {
	     if (event->atom == _XA_KWM_DESKTOP_NAME_[i]) {
		 char *name;

		 name = wKWMGetWorkspaceName(scr, i);

#ifdef DEBUG1
		 printf("got KDE workspace name change to %s\n", name);
#endif

		 if (name && strncmp(name, scr->workspaces[i]->name, 
				     MAX_WORKSPACENAME_WIDTH)!=0) {
		     scr->flags.kwm_syncing_name = 1;
		     wWorkspaceRename(scr, i, name);
		     scr->flags.kwm_syncing_name = 0;
		 }
		 if (name)
		     XFree(name);
		 processed = True;
		 break;
	     } else if (event->atom == _XA_KWM_WINDOW_REGION_[i]) {
		 WArea area;

		 if (getAreaHint(scr->root_win, event->atom, &area)) {
			    
		     if (scr->totalUsableArea.x1 != area.x1
			 || scr->totalUsableArea.y1 != area.y1
			 || scr->totalUsableArea.x2 != area.x2
			 || scr->totalUsableArea.y2 != area.y2) {
			 wScreenUpdateUsableArea(scr);
		     }
		 }

		 processed = True;
		 break;
	     }
	}
    }

    return processed;
}


Bool
wKWMManageableClient(WScreen *scr, Window win, char *title)
{
    KWMDoNotManageList *ptr, *next;
    long val;

    if (getSimpleHint(win, _XA_KWM_DOCKWINDOW, &val) && val) {
	addDockWindow(scr, win);
	return False;
    }
    
    ptr = KWMDoNotManageCrap;
    /*
     * TODO: support for glob patterns or regexes
     */
    if (ptr && strncmp(ptr->title, title, strlen(ptr->title))==0) {
	next = ptr->next;
	free(ptr);
	KWMDoNotManageCrap = next;
#ifdef DEBUG1
	printf("window %s not managed per KDE request\n", title);
#endif

	sendToModules(scr, _XA_KWM_MODULE_WIN_ADD, NULL, win);
	sendToModules(scr, _XA_KWM_MODULE_WIN_REMOVE, NULL, win);

	return False;
    } else if (ptr) {
	while (ptr->next) {
	    if (strncmp(ptr->next->title, title, strlen(ptr->next->title))==0) {
#ifdef DEBUG1
		printf("window %s not managed per KDE request\n", title);
#endif
		next = ptr->next->next;
		free(ptr->next);
		ptr->next = next;

		sendToModules(scr, _XA_KWM_MODULE_WIN_ADD, NULL, win);
		sendToModules(scr, _XA_KWM_MODULE_WIN_REMOVE, NULL, win);

		return False;
	    }

	    ptr = ptr->next;
	}
    }

    return True;
}


void
wKWMUpdateCurrentWorkspaceHint(WScreen *scr)
{
    setSimpleHint(scr->root_win, _XA_KWM_CURRENT_DESKTOP,
		  scr->current_workspace+1);

    sendToModules(scr, _XA_KWM_MODULE_DESKTOP_CHANGE, NULL,
		  scr->current_workspace+1);
}


void
wKWMUpdateActiveWindowHint(WScreen *scr)
{
    long val;

    if (!scr->focused_window || !scr->focused_window->flags.focused)
	val = None;
    else
	val = (long)(scr->focused_window->client_win);

    XChangeProperty(dpy, scr->root_win, _XA_KWM_ACTIVE_WINDOW,
		    _XA_KWM_ACTIVE_WINDOW, 32, PropModeReplace, 
		    (unsigned char*)&val, 1);
    XFlush(dpy);
}


void
wKWMUpdateWorkspaceCountHint(WScreen *scr)
{
    if (scr->flags.kwm_syncing_count)
	return;

    setSimpleHint(scr->root_win, _XA_KWM_NUMBER_OF_DESKTOPS,
		  scr->workspace_count);

    sendToModules(scr, _XA_KWM_MODULE_DESKTOP_NUMBER_CHANGE, NULL,
		  scr->workspace_count);
}


void
wKWMCheckDestroy(XDestroyWindowEvent *event)
{
    WScreen *scr;

    if (event->event == event->window) {
	return;
    }

    scr = wScreenSearchForRootWindow(event->event);
    if (!scr) {
	return;
    }

    removeModule(scr, event->window);
    removeDockWindow(scr, event->window);
}


void
wKWMUpdateWorkspaceNameHint(WScreen *scr, int workspace)
{
    char buffer[64];

    assert(workspace >= 0 && workspace < MAX_WORKSPACES);

    if (_XA_KWM_DESKTOP_NAME_[workspace]==0) {
	sprintf(buffer, "KWM_DESKTOP_NAME_%d", workspace + 1);

	_XA_KWM_DESKTOP_NAME_[workspace] = XInternAtom(dpy, buffer, False);
    }

    XChangeProperty(dpy, scr->root_win, _XA_KWM_DESKTOP_NAME_[workspace],
		    XA_STRING, 8, PropModeReplace, 
		    (unsigned char*)scr->workspaces[workspace]->name,
		    strlen(scr->workspaces[workspace]->name)+1);

    sendToModules(scr, _XA_KWM_MODULE_DESKTOP_NAME_CHANGE, NULL, workspace+1);
}



void
wKWMUpdateClientWorkspace(WWindow *wwin)
{
#ifdef DEBUG1
    printf("updating workspace of %s to %i\n",
	   wwin->frame->title, wwin->frame->workspace+1);
#endif
    setSimpleHint(wwin->client_win, _XA_KWM_WIN_DESKTOP, 
		  wwin->frame->workspace+1);
}


void
wKWMUpdateClientGeometryRestore(WWindow *wwin)
{
    WArea rect;
    
    rect.x1 = wwin->old_geometry.x;
    rect.y1 = wwin->old_geometry.y;
    rect.x2 = wwin->old_geometry.x + wwin->old_geometry.width;
    rect.y2 = wwin->old_geometry.y + wwin->old_geometry.height;
    
    setAreaHint(wwin->client_win, _XA_KWM_WIN_GEOMETRY_RESTORE, rect);
}


void
wKWMUpdateClientStateHint(WWindow *wwin, WKWMStateFlag flags)
{
    if (flags & KWMIconifiedFlag) {
	setSimpleHint(wwin->client_win, _XA_KWM_WIN_ICONIFIED, 
		      wwin->flags.miniaturized /*|| wwin->flags.shaded
		      || wwin->flags.hidden*/);
    }
    if (flags & KWMStickyFlag) {
	setSimpleHint(wwin->client_win, _XA_KWM_WIN_STICKY, 
		      IS_OMNIPRESENT(wwin));
    }
    if (flags & KWMMaximizedFlag) {
	setSimpleHint(wwin->client_win, _XA_KWM_WIN_MAXIMIZED, 
		      wwin->flags.maximized!=0);
    }
}


Bool
wKWMGetUsableArea(WScreen *scr, WArea *area)
{
    char buffer[64];

    if (_XA_KWM_WINDOW_REGION_[0]==0) {
	sprintf(buffer, "KWM_WINDOW_REGION_%d", 1);

	_XA_KWM_WINDOW_REGION_[0] = XInternAtom(dpy, buffer, False);
    }

    return getAreaHint(scr->root_win, _XA_KWM_WINDOW_REGION_[0], area);
}


#ifdef not_used
void
wKWMSetUsableAreaHint(WScreen *scr, int workspace)
{
    /* if we set this after making changes of our own to the area,
     * the next time the area changes, we won't know what should
     * be the new final area. This protocol isn't worth a shit :/
     */
/*
 * According to Matthias Ettrich:  
 * Indeed, there's no protocol to deal with the area yet in case several
 * clients want to influence it. It is sufficent, though, if it is clear
 * that one process is responsable for the area. For KDE this is kpanel, but
 * I see that there might be a conflict with the docking area of windowmaker
 * itself.
 * 
 */

#ifdef notdef
    char buffer[64];

    assert(workspace >= 0 && workspace < MAX_WORKSPACES);

    if (_XA_KWM_WINDOW_REGION_[workspace]==0) {
	sprintf(buffer, "KWM_WINDOW_REGION_%d", workspace+1);

	_XA_KWM_WINDOW_REGION_[workspace] = XInternAtom(dpy, buffer, False);
    }

    setAreaHint(scr->root_win, _XA_KWM_WINDOW_REGION_[workspace],
		scr->totalUsableArea);
#endif
}
#endif /* not_used */

void
wKWMSendEventMessage(WWindow *wwin, WKWMEventMessage message)
{
    Atom msg;

    if (wwin && (wwin->flags.internal_window
		 || wwin->flags.kwm_hidden_for_modules
		 || WFLAGP(wwin, skip_window_list)))
	return;

    switch (message) {
     case WKWMAddWindow:
	msg = _XA_KWM_MODULE_WIN_ADD;
	break;
     case WKWMRemoveWindow:
	msg = _XA_KWM_MODULE_WIN_REMOVE;
	break;
     case WKWMFocusWindow:
	msg = _XA_KWM_MODULE_WIN_ACTIVATE;
	break;
     case WKWMRaiseWindow:
	msg = _XA_KWM_MODULE_WIN_RAISE;
	break;
     case WKWMLowerWindow:
	msg = _XA_KWM_MODULE_WIN_LOWER;
	break;
     case WKWMChangedClient:
	msg = _XA_KWM_MODULE_WIN_CHANGE;
	break;
     case WKWMIconChange:
	msg = _XA_KWM_MODULE_WIN_ICON_CHANGE;
	break;
     default:
	return;
    }

    sendToModules(wwin ? wwin->screen_ptr : NULL, msg, wwin, 0);
}


#endif /* KWM_HINTS */
