/*
 * openlook.c - OPEN LOOK (tm) compatibility stuff
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
 * Semantics and hint information taken from olwm code
 */

#include "wconfig.h"

#ifdef OLWM_HINTS


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "WindowMaker.h"

#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "properties.h"
#include "icon.h"
#include "client.h"
#include "funcs.h"

#include "openlook.h"


/* pin states */
#define OL_PIN_OUT 0
#define OL_PIN_IN 1

/* flags */
#define OL_DECORATION_HEADER      (1<<0)
#define OL_DECORATION_FOOTER      (1<<1)
#define OL_DECORATION_PUSHPIN     (1<<2)
#define OL_DECORATION_CLOSEBUTTON (1<<3)
#define OL_DECORATION_RESIZEABLE  (1<<4)
#define OL_DECORATION_ICONNAME    (1<<5)
#define OL_DECORATION_WARPTOPIN   (1<<6)
#define OL_DECORATION_NONE        (1<<7)



typedef struct {
    long flags;
    Atom winType;
    Atom menuType;
    long pinInitState;
    long cancel;
} OLHints;

#define OL_WINTYPE	(1<<0)
#define OL_MENUTYPE	(1<<1)
#define OL_PINSTATE	(1<<2)
#define OL_CANCEL	(1<<3)



typedef struct {
    unsigned used:1;

    unsigned semantic:1;

    unsigned semantic_compose:1;
    unsigned semantic_capslock:1;
    unsigned semantic_numlock:1;
    unsigned semantic_scrolllock:1;
} WOLWindowState;


static Atom _XA_SUN_WM_PROTOCOLS = 0;

#ifdef unused
static Bool
getWindowState(Window win, OLWindowState *state)
{
    static Atom _XA_SUN_WINDOW_STATE = 0;
    unsigned long *data;

    if (!_XA_SUN_WINDOW_STATE) {
	_XA_SUN_WINDOW_STATE = XInternAtom(dpy, "_SUN_WINDOW_STATE", False);
    }

    data = (unsigned long*)PropGetCheckProperty(win, _XA_SUN_WINDOW_STATE,
						XA_INTEGER, 32, 2, NULL);

    if (!data) {
	return False;
    }
    
    state->flags = data[0];
    state->state = data[1];

    XFree(data);
    
    return True;
}
#endif

static Bool
getWindowHints(Window window, OLHints *hints)
{
    long *data;
    int count;
    static Atom _XA_OL_WIN_ATTR = 0;

    if (!_XA_OL_WIN_ATTR) {
	_XA_OL_WIN_ATTR = XInternAtom(dpy, "_OL_WIN_ATTR", False);
    }

    data = (long*)PropGetCheckProperty(window, _XA_OL_WIN_ATTR, 
				       _XA_OL_WIN_ATTR, 32, 0, &count);

    if (!data)
	return False;

    if (count == 3) {
	/* old format */

	hints->flags = OL_WINTYPE|OL_MENUTYPE|OL_PINSTATE;
	hints->winType = data[0];
	hints->menuType = data[1];
	hints->pinInitState = data[2];
	hints->cancel = 0;

    } else if (count == 5) {
	/* new format */

	hints->flags = data[0];
	hints->winType = data[1];
	hints->menuType = data[2];
	hints->pinInitState = data[3];
	hints->cancel = data[4];

    } else {
	XFree(data);
	return False;
    }

    XFree(data);

    /* do backward compatibility stuff */
    if (hints->flags & OL_PINSTATE) {
	static Atom pinIn = 0, pinOut;
	
	if (!pinIn) {
	    pinIn = XInternAtom(dpy, "_OL_PIN_IN", False);
	    pinOut = XInternAtom(dpy, "_OL_PIN_OUT", False);
	}
	
	if (hints->pinInitState == pinIn)
	    hints->pinInitState = OL_PIN_IN;
	else if (hints->pinInitState == pinOut)
	    hints->pinInitState = OL_PIN_OUT;
    }

    return True;
}




static void
applyDecorationHints(Window win, int *flags)
{
    Atom *atoms;
    static Atom _XA_OL_DECOR_ADD = 0;
    static Atom _XA_OL_DECOR_DEL = 0;
    static Atom _XA_CLOSE, _XA_FOOTER, _XA_RESIZE, _XA_HEADER, _XA_PIN,
	_XA_ICONNAME;
    int count;
    int i;

    if (!_XA_OL_DECOR_DEL) {
	_XA_OL_DECOR_ADD = XInternAtom(dpy, "_OL_DECOR_ADD", False);
	_XA_OL_DECOR_DEL = XInternAtom(dpy, "_OL_DECOR_DEL", False);

	_XA_CLOSE = XInternAtom(dpy, "_OL_DECOR_CLOSE", False);
	_XA_FOOTER = XInternAtom(dpy, "_OL_DECOR_FOOTER", False);
	_XA_RESIZE = XInternAtom(dpy, "_OL_DECOR_RESIZE", False);
	_XA_HEADER = XInternAtom(dpy, "_OL_DECOR_HEADER", False);
	_XA_PIN = XInternAtom(dpy, "_OL_DECOR_PIN", False);
	_XA_ICONNAME = XInternAtom(dpy, "_OL_DECOR_ICON_NAME", False);
    }

    atoms = (Atom*)PropGetCheckProperty(win, _XA_OL_DECOR_ADD, XA_ATOM, 32, 0, 
					&count);
    if (atoms) {
	for (i=0; i < count; i++) {
	    if (atoms[i] == _XA_CLOSE)
		*flags |= OL_DECORATION_CLOSEBUTTON;
	    else if (atoms[i] == _XA_FOOTER)
		*flags |= OL_DECORATION_FOOTER;
	    else if (atoms[i] == _XA_RESIZE)
		*flags |= OL_DECORATION_RESIZEABLE;
	    else if (atoms[i] == _XA_HEADER)
		*flags |= OL_DECORATION_HEADER;
	    else if (atoms[i] == _XA_PIN)
		*flags |= OL_DECORATION_PUSHPIN;
	    else if (atoms[i] == _XA_ICONNAME)
		*flags |= OL_DECORATION_ICONNAME;
	}
	XFree(atoms);
    }

    atoms = (Atom*)PropGetCheckProperty(win, _XA_OL_DECOR_DEL, XA_ATOM, 32, 0, 
					&count);
    if (atoms) {
	for (i=0; i < count; i++) {
	    if (atoms[i] == _XA_CLOSE)
		*flags &= ~OL_DECORATION_CLOSEBUTTON;
	    else if (atoms[i] == _XA_FOOTER)
		*flags &= ~OL_DECORATION_FOOTER;
	    else if (atoms[i] == _XA_RESIZE)
		*flags &= ~OL_DECORATION_RESIZEABLE;
	    else if (atoms[i] == _XA_HEADER)
		*flags &= ~OL_DECORATION_HEADER;
	    else if (atoms[i] == _XA_PIN)
		*flags &= ~OL_DECORATION_PUSHPIN;
	    else if (atoms[i] == _XA_ICONNAME)
		*flags &= ~OL_DECORATION_ICONNAME;
	}
	XFree(atoms);
    }
}


void
wOLWMInitStuff(WScreen *scr)
{
    static Atom _SUN_OL_WIN_ATTR_5;

    if (!_XA_SUN_WM_PROTOCOLS) {
	_XA_SUN_WM_PROTOCOLS = XInternAtom(dpy, "_SUN_WM_PROTOCOLS", False);
	_SUN_OL_WIN_ATTR_5 = XInternAtom(dpy, "_SUN_OL_WIN_ATTR_5", False);
    }

    XChangeProperty(dpy, scr->root_win, _XA_SUN_WM_PROTOCOLS, XA_ATOM, 32,
		    PropModeReplace, (unsigned char*)&_SUN_OL_WIN_ATTR_5, 1);
}


void
wOLWMChangePushpinState(WWindow *wwin, Bool state)
{
    static Atom pinState = 0;

    if (!pinState) {
	pinState = XInternAtom(dpy, "_OL_PIN_STATE", False);
    }

    XChangeProperty(dpy, wwin->client_win, pinState, XA_INTEGER, 32,
		    PropModeReplace, (unsigned char *)&state, 1);
}


void
wOLWMShutdown(WScreen *scr)
{
    XDeleteProperty(dpy, scr->root_win, _XA_SUN_WM_PROTOCOLS);
}


#ifdef unfinished
void
wOLWMUpdateWindowState(WWindow *wwin)
{
    if (wwin->ol_window_state.used) {
	if (wwin->ol_window_state.semantic) {
	    if (wwin->ol_window_state.semantic_compose)
		setComposeLed(True);
	    else
		setComposeLed(False);
	}
    } else {
	setComposeLed(False);
    }
}
#endif /* unfinished */

void
wOLWMCheckClientHints(WWindow *wwin)
{
    OLHints hints;
    static Atom WT_BASE = 0, WT_CMD, WT_NOTICE, WT_HELP, WT_OTHER;
    static Atom MT_FULL, MT_LIMITED, MT_NONE;
    int decoration;
    int pinInitState = OL_PIN_IN;
    Atom menuType;

    if (!WT_BASE) {
	WT_BASE = XInternAtom(dpy, "_OL_WT_BASE", False);
	WT_CMD = XInternAtom(dpy, "_OL_WT_CMD", False);
	WT_NOTICE = XInternAtom(dpy, "_OL_WT_NOTICE", False);
	WT_HELP = XInternAtom(dpy, "_OL_WT_HELP", False);
	WT_OTHER = XInternAtom(dpy, "_OL_WT_OTHER", False);

	MT_FULL = XInternAtom(dpy, "_OL_MENU_FULL", False);
	MT_LIMITED = XInternAtom(dpy, "_OL_MENU_LIMITED", False);
	MT_NONE = XInternAtom(dpy, "_OL_NONE", False);
    }

    /* get attributes */

    if (!getWindowHints(wwin->client_win, &hints) ||
	!(hints.flags & OL_WINTYPE)) {

	decoration = OL_DECORATION_CLOSEBUTTON|OL_DECORATION_RESIZEABLE
	    |OL_DECORATION_HEADER|OL_DECORATION_ICONNAME;

	menuType = MT_FULL;

    } else {
	if (hints.winType == WT_BASE) {

	    decoration = OL_DECORATION_CLOSEBUTTON|OL_DECORATION_RESIZEABLE
		|OL_DECORATION_HEADER|OL_DECORATION_ICONNAME;

	    menuType = MT_FULL;

	} else if (hints.winType == WT_CMD) {

	    decoration = OL_DECORATION_PUSHPIN|OL_DECORATION_RESIZEABLE
		|OL_DECORATION_HEADER|OL_DECORATION_ICONNAME;

	    menuType = MT_LIMITED;

	} else if (hints.winType == WT_NOTICE) {

	    decoration = OL_DECORATION_ICONNAME;
	    menuType = MT_NONE;

	} else if (hints.winType == WT_HELP) {

	    decoration = OL_DECORATION_PUSHPIN|OL_DECORATION_HEADER
		|OL_DECORATION_ICONNAME|OL_DECORATION_WARPTOPIN;
	    menuType = MT_LIMITED;
	    
	} else if (hints.winType == WT_OTHER) {

	    decoration = OL_DECORATION_ICONNAME;
	    menuType = MT_NONE;

	    if (hints.flags & OL_MENUTYPE) {
		menuType = hints.menuType;
	    }
	}

	if (hints.flags & OL_PINSTATE) {
	    pinInitState = hints.pinInitState;
	} else {
	    pinInitState = OL_PIN_OUT;
	}
    }

    /* mask attributes with decoration hints */
    applyDecorationHints(wwin->client_win, &decoration);

    if ((decoration & OL_DECORATION_CLOSEBUTTON)
	&& (decoration & OL_DECORATION_PUSHPIN))
	decoration &= ~OL_DECORATION_CLOSEBUTTON;

    if (!(decoration & OL_DECORATION_PUSHPIN))
	decoration &= ~OL_DECORATION_WARPTOPIN;


    /* map the hints to our attributes */
    if (menuType == MT_FULL)
	wwin->flags.olwm_limit_menu = 0;
    else
	wwin->flags.olwm_limit_menu = 1;

    /* this is a transient-like window */
    if (hints.winType == WT_CMD) {
	wwin->client_flags.olwm_transient = 1;
    }

    /*
     * Emulate olwm pushpin.
     * If the initial state of the pin is in, then put the normal close
     * button. If not, make the close button different and when the
     * user moves the window or clicks in the close button, turn it
     * into a normal close button.
     */
    if ((decoration & OL_DECORATION_PUSHPIN) && pinInitState==OL_PIN_OUT) {
	wwin->flags.olwm_push_pin_out = 1;

	wOLWMChangePushpinState(wwin, False);
    } else {
	wOLWMChangePushpinState(wwin, True);
    }

    if (!(decoration & OL_DECORATION_RESIZEABLE)) {
	wwin->client_flags.no_resizable = 1;
	wwin->client_flags.no_resizebar = 1;
    }

    if (decoration & OL_DECORATION_WARPTOPIN) {
	wwin->client_flags.olwm_warp_to_pin = 1;
    }

}
#endif

