/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
 *  Copyright (c) 1998-2003 Dan Pascu
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "wconfig.h"

#include <X11/Xlib.h>

#include <string.h>
#include <unistd.h>

#include "WindowMaker.h"
#include "menu.h"
#include "window.h"
#ifdef USER_MENU
#include "usermenu.h"
#endif				/* USER_MENU */
#include "appicon.h"
#include "application.h"
#include "appmenu.h"
#include "properties.h"
#include "stacking.h"
#include "actions.h"
#include "workspace.h"
#include "dock.h"

/******** Global variables ********/

extern XContext wAppWinContext;
extern XContext wWinContext;
extern WPreferences wPreferences;
extern WDDomain *WDWindowAttributes;

/******** Local variables ********/

static WWindow *makeMainWindow(WScreen * scr, Window window)
{
	WWindow *wwin;
	XWindowAttributes attr;

	if (!XGetWindowAttributes(dpy, window, &attr))
		return NULL;

	wwin = wWindowCreate();
	wwin->screen_ptr = scr;
	wwin->client_win = window;
	wwin->main_window = window;
	wwin->wm_hints = XGetWMHints(dpy, window);

	PropGetWMClass(window, &wwin->wm_class, &wwin->wm_instance);

	wDefaultFillAttributes(scr, wwin->wm_instance, wwin->wm_class,
			       &wwin->user_flags, &wwin->defined_user_flags, True);

	XSelectInput(dpy, window, attr.your_event_mask | PropertyChangeMask | StructureNotifyMask);
	return wwin;
}

WApplication *wApplicationOf(Window window)
{
	WApplication *wapp;

	if (window == None)
		return NULL;
	if (XFindContext(dpy, window, wAppWinContext, (XPointer *) & wapp) != XCSUCCESS)
		return NULL;
	return wapp;
}

static WAppIcon *findDockIconFor(WDock * dock, Window main_window)
{
	WAppIcon *aicon = NULL;

	aicon = wDockFindIconForWindow(dock, main_window);
	if (!aicon) {
		wDockTrackWindowLaunch(dock, main_window);
		aicon = wDockFindIconForWindow(dock, main_window);
	}
	return aicon;
}

static void extractIcon(WWindow * wwin)
{
	char *progname;

	/* Get the application name */
	progname = GetProgramNameForWindow(wwin->client_win);
	if (progname) {
		/* Save the icon path if the application is ".app" */
		wApplicationExtractDirPackIcon(wwin->screen_ptr, progname, wwin->wm_instance, wwin->wm_class);
		wfree(progname);
	}
}

void wApplicationSaveIconPathFor(char *iconPath, char *wm_instance, char *wm_class)
{
	WMPropList *dict = WDWindowAttributes->dictionary;
	WMPropList *adict, *key, *iconk;
	WMPropList *val;
	char *tmp;
	int i;

	i = 0;
	if (wm_instance)
		i += strlen(wm_instance);
	if (wm_class)
		i += strlen(wm_class);

	tmp = wmalloc(i + 8);
	*tmp = 0;
	if (wm_class && wm_instance) {
		sprintf(tmp, "%s.%s", wm_instance, wm_class);
	} else {
		if (wm_instance)
			strcat(tmp, wm_instance);
		if (wm_class)
			strcat(tmp, wm_class);
	}

	key = WMCreatePLString(tmp);
	wfree(tmp);

	adict = WMGetFromPLDictionary(dict, key);
	iconk = WMCreatePLString("Icon");

	if (adict) {
		val = WMGetFromPLDictionary(adict, iconk);
	} else {
		/* no dictionary for app, so create one */
		adict = WMCreatePLDictionary(NULL, NULL);
		WMPutInPLDictionary(dict, key, adict);
		WMReleasePropList(adict);
		val = NULL;
	}

	if (!val) {
		val = WMCreatePLString(iconPath);
		WMPutInPLDictionary(adict, iconk, val);
		WMReleasePropList(val);
	} else {
		val = NULL;
	}

	WMReleasePropList(key);
	WMReleasePropList(iconk);

	if (val && !wPreferences.flags.noupdates)
		UpdateDomainFile(WDWindowAttributes);
}

/* This function is used if the application is a .app. It checks if it has an icon in it
 * like for example /usr/local/GNUstep/Applications/WPrefs.app/WPrefs.tiff
 */
void wApplicationExtractDirPackIcon(WScreen * scr, char *path, char *wm_instance, char *wm_class)
{
	char *iconPath = NULL;
	char *tmp = NULL;

	if (strstr(path, ".app")) {
		tmp = wmalloc(strlen(path) + 16);

		if (scr->flags.supports_tiff) {
			strcpy(tmp, path);
			strcat(tmp, ".tiff");
			if (access(tmp, R_OK) == 0)
				iconPath = tmp;
		}

		if (!iconPath) {
			strcpy(tmp, path);
			strcat(tmp, ".xpm");
			if (access(tmp, R_OK) == 0)
				iconPath = tmp;
		}

		if (!iconPath)
			wfree(tmp);

		if (iconPath) {
			wApplicationSaveIconPathFor(iconPath, wm_instance, wm_class);
			wfree(iconPath);
		}
	}
}

static void app_icon_create_from_docks(WWindow *wwin, WApplication *wapp, Window main_window)
{
	WScreen *scr = wwin->screen_ptr;

	if (scr->last_dock)
		wapp->app_icon = findDockIconFor(scr->last_dock, main_window);

	/* check main dock if we did not find it in last dock */
	if (!wapp->app_icon && scr->dock)
		wapp->app_icon = findDockIconFor(scr->dock, main_window);

	/* finally check clips */
	if (!wapp->app_icon) {
		int i;
		for (i = 0; i < scr->workspace_count; i++) {
			WDock *dock = scr->workspaces[i]->clip;
			if (dock)
				wapp->app_icon = findDockIconFor(dock, main_window);
			if (wapp->app_icon)
				break;
		}
	}

	/* If created, then set some flags */
	if (wapp->app_icon) {
		WWindow *mainw = wapp->main_window_desc;

		wapp->app_icon->running = 1;
		wapp->app_icon->icon->force_paint = 1;
		wapp->app_icon->icon->owner = mainw;
		if (mainw->wm_hints && (mainw->wm_hints->flags & IconWindowHint))
			wapp->app_icon->icon->icon_win = mainw->wm_hints->icon_window;

		wAppIconPaint(wapp->app_icon);
		wAppIconSave(wapp->app_icon);
	} else {
		wapp->app_icon = wAppIconCreate(wapp->main_window_desc);
	}
}

WApplication *wApplicationCreate(WWindow * wwin)
{
	WScreen *scr = wwin->screen_ptr;
	Window main_window = wwin->main_window;
	WApplication *wapp;
	WWindow *leader;

	if (main_window == None || main_window == scr->root_win)
		return NULL;

	{
		Window root;
		int foo;
		unsigned int bar;
		/* check if the window is valid */
		if (!XGetGeometry(dpy, main_window, &root, &foo, &foo, &bar, &bar, &bar, &bar))
			return NULL;
	}

	wapp = wApplicationOf(main_window);
	if (wapp) {
		wapp->refcount++;
		if (wapp->app_icon && wapp->app_icon->docked &&
		    wapp->app_icon->relaunching && wapp->main_window_desc->fake_group)
			wDockFinishLaunch(wapp->app_icon->dock, wapp->app_icon);

		return wapp;
	}

	wapp = wmalloc(sizeof(WApplication));

	wapp->refcount = 1;
	wapp->last_focused = NULL;
	wapp->urgent_bounce_timer = NULL;

	wapp->last_workspace = 0;

	wapp->main_window = main_window;
	wapp->main_window_desc = makeMainWindow(scr, main_window);
	if (!wapp->main_window_desc) {
		wfree(wapp);
		return NULL;
	}

	wapp->main_window_desc->fake_group = wwin->fake_group;
	wapp->main_window_desc->net_icon_image = RRetainImage(wwin->net_icon_image);

	extractIcon(wapp->main_window_desc);

	leader = wWindowFor(main_window);
	if (leader)
		leader->main_window = main_window;

	wapp->menu = wAppMenuGet(scr, main_window);
#ifdef USER_MENU
	if (!wapp->menu)
		wapp->menu = wUserMenuGet(scr, wapp->main_window_desc);
#endif	/* USER_MENU */

	/*
	 * Set application wide attributes from the leader.
	 */
	wapp->flags.hidden = WFLAGP(wapp->main_window_desc, start_hidden);
	wapp->flags.emulated = WFLAGP(wapp->main_window_desc, emulate_appicon);

	/* application descriptor */
	XSaveContext(dpy, main_window, wAppWinContext, (XPointer) wapp);

	/* Create the application icon */
	wapp->app_icon = NULL;
	if (!WFLAGP(wapp->main_window_desc, no_appicon)) {
		/* Create the application icon using the icon from docks
		 * If not found in docks, create a new icon 
		 * using the function wAppIconCreate() */
		app_icon_create_from_docks(wwin, wapp, main_window);

		/* Now, paint the icon */
		paint_app_icon(wapp);

		/* Save the app_icon in a file */
		save_app_icon(wwin, wapp);
	}

	return wapp;
}

void wApplicationDestroy(WApplication * wapp)
{
	WWindow *wwin;
	WScreen *scr;

	if (!wapp)
		return;

	wapp->refcount--;
	if (wapp->refcount > 0)
		return;

	if (wapp->urgent_bounce_timer) {
		WMDeleteTimerHandler(wapp->urgent_bounce_timer);
		wapp->urgent_bounce_timer = NULL;
	}
	if (wapp->flags.bouncing) {
		/* event.c:handleDestroyNotify forced this destroy
		   and thereby overlooked the bounce callback */
		wapp->refcount = 1;
		return;
	}

	scr = wapp->main_window_desc->screen_ptr;

	if (wapp == scr->wapp_list) {
		if (wapp->next)
			wapp->next->prev = NULL;
		scr->wapp_list = wapp->next;
	} else {
		if (wapp->next)
			wapp->next->prev = wapp->prev;
		if (wapp->prev)
			wapp->prev->next = wapp->next;
	}

	XDeleteContext(dpy, wapp->main_window, wAppWinContext);
	wAppMenuDestroy(wapp->menu);
	wApplicationDeactivate(wapp);

	if (wapp->app_icon) {
		if (wapp->app_icon->docked && !wapp->app_icon->attracted) {
			wapp->app_icon->running = 0;
			/* since we keep it, we don't care if it was attracted or not */
			wapp->app_icon->attracted = 0;
			wapp->app_icon->icon->shadowed = 0;
			wapp->app_icon->main_window = None;
			wapp->app_icon->pid = 0;
			wapp->app_icon->icon->owner = NULL;
			wapp->app_icon->icon->icon_win = None;
			wapp->app_icon->icon->force_paint = 1;
			wAppIconPaint(wapp->app_icon);
		} else if (wapp->app_icon->docked) {
			wapp->app_icon->running = 0;
			wDockDetach(wapp->app_icon->dock, wapp->app_icon);
		} else {
			wAppIconDestroy(wapp->app_icon);
		}
	}
	wwin = wWindowFor(wapp->main_window_desc->client_win);

	wWindowDestroy(wapp->main_window_desc);
	if (wwin) {
		/* undelete client window context that was deleted in
		 * wWindowDestroy */
		XSaveContext(dpy, wwin->client_win, wWinContext, (XPointer) & wwin->client_descriptor);
	}
	wfree(wapp);

	if (wPreferences.auto_arrange_icons)
		wArrangeIcons(scr, True);
}

void wApplicationActivate(WApplication *wapp)
{
#ifdef NEWAPPICON
	if (wapp->app_icon) {
		wIconSetHighlited(wapp->app_icon->icon, True);
		wAppIconPaint(wapp->app_icon);
	}
#endif
}

void wApplicationDeactivate(WApplication *wapp)
{
#ifdef NEWAPPICON
	if (wapp->app_icon) {
		wIconSetHighlited(wapp->app_icon->icon, False);
		wAppIconPaint(wapp->app_icon);
	}
#endif
}

