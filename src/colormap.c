/* colormap.c - colormap handling code
 * 
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
 * 
 *  This code slightly based on fvwm code, 
 *  Copyright (c) Rob Nation and others
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

#include "WindowMaker.h"
#include <X11/Xatom.h>
#include "window.h"



void 
wColormapInstallForWindow(WScreen *scr, WWindow *wwin)
{
    int i;
    XWindowAttributes attributes;
    int done = 0;
    Window xwin = None;

    if(wwin) {
	xwin = wwin->client_win;
    } else {
	xwin = scr->root_win;
    }

    scr->cmap_window = wwin;

    if (scr->root_colormap_install_count > 0) {
	scr->original_cmap_window = wwin;
	return;
    }

    /* install colormap for all windows of the client */
    if (wwin && wwin->cmap_window_no > 0) {
	for (i = wwin->cmap_window_no - 1; i >= 0; i--) {
	    Window w;

	    w = wwin->cmap_windows[i];
	    if (w == wwin->client_win)
		done = 1;

	    XGetWindowAttributes(dpy, w, &attributes);
	    if (attributes.colormap == None)
		attributes.colormap = scr->colormap;

	    if (scr->current_colormap != attributes.colormap) {
		scr->current_colormap = attributes.colormap;
		XInstallColormap(dpy, attributes.colormap);    
	    }
	}
    }

    if (!done) {
	XGetWindowAttributes(dpy, xwin, &attributes);
	if (attributes.colormap == None)
	    attributes.colormap = scr->colormap;

	if (scr->current_colormap != attributes.colormap) {
	    scr->current_colormap = attributes.colormap;
	    XInstallColormap(dpy, attributes.colormap);    
	}
    }
}


void 
wColormapInstallRoot(WScreen *scr)
{
    if (scr->root_colormap_install_count == 0) {
	wColormapInstallForWindow(scr, NULL);
	scr->original_cmap_window = scr->cmap_window;
    }
    scr->root_colormap_install_count++;
}


void
wColormapUninstallRoot(WScreen *scr)
{
    if (scr->root_colormap_install_count > 0)
	scr->root_colormap_install_count--;

    if (scr->root_colormap_install_count == 0) {
	wColormapInstallForWindow(scr, scr->original_cmap_window);
	scr->original_cmap_window = NULL;
    }
}

