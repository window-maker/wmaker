/*
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

#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

#include "WindowMaker.h"
#include "window.h"
#include "client.h"
#include "funcs.h"
#include "properties.h"
#include "winspector.h"


extern Atom _XA_WM_DELETE_WINDOW;
extern Time LastTimestamp;

/*
 *----------------------------------------------------------------------
 * RestoreDesktop--
 * 	Puts the desktop in a usable state when exiting.
 * 
 * Side effects:
 * 	All frame windows are removed and windows are reparented
 * back to root. Windows that are outside the screen are 
 * brought to a viable place. 
 * 
 *---------------------------------------------------------------------- 
 */
void
RestoreDesktop(WScreen *scr)
{
    int i;
    
    XGrabServer(dpy);
    wDestroyInspectorPanels();

    /* reparent windows back to the root window, keeping the stacking order */
    for (i=0; i<MAX_WINDOW_LEVELS; i++) {
        WCoreWindow *core, *next;
        WWindow *wwin;
        
        if (!scr->stacking_list[i])
            continue;
        
        core = scr->stacking_list[i];
        /* go to the end of the list */
        while (core->stacking->under)
            core = core->stacking->under;
        
        while (core) {
            next = core->stacking->above;
            
            if (core->descriptor.parent_type==WCLASS_WINDOW) {
                wwin = core->descriptor.parent;
                wwin->flags.mapped=1;
                wUnmanageWindow(wwin, !wwin->flags.internal_window);
            }
            core = next;
        }
    }

    XUngrabServer(dpy);
    XSetInputFocus(dpy, PointerRoot, RevertToParent, CurrentTime);
    wColormapInstallForWindow(scr, NULL);
    PropCleanUp(scr->root_win);
    XSync(dpy, 0);
}


/*
 *----------------------------------------------------------------------
 * WipeDesktop--
 * 	Kills all windows in a screen. Send DeleteWindow to all windows
 * that support it and KillClient on all windows that don't.
 * 
 * Side effects:
 * 	All managed windows are closed.
 * 
 * TODO: change to XQueryTree()
 *---------------------------------------------------------------------- 
 */
void
WipeDesktop(WScreen *scr)
{
    WWindow *wwin;
    
    wwin = scr->focused_window;
    while (wwin) {
	if (wwin->protocols.DELETE_WINDOW)
	    wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW, LastTimestamp);
	else
	    wClientKill(wwin);
	wwin = wwin->prev;
    }
    XSync(dpy, False);
}

