/* appicon.c- icon for applications (not mini-window)
 *
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
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "WindowMaker.h"
#include "window.h"
#include "icon.h"
#include "application.h"
#include "appicon.h"
#include "actions.h"
#include "stacking.h"
#include "dock.h"
#include "funcs.h"
#include "defaults.h"
#include "workspace.h"
#include "superfluous.h"
#include "menu.h"
#include "framewin.h"
#include "dialog.h"
#include "xinerama.h"
#include "client.h"
#ifdef XDND
#include "xdnd.h"
#endif

/*
 * icon_file for the dock is got from the preferences file by
 * using the classname/instancename
 */

/**** Global variables ****/
extern Cursor wCursor[WCUR_LAST];
extern WPreferences wPreferences;
extern WDDomain *WDWindowAttributes;

#define MOD_MASK       wPreferences.modifier_mask

void appIconMouseDown(WObjDescriptor * desc, XEvent * event);
static void iconDblClick(WObjDescriptor * desc, XEvent * event);
static void iconExpose(WObjDescriptor * desc, XEvent * event);
static void wApplicationSaveIconPathFor(char *iconPath, char *wm_instance, char *wm_class);

/* This function is used if the application is a .app. It checks if it has an icon in it
 * like for example /usr/local/GNUstep/Applications/WPrefs.app/WPrefs.tiff
 */
static void wApplicationExtractDirPackIcon(WScreen * scr, char *path, char *wm_instance, char *wm_class)
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

WAppIcon *wAppIconCreateForDock(WScreen * scr, char *command, char *wm_instance, char *wm_class, int tile)
{
	WAppIcon *dicon;
	char *path;

	dicon = wmalloc(sizeof(WAppIcon));
	wretain(dicon);
	dicon->yindex = -1;
	dicon->xindex = -1;

	dicon->prev = NULL;
	dicon->next = scr->app_icon_list;
	if (scr->app_icon_list)
		scr->app_icon_list->prev = dicon;

	scr->app_icon_list = dicon;

	if (command)
		dicon->command = wstrdup(command);

	if (wm_class)
		dicon->wm_class = wstrdup(wm_class);

	if (wm_instance)
		dicon->wm_instance = wstrdup(wm_instance);

	path = wDefaultGetIconFile(scr, wm_instance, wm_class, True);
	if (!path && command) {
		wApplicationExtractDirPackIcon(scr, command, wm_instance, wm_class);
		path = wDefaultGetIconFile(scr, wm_instance, wm_class, False);
	}

	if (path)
		path = FindImage(wPreferences.icon_path, path);

	dicon->icon = wIconCreateWithIconFile(scr, path, tile);
	if (path)
		wfree(path);
#ifdef XDND
	wXDNDMakeAwareness(dicon->icon->core->window);
#endif

	/* will be overriden by dock */
	dicon->icon->core->descriptor.handle_mousedown = appIconMouseDown;
	dicon->icon->core->descriptor.handle_expose = iconExpose;
	dicon->icon->core->descriptor.parent_type = WCLASS_APPICON;
	dicon->icon->core->descriptor.parent = dicon;
	AddToStackList(dicon->icon->core);

	return dicon;
}

void makeAppIconFor(WApplication * wapp)
{
	/* If app_icon, work is done, return */
	if (wapp->app_icon)
		return;

	if (!WFLAGP(wapp->main_window_desc, no_appicon))
		wapp->app_icon = wAppIconCreate(wapp->main_window_desc);
	else
		wapp->app_icon = NULL;

	/* Now, paint the icon */
	paint_app_icon(wapp);
}

void paint_app_icon(WApplication *wapp)
{
	WIcon *icon;
	WScreen *scr = wapp->main_window_desc->screen_ptr;
	WDock *clip = scr->workspaces[scr->current_workspace]->clip;
	int x = 0, y = 0;

	if (!wapp || !wapp->app_icon)
		return;

	icon = wapp->app_icon->icon;
	wapp->app_icon->main_window = wapp->main_window;

	/* If the icon is docked, don't continue */
	if (wapp->app_icon->docked)
		return;

	if (clip && clip->attract_icons && wDockFindFreeSlot(clip, &x, &y)) {
		wapp->app_icon->attracted = 1;
		if (!icon->shadowed) {
			icon->shadowed = 1;
			icon->force_paint = 1;
		}
		wDockAttachIcon(clip, wapp->app_icon, x, y);
	} else {
		PlaceIcon(scr, &x, &y, wGetHeadForWindow(wapp->main_window_desc));
		wAppIconMove(wapp->app_icon, x, y);
		wLowerFrame(icon->core);
	}

	if (!clip || !wapp->app_icon->attracted || !clip->collapsed)
		XMapWindow(dpy, icon->core->window);

	if (wPreferences.auto_arrange_icons && !wapp->app_icon->attracted)
		wArrangeIcons(scr, True);
}

void removeAppIconFor(WApplication * wapp)
{
	if (!wapp->app_icon)
		return;

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

	wapp->app_icon = NULL;

	if (wPreferences.auto_arrange_icons)
		wArrangeIcons(wapp->main_window_desc->screen_ptr, True);
}

WAppIcon *wAppIconCreate(WWindow * leader_win)
{
	WAppIcon *aicon;
	WScreen *scr = leader_win->screen_ptr;

	aicon = wmalloc(sizeof(WAppIcon));
	wretain(aicon);

	aicon->yindex = -1;
	aicon->xindex = -1;

	aicon->prev = NULL;
	aicon->next = scr->app_icon_list;
	if (scr->app_icon_list)
		scr->app_icon_list->prev = aicon;

	scr->app_icon_list = aicon;

	if (leader_win->wm_class)
		aicon->wm_class = wstrdup(leader_win->wm_class);

	if (leader_win->wm_instance)
		aicon->wm_instance = wstrdup(leader_win->wm_instance);

	aicon->icon = wIconCreate(leader_win);
#ifdef XDND
	wXDNDMakeAwareness(aicon->icon->core->window);
#endif

	/* will be overriden if docked */
	aicon->icon->core->descriptor.handle_mousedown = appIconMouseDown;
	aicon->icon->core->descriptor.handle_expose = iconExpose;
	aicon->icon->core->descriptor.parent_type = WCLASS_APPICON;
	aicon->icon->core->descriptor.parent = aicon;
	AddToStackList(aicon->icon->core);
	aicon->icon->show_title = 0;
	wIconUpdate(aicon->icon);

	return aicon;
}

void wAppIconDestroy(WAppIcon * aicon)
{
	WScreen *scr = aicon->icon->core->screen_ptr;

	RemoveFromStackList(aicon->icon->core);
	wIconDestroy(aicon->icon);
	if (aicon->command)
		wfree(aicon->command);
#ifdef XDND
	if (aicon->dnd_command)
		wfree(aicon->dnd_command);
#endif
	if (aicon->wm_instance)
		wfree(aicon->wm_instance);

	if (aicon->wm_class)
		wfree(aicon->wm_class);

	if (aicon == scr->app_icon_list) {
		if (aicon->next)
			aicon->next->prev = NULL;
		scr->app_icon_list = aicon->next;
	} else {
		if (aicon->next)
			aicon->next->prev = aicon->prev;
		if (aicon->prev)
			aicon->prev->next = aicon->next;
	}

	aicon->destroyed = 1;
	wrelease(aicon);
}

static void drawCorner(WIcon * icon)
{
	WScreen *scr = icon->core->screen_ptr;
	XPoint points[3];

	points[0].x = 1;
	points[0].y = 1;
	points[1].x = 12;
	points[1].y = 1;
	points[2].x = 1;
	points[2].y = 12;
	XFillPolygon(dpy, icon->core->window, scr->icon_title_texture->normal_gc,
		     points, 3, Convex, CoordModeOrigin);
	XDrawLine(dpy, icon->core->window, scr->icon_title_texture->light_gc, 0, 0, 0, 12);
	XDrawLine(dpy, icon->core->window, scr->icon_title_texture->light_gc, 0, 0, 12, 0);
}

void wAppIconMove(WAppIcon * aicon, int x, int y)
{
	XMoveWindow(dpy, aicon->icon->core->window, x, y);
	aicon->x_pos = x;
	aicon->y_pos = y;
}

#ifdef WS_INDICATOR
static void updateDockNumbers(WScreen * scr)
{
	int length;
	char *ws_numbers;
	WAppIcon *dicon = scr->dock->icon_array[0];

	ws_numbers = wmalloc(20);
	snprintf(ws_numbers, 20, "%i [ %i ]", scr->current_workspace + 1, ((scr->current_workspace / 10) + 1));
	length = strlen(ws_numbers);

	XClearArea(dpy, dicon->icon->core->window, 2, 2, 50, WMFontHeight(scr->icon_title_font) + 1, False);

	WMDrawString(scr->wmscreen, dicon->icon->core->window, scr->black,
		     scr->icon_title_font, 4, 3, ws_numbers, length);

	WMDrawString(scr->wmscreen, dicon->icon->core->window, scr->white,
		     scr->icon_title_font, 3, 2, ws_numbers, length);

	wfree(ws_numbers);
}
#endif				/* WS_INDICATOR */

void wAppIconPaint(WAppIcon * aicon)
{
	WApplication *wapp;
	WScreen *scr = aicon->icon->core->screen_ptr;

	if (aicon->icon->owner)
		wapp = wApplicationOf(aicon->icon->owner->main_window);
	else
		wapp = NULL;

	wIconPaint(aicon->icon);

# ifdef WS_INDICATOR
	if (aicon->docked && scr->dock && scr->dock == aicon->dock && aicon->yindex == 0)
		updateDockNumbers(scr);
# endif
	if (scr->dock_dots && aicon->docked && !aicon->running && aicon->command != NULL) {
		XSetClipMask(dpy, scr->copy_gc, scr->dock_dots->mask);
		XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
		XCopyArea(dpy, scr->dock_dots->image, aicon->icon->core->window,
			  scr->copy_gc, 0, 0, scr->dock_dots->width, scr->dock_dots->height, 0, 0);
	}
#ifdef HIDDENDOT
	if (wapp && wapp->flags.hidden) {
		XSetClipMask(dpy, scr->copy_gc, scr->dock_dots->mask);
		XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
		XCopyArea(dpy, scr->dock_dots->image,
			  aicon->icon->core->window, scr->copy_gc, 0, 0, 7, scr->dock_dots->height, 0, 0);
	}
#endif				/* HIDDENDOT */

	if (aicon->omnipresent)
		drawCorner(aicon->icon);

	XSetClipMask(dpy, scr->copy_gc, None);
	if (aicon->launching)
		XFillRectangle(dpy, aicon->icon->core->window, scr->stipple_gc,
			       0, 0, wPreferences.icon_size, wPreferences.icon_size);
}

/* Internal application to save the application icon */
static void save_app_icon_core(WAppIcon *aicon)
{
	char *path;

	path = wIconStore(aicon->icon);
	if (!path)
		return;

	wApplicationSaveIconPathFor(path, aicon->wm_instance, aicon->wm_class);

	wfree(path);
}

/* Save the application icon */
/* This function is used when the icon doesn't have window, like dock or clip */
void wAppIconSave(WAppIcon *aicon)
{
	if (!aicon->docked || aicon->attracted)
		return;

	save_app_icon_core(aicon);
}

#define canBeDocked(wwin)  ((wwin) && ((wwin)->wm_class||(wwin)->wm_instance))

/* main_window may not have the full command line; try to find one which does */
static void relaunchApplication(WApplication *wapp)
{
	WScreen *scr;
	WWindow *wlist, *next;

	scr = wapp->main_window_desc->screen_ptr;
	wlist = scr->focused_window;
	if (! wlist)
		return;

	while (wlist->prev)
		wlist = wlist->prev;

	while (wlist) {
		next = wlist->next;

		if (wlist->main_window == wapp->main_window) {
			if (RelaunchWindow(wlist))
				return;
		}

		wlist = next;
	}
}

static void relaunchCallback(WMenu * menu, WMenuEntry * entry)
{
	WApplication *wapp = (WApplication *) entry->clientdata;

	relaunchApplication(wapp);
}

static void hideCallback(WMenu * menu, WMenuEntry * entry)
{
	WApplication *wapp = (WApplication *) entry->clientdata;

	if (wapp->flags.hidden) {
		wWorkspaceChange(menu->menu->screen_ptr, wapp->last_workspace);
		wUnhideApplication(wapp, False, False);
	} else {
		wHideApplication(wapp);
	}
}

static void unhideHereCallback(WMenu * menu, WMenuEntry * entry)
{
	WApplication *wapp = (WApplication *) entry->clientdata;

	wUnhideApplication(wapp, False, True);
}

static void setIconCallback(WMenu * menu, WMenuEntry * entry)
{
	WAppIcon *icon = ((WApplication *) entry->clientdata)->app_icon;
	char *file = NULL;
	WScreen *scr;
	int result;

	assert(icon != NULL);

	if (icon->editing)
		return;

	icon->editing = 1;
	scr = icon->icon->core->screen_ptr;

	wretain(icon);

	result = wIconChooserDialog(scr, &file, icon->wm_instance, icon->wm_class);

	if (result && !icon->destroyed) {
		if (file && *file == 0) {
			wfree(file);
			file = NULL;
		}
		if (!wIconChangeImageFile(icon->icon, file)) {
			wMessageDialog(scr, _("Error"),
				       _("Could not open specified icon file"), _("OK"), NULL, NULL);
		} else {
			wDefaultChangeIcon(scr, icon->wm_instance, icon->wm_class, file);
			wAppIconPaint(icon);
		}
		if (file)
			wfree(file);
	}
	icon->editing = 0;
	wrelease(icon);
}

static void killCallback(WMenu * menu, WMenuEntry * entry)
{
	WApplication *wapp = (WApplication *) entry->clientdata;
	WFakeGroupLeader *fPtr;
	char *buffer;
	char *shortname;
	char *basename(const char *shortname);

	if (!WCHECK_STATE(WSTATE_NORMAL))
		return;

	WCHANGE_STATE(WSTATE_MODAL);

	assert(entry->clientdata != NULL);

	shortname = basename(wapp->app_icon->wm_instance);

	buffer = wstrconcat(wapp->app_icon ? shortname : NULL,
			    _(" will be forcibly closed.\n"
			      "Any unsaved changes will be lost.\n" "Please confirm."));

	fPtr = wapp->main_window_desc->fake_group;

	wretain(wapp->main_window_desc);
	if (wPreferences.dont_confirm_kill
	    || wMessageDialog(menu->frame->screen_ptr, _("Kill Application"),
			      buffer, _("Yes"), _("No"), NULL) == WAPRDefault) {
		if (fPtr != NULL) {
			WWindow *wwin, *twin;

			wwin = wapp->main_window_desc->screen_ptr->focused_window;
			while (wwin) {
				twin = wwin->prev;
				if (wwin->fake_group == fPtr)
					wClientKill(wwin);
				wwin = twin;
			}
		} else if (!wapp->main_window_desc->flags.destroyed) {
			wClientKill(wapp->main_window_desc);
		}
	}
	wrelease(wapp->main_window_desc);
	wfree(buffer);
	WCHANGE_STATE(WSTATE_NORMAL);
}

static WMenu *createApplicationMenu(WScreen * scr)
{
	WMenu *menu;

	menu = wMenuCreate(scr, NULL, False);
	wMenuAddCallback(menu, _("Launch"), relaunchCallback, NULL);
	wMenuAddCallback(menu, _("Unhide Here"), unhideHereCallback, NULL);
	wMenuAddCallback(menu, _("Hide"), hideCallback, NULL);
	wMenuAddCallback(menu, _("Set Icon..."), setIconCallback, NULL);
	wMenuAddCallback(menu, _("Kill"), killCallback, NULL);

	return menu;
}

static void openApplicationMenu(WApplication * wapp, int x, int y)
{
	WMenu *menu;
	WScreen *scr = wapp->main_window_desc->screen_ptr;
	int i;

	if (!scr->icon_menu) {
		scr->icon_menu = createApplicationMenu(scr);
		wfree(scr->icon_menu->entries[1]->text);
	}

	menu = scr->icon_menu;

	if (wapp->flags.hidden)
		menu->entries[1]->text = _("Unhide");
	else
		menu->entries[1]->text = _("Hide");

	menu->flags.realized = 0;
	wMenuRealize(menu);

	x -= menu->frame->core->width / 2;
	if (x + menu->frame->core->width > scr->scr_width)
		x = scr->scr_width - menu->frame->core->width;

	if (x < 0)
		x = 0;

	/* set client data */
	for (i = 0; i < menu->entry_no; i++)
		menu->entries[i]->clientdata = wapp;

	wMenuMapAt(menu, x, y, False);
}

/******************************************************************/

static void iconExpose(WObjDescriptor * desc, XEvent * event)
{
	wAppIconPaint(desc->parent);
}

static void iconDblClick(WObjDescriptor * desc, XEvent * event)
{
	WAppIcon *aicon = desc->parent;
	WApplication *wapp;
	WScreen *scr = aicon->icon->core->screen_ptr;
	int unhideHere;

	assert(aicon->icon->owner != NULL);

	wapp = wApplicationOf(aicon->icon->owner->main_window);

	if (event->xbutton.state & ControlMask) {
		relaunchApplication(wapp);
		return;
	}

	unhideHere = (event->xbutton.state & ShiftMask);
	/* go to the last workspace that the user worked on the app */
	if (!unhideHere && wapp->last_workspace != scr->current_workspace)
		wWorkspaceChange(scr, wapp->last_workspace);

	wUnhideApplication(wapp, event->xbutton.button == Button2, unhideHere);

	if (event->xbutton.state & MOD_MASK)
		wHideOtherApplications(aicon->icon->owner);
}

void appIconMouseDown(WObjDescriptor * desc, XEvent * event)
{
	WAppIcon *aicon = desc->parent;
	WIcon *icon = aicon->icon;
	XEvent ev;
	int x = aicon->x_pos, y = aicon->y_pos;
	int dx = event->xbutton.x, dy = event->xbutton.y;
	int grabbed = 0;
	int done = 0;
	int superfluous = wPreferences.superfluous;	/* we catch it to avoid problems */
	WScreen *scr = icon->core->screen_ptr;
	WWorkspace *workspace = scr->workspaces[scr->current_workspace];
	int shad_x = 0, shad_y = 0, docking = 0, dockable, collapsed = 0;
	int ix, iy;
	int clickButton = event->xbutton.button;
	Pixmap ghost = None;
	Window wins[2];
	Bool movingSingle = False;
	int oldX = x;
	int oldY = y;
	Bool hasMoved = False;

	if (aicon->editing || WCHECK_STATE(WSTATE_MODAL))
		return;

	if (IsDoubleClick(scr, event)) {
		/* Middle or right mouse actions were handled on first click */
		if (event->xbutton.button == Button1)
			iconDblClick(desc, event);
		return;
	}

	if (event->xbutton.button == Button2) {
		WApplication *wapp = wApplicationOf(aicon->icon->owner->main_window);

		if (wapp)
			relaunchApplication(wapp);

		return;
	}

	if (event->xbutton.button == Button3) {
		WObjDescriptor *desc;
		WApplication *wapp = wApplicationOf(aicon->icon->owner->main_window);

		if (!wapp)
			return;

		if (event->xbutton.send_event &&
		    XGrabPointer(dpy, aicon->icon->core->window, True, ButtonMotionMask
				 | ButtonReleaseMask | ButtonPressMask, GrabModeAsync,
				 GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
			wwarning("pointer grab failed for appicon menu");
			return;
		}

		openApplicationMenu(wapp, event->xbutton.x_root, event->xbutton.y_root);

		/* allow drag select of menu */
		desc = &scr->icon_menu->menu->descriptor;
		event->xbutton.send_event = True;
		(*desc->handle_mousedown) (desc, event);
		return;
	}

	if (event->xbutton.state & MOD_MASK)
		wLowerFrame(icon->core);
	else
		wRaiseFrame(icon->core);

	if (XGrabPointer(dpy, icon->core->window, True, ButtonMotionMask
			 | ButtonReleaseMask | ButtonPressMask, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime) != GrabSuccess)
		wwarning("pointer grab failed for appicon move");

	if (wPreferences.flags.nodock && wPreferences.flags.noclip)
		dockable = 0;
	else
		dockable = canBeDocked(icon->owner);

	wins[0] = icon->core->window;
	wins[1] = scr->dock_shadow;
	XRestackWindows(dpy, wins, 2);
	if (superfluous) {
		if (icon->pixmap != None)
			ghost = MakeGhostIcon(scr, icon->pixmap);
		else
			ghost = MakeGhostIcon(scr, icon->core->window);
		XSetWindowBackgroundPixmap(dpy, scr->dock_shadow, ghost);
		XClearWindow(dpy, scr->dock_shadow);
	}

	while (!done) {
		WMMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask | ButtonPressMask
			    | ButtonMotionMask | ExposureMask | EnterWindowMask, &ev);
		switch (ev.type) {
		case Expose:
			WMHandleEvent(&ev);
			break;

                case EnterNotify:
                        /* It means the cursor moved so fast that it entered
                         * something else (if moving slowly, it would have
                         * stayed in the appIcon that is being moved. Ignore
                         * such "spurious" EnterNotifiy's */
                        break;

		case MotionNotify:
			hasMoved = True;
			if (!grabbed) {
				if (abs(dx - ev.xmotion.x) >= MOVE_THRESHOLD
				    || abs(dy - ev.xmotion.y) >= MOVE_THRESHOLD) {
					XChangeActivePointerGrab(dpy, ButtonMotionMask
								 | ButtonReleaseMask | ButtonPressMask,
								 wCursor[WCUR_MOVE], CurrentTime);
					grabbed = 1;
				} else {
					break;
				}
			}
			x = ev.xmotion.x_root - dx;
			y = ev.xmotion.y_root - dy;

			if (movingSingle)
				XMoveWindow(dpy, icon->core->window, x, y);
			else
				wAppIconMove(aicon, x, y);

			if (dockable) {
				if (scr->dock && wDockSnapIcon(scr->dock, aicon, x, y, &ix, &iy, False)) {
					shad_x = scr->dock->x_pos + ix * wPreferences.icon_size;
					shad_y = scr->dock->y_pos + iy * wPreferences.icon_size;

					if (scr->last_dock != scr->dock && collapsed) {
						scr->last_dock->collapsed = 1;
						wDockHideIcons(scr->last_dock);
						collapsed = 0;
					}
					if (!collapsed && (collapsed = scr->dock->collapsed)) {
						scr->dock->collapsed = 0;
						wDockShowIcons(scr->dock);
					}

					if (scr->dock->auto_raise_lower)
						wDockRaise(scr->dock);

					scr->last_dock = scr->dock;

					XMoveWindow(dpy, scr->dock_shadow, shad_x, shad_y);
					if (!docking)
						XMapWindow(dpy, scr->dock_shadow);

					docking = 1;
				} else if (workspace->clip &&
					   wDockSnapIcon(workspace->clip, aicon, x, y, &ix, &iy, False)) {
					shad_x = workspace->clip->x_pos + ix * wPreferences.icon_size;
					shad_y = workspace->clip->y_pos + iy * wPreferences.icon_size;

					if (scr->last_dock != workspace->clip && collapsed) {
						scr->last_dock->collapsed = 1;
						wDockHideIcons(scr->last_dock);
						collapsed = 0;
					}
					if (!collapsed && (collapsed = workspace->clip->collapsed)) {
						workspace->clip->collapsed = 0;
						wDockShowIcons(workspace->clip);
					}

					if (workspace->clip->auto_raise_lower)
						wDockRaise(workspace->clip);

					scr->last_dock = workspace->clip;

					XMoveWindow(dpy, scr->dock_shadow, shad_x, shad_y);
					if (!docking)
						XMapWindow(dpy, scr->dock_shadow);

					docking = 1;
				} else if (docking) {
					XUnmapWindow(dpy, scr->dock_shadow);
					docking = 0;
				}
			}
			break;

		case ButtonPress:
			break;

		case ButtonRelease:
			if (ev.xbutton.button != clickButton)
				break;
			XUngrabPointer(dpy, CurrentTime);

			if (docking) {
				Bool docked;

				/* icon is trying to be docked */
				SlideWindow(icon->core->window, x, y, shad_x, shad_y);
				XUnmapWindow(dpy, scr->dock_shadow);
				docked = wDockAttachIcon(scr->last_dock, aicon, ix, iy);
				if (scr->last_dock->auto_collapse)
					collapsed = 0;

				if (workspace->clip &&
				    workspace->clip != scr->last_dock && workspace->clip->auto_raise_lower)
					wDockLower(workspace->clip);

				if (!docked) {
					/* If icon could not be docked, slide it back to the old
					 * position */
					SlideWindow(icon->core->window, x, y, oldX, oldY);
				}
			} else {
				if (movingSingle) {
					/* move back to its place */
					SlideWindow(icon->core->window, x, y, oldX, oldY);
					wAppIconMove(aicon, oldX, oldY);
				} else {
					XMoveWindow(dpy, icon->core->window, x, y);
					aicon->x_pos = x;
					aicon->y_pos = y;
				}
				if (workspace->clip && workspace->clip->auto_raise_lower)
					wDockLower(workspace->clip);
			}
			if (collapsed) {
				scr->last_dock->collapsed = 1;
				wDockHideIcons(scr->last_dock);
				collapsed = 0;
			}
			if (superfluous) {
				if (ghost != None)
					XFreePixmap(dpy, ghost);
				XSetWindowBackground(dpy, scr->dock_shadow, scr->white_pixel);
			}

			if (wPreferences.auto_arrange_icons)
				wArrangeIcons(scr, True);

			if (wPreferences.single_click && !hasMoved)
				iconDblClick(desc, event);

			done = 1;
			break;
		}
	}
}

/* This function save the application icon and store the path in the Dictionary */
static void wApplicationSaveIconPathFor(char *iconPath, char *wm_instance, char *wm_class)
{
	WMPropList *dict = WDWindowAttributes->dictionary;
	WMPropList *adict, *key, *iconk;
	WMPropList *val;
	char *tmp;

	tmp = get_name_for_instance_class(wm_instance, wm_class);
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

/* Save the application icon */
/* This function is used by normal windows */
void save_app_icon(WApplication *wapp)
{
	if (!wapp->app_icon)
		return;

	save_app_icon_core(wapp->app_icon);
}

static WAppIcon *findDockIconFor(WDock *dock, Window main_window)
{
	WAppIcon *aicon = NULL;

	aicon = wDockFindIconForWindow(dock, main_window);
	if (!aicon) {
		wDockTrackWindowLaunch(dock, main_window);
		aicon = wDockFindIconForWindow(dock, main_window);
	}
	return aicon;
}

void app_icon_create_from_docks(WWindow *wwin, WApplication *wapp, Window main_window)
{
	WScreen *scr = wwin->screen_ptr;

	/* Create the application icon */
	wapp->app_icon = NULL;

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
