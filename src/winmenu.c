/* winmenu.c - command menu for windows
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "WindowMaker.h"
#include "actions.h"
#include "menu.h"
#include "funcs.h" 
#include "window.h"
#include "client.h"
#include "application.h"
#include "keybind.h"
#include "framewin.h"
#include "workspace.h"
#include "winspector.h"
#include "dialog.h"

#define MC_MAXIMIZE	0
#define MC_MINIATURIZE	1
#define MC_SHADE	2
#define MC_HIDE		3
#define MC_HIDE_OTHERS	4
#define MC_SELECT       5
#define MC_DUMMY_MOVETO	6
#define MC_PROPERTIES   7

#define MC_CLOSE        8
#define MC_KILL         9



/**** Global data ***/
extern Time LastTimestamp;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;

extern WShortKey wKeyBindings[WKBD_LAST];

extern WPreferences wPreferences;

static void
execMenuCommand(WMenu *menu, WMenuEntry *entry)
{
    WWindow *wwin = (WWindow*)entry->clientdata;
    WApplication *wapp;
    
    CloseWindowMenu(menu->frame->screen_ptr);

    switch (entry->order) {
     case MC_CLOSE:
	/* send delete message */
	wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW, LastTimestamp);
	break;
     case MC_KILL:
	wretain(wwin);
	if (wPreferences.dont_confirm_kill
	    || wMessageDialog(menu->frame->screen_ptr, _("Kill Application"),
			   _("This will kill the application.\nAny unsaved changes will be lost.\nPlease confirm."),
			   _("Yes"), _("No"), NULL)==WAPRDefault) {
	    if (!wwin->flags.destroyed)
		wClientKill(wwin);
	}
	wrelease(wwin);
	break;
     case MC_MINIATURIZE:
	if (wwin->protocols.MINIATURIZE_WINDOW) {
	    wClientSendProtocol(wwin, _XA_GNUSTEP_WM_MINIATURIZE_WINDOW,
				LastTimestamp);
	} else {
	    wIconifyWindow(wwin);
	}
	break;
     case MC_MAXIMIZE:
	if (wwin->flags.maximized)
	    wUnmaximizeWindow(wwin);
	else
	    wMaximizeWindow(wwin, MAX_VERTICAL|MAX_HORIZONTAL);
	break;
     case MC_SHADE:
	if (wwin->flags.shaded)
	    wUnshadeWindow(wwin);
	else
	    wShadeWindow(wwin);
	break;
     case MC_SELECT:
        wSelectWindow(wwin);
	break;
     case MC_PROPERTIES:
	if (wwin->wm_class || wwin->wm_instance)
	    wShowInspectorForWindow(wwin);
	break;
	
     case MC_HIDE:
	wapp = wApplicationOf(wwin->main_window);
	wHideApplication(wapp);
	break;
     case MC_HIDE_OTHERS:
	wHideOtherApplications(wwin);
	break;
    }
}


static void
switchWSCommand(WMenu *menu, WMenuEntry *entry)
{
    WWindow *wwin = (WWindow*)entry->clientdata;

    if (wwin->flags.selected)
	wSelectWindow(wwin);
    wWindowChangeWorkspace(wwin, entry->order);
}


static void
updateWorkspaceMenu(WMenu *menu)
{
    WScreen *scr = menu->frame->screen_ptr;
    char title[MAX_WORKSPACENAME_WIDTH+1];
    int i;
    
    if (!menu)
	return;
    
    for (i=0; i<scr->workspace_count; i++) {
	if (i < menu->entry_no) {
	    if (strcmp(menu->entries[i]->text,scr->workspaces[i]->name)!=0) {
		free(menu->entries[i]->text);
		strcpy(title, scr->workspaces[i]->name);
		menu->entries[i]->text = wstrdup(title);
		menu->flags.realized = 0;
	    }
	} else {
	    strcpy(title, scr->workspaces[i]->name);

	    wMenuAddCallback(menu, title, switchWSCommand, NULL);
	    
	    menu->flags.realized = 0;
	}
    }
    
    if (!menu->flags.realized)
	wMenuRealize(menu);
}


static WMenu*
makeWorkspaceMenu(WScreen *scr)
{
    WMenu *menu;
    
    menu = wMenuCreate(scr, NULL, False);
    if (!menu)
	wwarning(_("could not create workspace submenu for window menu"));

    updateWorkspaceMenu(menu);
    
    return menu;
}


static WMenu*
createWindowMenu(WScreen *scr)
{
    WMenu *menu;
    KeyCode kcode;
    WMenuEntry *entry;
    char *tmp;

    menu = wMenuCreate(scr, NULL, False);
    /* 
     * Warning: If you make some change that affects the order of the 
     * entries, you must update the command #defines in the top of
     * this file.
     */
    entry = wMenuAddCallback(menu, _("(Un)Maximize"), execMenuCommand, NULL);
    if (wKeyBindings[WKBD_MAXIMIZE].keycode!=0) {
	kcode = wKeyBindings[WKBD_MAXIMIZE].keycode;
	
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }

    entry = wMenuAddCallback(menu, _("Miniaturize"), execMenuCommand, NULL);
    
    if (wKeyBindings[WKBD_MINIATURIZE].keycode!=0) {
	kcode = wKeyBindings[WKBD_MINIATURIZE].keycode;
	
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }
    
    entry = wMenuAddCallback(menu, _("(Un)Shade"), execMenuCommand, NULL);
    if (wKeyBindings[WKBD_SHADE].keycode!=0) {
	kcode = wKeyBindings[WKBD_SHADE].keycode;
	
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }
    
    entry = wMenuAddCallback(menu, _("Hide"), execMenuCommand, NULL);
    if (wKeyBindings[WKBD_HIDE].keycode!=0) {
	kcode = wKeyBindings[WKBD_HIDE].keycode;
	
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }
    entry = wMenuAddCallback(menu, _("Hide Others"), execMenuCommand, NULL);

    entry = wMenuAddCallback(menu, _("Select"), execMenuCommand, NULL);
    if (wKeyBindings[WKBD_SELECT].keycode!=0) {
	kcode = wKeyBindings[WKBD_SELECT].keycode;
	
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }
    
    entry = wMenuAddCallback(menu, _("Move To"), NULL, NULL);
    scr->workspace_submenu = makeWorkspaceMenu(scr);
    if (scr->workspace_submenu)
	wMenuEntrySetCascade(menu, entry, scr->workspace_submenu);

    entry = wMenuAddCallback(menu, _("Attributes..."), execMenuCommand, NULL);
    
    entry = wMenuAddCallback(menu, _("Close"), execMenuCommand, NULL);
    if (wKeyBindings[WKBD_CLOSE].keycode!=0) {
	kcode = wKeyBindings[WKBD_CLOSE].keycode;
	if (kcode && (tmp = XKeysymToString(XKeycodeToKeysym(dpy, kcode, 0))))
	  entry->rtext = wstrdup(tmp);
    }
    
    entry = wMenuAddCallback(menu, _("Kill"), execMenuCommand, NULL);

    return menu;
}


void
CloseWindowMenu(WScreen *scr)
{
    if (scr->window_menu && scr->window_menu->flags.mapped)
      wMenuUnmap(scr->window_menu);

}


void
OpenWindowMenu(WWindow *wwin, int x, int y, int keyboard)
{
    WMenu *menu;
    WApplication *wapp = wApplicationOf(wwin->main_window);
    WScreen *scr = wwin->screen_ptr;
    int i;

    if (!scr->window_menu) {
	scr->window_menu = createWindowMenu(scr);
    } else {
	updateWorkspaceMenu(scr->workspace_submenu);
    }
    menu = scr->window_menu;
    if (menu->flags.mapped) {
	wMenuUnmap(menu);
	if (menu->entries[0]->clientdata==wwin) {
	    return;
	}
    }

    wMenuSetEnabled(menu, MC_HIDE, wapp!=NULL 
		    && !wapp->main_window_desc->window_flags.no_appicon);
    
    wMenuSetEnabled(menu, MC_CLOSE, 
		    (wwin->protocols.DELETE_WINDOW
		     && !wwin->window_flags.no_closable));
    
    wMenuSetEnabled(menu, MC_MINIATURIZE, !wwin->window_flags.no_miniaturizable);

    wMenuSetEnabled(menu, MC_SHADE, !wwin->window_flags.no_shadeable);

    wMenuSetEnabled(menu, MC_DUMMY_MOVETO, !wwin->window_flags.omnipresent);
    
    if ((wwin->wm_class || wwin->wm_instance) && !wwin->flags.inspector_open) {
	wMenuSetEnabled(menu, MC_PROPERTIES, True);
    } else {
	wMenuSetEnabled(menu, MC_PROPERTIES, False);
    }
    
    /* set the client data of the entries to the window */
    for (i = 0; i < menu->entry_no; i++) {
	menu->entries[i]->clientdata = wwin;
    }
    for (i = 0; i < scr->workspace_submenu->entry_no; i++) {
	scr->workspace_submenu->entries[i]->clientdata = wwin;
	if (i == scr->current_workspace) {
	    wMenuSetEnabled(scr->workspace_submenu, i, False);
	} else {
	    wMenuSetEnabled(scr->workspace_submenu, i, True);
	}
    }

    if (!menu->flags.realized)
	wMenuRealize(menu);
    
    x -= menu->frame->core->width/2;
    if (x + menu->frame->core->width > wwin->frame_x+wwin->frame->core->width)
	x = wwin->frame_x+wwin->frame->core->width - menu->frame->core->width;
    if (x < wwin->frame_x) 
	x = wwin->frame_x;

    if (!wwin->flags.internal_window)
	wMenuMapAt(menu, x, y, keyboard);
}
