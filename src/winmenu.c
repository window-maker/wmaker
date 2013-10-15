/* winmenu.c - command menu for windows
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "WindowMaker.h"
#include "actions.h"
#include "menu.h"
#include "main.h"
#include "window.h"
#include "client.h"
#include "application.h"
#include "keybind.h"
#include "misc.h"
#include "framewin.h"
#include "workspace.h"
#include "winspector.h"
#include "dialog.h"
#include "stacking.h"
#include "icon.h"
#include "xinerama.h"
#include "winmenu.h"

enum
{
	MC_MAXIMIZE,
	MC_OTHERMAX,
	MC_MINIATURIZE,
	MC_SHADE,
	MC_HIDE,
	MC_MOVERESIZE,
	MC_SELECT,
	MC_DUMMY_MOVETO,
	MC_PROPERTIES,
	MC_OPTIONS,
	MC_RELAUNCH,
	MC_CLOSE,
	MC_KILL
};

enum
{
	WO_KEEP_ON_TOP,
	WO_KEEP_AT_BOTTOM,
	WO_OMNIPRESENT,
	WO_ENTRIES
};

enum
{
	MAXC_V,
	MAXC_H,
	MAXC_LH,
	MAXC_RH,
	MAXC_TH,
	MAXC_BH,
	MAXC_LTC,
	MAXC_RTC,
	MAXC_LBC,
	MAXC_RBC,
	MAXC_MAXIMUS
};

static void updateOptionsMenu(WMenu * menu, WWindow * wwin);

static void execWindowOptionCommand(WMenu * menu, WMenuEntry * entry)
{
	WWindow *wwin = (WWindow *) entry->clientdata;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) menu;

	switch (entry->order) {
	case WO_KEEP_ON_TOP:
		if (wwin->frame->core->stacking->window_level != WMFloatingLevel)
			ChangeStackingLevel(wwin->frame->core, WMFloatingLevel);
		else
			ChangeStackingLevel(wwin->frame->core, WMNormalLevel);
		break;

	case WO_KEEP_AT_BOTTOM:
		if (wwin->frame->core->stacking->window_level != WMSunkenLevel)
			ChangeStackingLevel(wwin->frame->core, WMSunkenLevel);
		else
			ChangeStackingLevel(wwin->frame->core, WMNormalLevel);
		break;

	case WO_OMNIPRESENT:
		wWindowSetOmnipresent(wwin, !wwin->flags.omnipresent);
		break;
	}
}

static void execMaximizeCommand(WMenu * menu, WMenuEntry * entry)
{
	WWindow *wwin = (WWindow *) entry->clientdata;
	
	(void) menu;


	switch (entry->order) {
	case MAXC_V:
		handleMaximize(wwin, MAX_VERTICAL);
		break;

	case MAXC_H:
		handleMaximize(wwin,MAX_HORIZONTAL);
		break;

	case MAXC_LH:
		handleMaximize(wwin,MAX_VERTICAL | MAX_LEFTHALF);
		break;

	case MAXC_RH:
		handleMaximize(wwin,MAX_VERTICAL | MAX_RIGHTHALF);
		break;

	case MAXC_TH:
		handleMaximize(wwin,MAX_HORIZONTAL | MAX_TOPHALF);
		break;

	case MAXC_BH:
		handleMaximize(wwin,MAX_HORIZONTAL | MAX_BOTTOMHALF);
		break;

	case MAXC_LTC:
		handleMaximize(wwin,MAX_LEFTHALF | MAX_TOPHALF);
		break;

	case MAXC_RTC:
		handleMaximize(wwin,MAX_RIGHTHALF | MAX_TOPHALF);
		break;

	case MAXC_LBC:
		handleMaximize(wwin,MAX_LEFTHALF | MAX_BOTTOMHALF);
		break;

	case MAXC_RBC:
		handleMaximize(wwin,MAX_RIGHTHALF | MAX_BOTTOMHALF);
		break;

	case MAXC_MAXIMUS:
		handleMaximize(wwin,MAX_MAXIMUS);
		break;

	}
}

static void execMenuCommand(WMenu * menu, WMenuEntry * entry)
{
	WWindow *wwin = (WWindow *) entry->clientdata;
	WApplication *wapp;

	CloseWindowMenu(menu->frame->screen_ptr);

	switch (entry->order) {
	case MC_CLOSE:
		/* send delete message */
		wClientSendProtocol(wwin, w_global.atom.wm.delete_window,
								  w_global.timestamp.last_event);
		break;

	case MC_KILL:
		wretain(wwin);
		if (wPreferences.dont_confirm_kill
		    || wMessageDialog(menu->frame->screen_ptr, _("Kill Application"),
				      _
				      ("This will kill the application.\nAny unsaved changes will be lost.\nPlease confirm."),
				      _("Yes"), _("No"), NULL) == WAPRDefault) {
			if (!wwin->flags.destroyed)
				wClientKill(wwin);
		}
		wrelease(wwin);
		break;

	case MC_MINIATURIZE:
		if (wwin->flags.miniaturized) {
			wDeiconifyWindow(wwin);
		} else {
			if (wwin->protocols.MINIATURIZE_WINDOW) {
				wClientSendProtocol(wwin, w_global.atom.gnustep.wm_miniaturize_window,
										  w_global.timestamp.last_event);
			} else {
				wIconifyWindow(wwin);
			}
		}
		break;

	case MC_MAXIMIZE:
		if (wwin->flags.maximized)
			wUnmaximizeWindow(wwin);
		else
			wMaximizeWindow(wwin, MAX_VERTICAL | MAX_HORIZONTAL);
		break;

	case MC_SHADE:
		if (wwin->flags.shaded)
			wUnshadeWindow(wwin);
		else
			wShadeWindow(wwin);
		break;

	case MC_SELECT:
		if (!wwin->flags.miniaturized)
			wSelectWindow(wwin, !wwin->flags.selected);
		else
			wIconSelect(wwin->icon);
		break;

	case MC_MOVERESIZE:
		wKeyboardMoveResizeWindow(wwin);
		break;

	case MC_PROPERTIES:
		wShowInspectorForWindow(wwin);
		break;

	case MC_RELAUNCH:
		(void) RelaunchWindow(wwin);
		break;

	case MC_HIDE:
		wapp = wApplicationOf(wwin->main_window);
		wHideApplication(wapp);
		break;
	}
}

static void switchWSCommand(WMenu * menu, WMenuEntry * entry)
{
	WWindow *wwin = (WWindow *) entry->clientdata;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) menu;

	wSelectWindow(wwin, False);
	wWindowChangeWorkspace(wwin, entry->order);
}

static void makeShortcutCommand(WMenu *menu, WMenuEntry *entry)
{
	WWindow *wwin = (WWindow *) entry->clientdata;
	WScreen *scr = wwin->screen_ptr;
	int index = entry->order - WO_ENTRIES;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) menu;

	if (w_global.shortcut.windows[index]) {
		WMFreeArray(w_global.shortcut.windows[index]);
		w_global.shortcut.windows[index] = NULL;
	}

	if (wwin->flags.selected && scr->selected_windows) {
		w_global.shortcut.windows[index] = WMDuplicateArray(scr->selected_windows);
	} else {
		w_global.shortcut.windows[index] = WMCreateArray(4);
		WMAddToArray(w_global.shortcut.windows[index], wwin);
	}

	wSelectWindow(wwin, !wwin->flags.selected);
	XFlush(dpy);
	wusleep(3000);
	wSelectWindow(wwin, !wwin->flags.selected);
	XFlush(dpy);
}

static void updateWorkspaceMenu(WMenu * menu)
{
	char title[MAX_WORKSPACENAME_WIDTH + 1];
	WMenuEntry *entry;
	int i;

	for (i = 0; i < w_global.workspace.count; i++) {
		if (i < menu->entry_no) {

			entry = menu->entries[i];
			if (strcmp(entry->text, w_global.workspace.array[i]->name) != 0) {
				wfree(entry->text);
				strncpy(title, w_global.workspace.array[i]->name, MAX_WORKSPACENAME_WIDTH);
				title[MAX_WORKSPACENAME_WIDTH] = 0;
				menu->entries[i]->text = wstrdup(title);
				menu->entries[i]->rtext = GetShortcutKey(wKeyBindings[WKBD_MOVE_WORKSPACE1 + i]);
				menu->flags.realized = 0;
			}
		} else {
			strncpy(title, w_global.workspace.array[i]->name, MAX_WORKSPACENAME_WIDTH);
			title[MAX_WORKSPACENAME_WIDTH] = 0;

			entry = wMenuAddCallback(menu, title, switchWSCommand, NULL);
			entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MOVE_WORKSPACE1 + i]);

			menu->flags.realized = 0;
		}

		/* workspace shortcut labels */
		if (i / 10 == w_global.workspace.current / 10)
			entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MOVE_WORKSPACE1 + (i % 10)]);
		else
			entry->rtext = NULL;
	}

	if (!menu->flags.realized)
		wMenuRealize(menu);
}

static void updateMakeShortcutMenu(WMenu *menu, WWindow *wwin)
{
	WMenu *smenu = menu->cascades[menu->entries[MC_OPTIONS]->cascade];
	int i;
	char *buffer;
	int buflen;
	KeyCode kcode;

	if (!smenu)
		return;

	buflen = strlen(_("Set Shortcut")) + 16;
	buffer = wmalloc(buflen);

	for (i = WO_ENTRIES; i < smenu->entry_no; i++) {
		char *tmp;
		int shortcutNo = i - WO_ENTRIES;
		WMenuEntry *entry = smenu->entries[i];
		WMArray *shortSelWindows = w_global.shortcut.windows[shortcutNo];

		snprintf(buffer, buflen, "%s %i", _("Set Shortcut"), shortcutNo + 1);

		if (!shortSelWindows) {
			entry->flags.indicator_on = 0;
		} else {
			entry->flags.indicator_on = 1;
			if (WMCountInArray(shortSelWindows, wwin))
				entry->flags.indicator_type = MI_DIAMOND;
			else
				entry->flags.indicator_type = MI_CHECK;
		}

		if (strcmp(buffer, entry->text) != 0) {
			wfree(entry->text);
			entry->text = wstrdup(buffer);
			smenu->flags.realized = 0;
		}

		kcode = wKeyBindings[WKBD_WINDOW1 + shortcutNo].keycode;

		if (kcode) {
			if ((tmp = GetShortcutKey(wKeyBindings[WKBD_WINDOW1 + shortcutNo]))
			    && (!entry->rtext || strcmp(tmp, entry->rtext) != 0)) {
				if (entry->rtext)
					wfree(entry->rtext);
				entry->rtext = tmp;
				smenu->flags.realized = 0;
			}
			wMenuSetEnabled(smenu, i, True);
		} else {
			wMenuSetEnabled(smenu, i, False);
			if (entry->rtext) {
				wfree(entry->rtext);
				entry->rtext = NULL;
				smenu->flags.realized = 0;
			}
		}
		entry->clientdata = wwin;
	}
	wfree(buffer);
	if (!smenu->flags.realized)
		wMenuRealize(smenu);
}

static void updateOptionsMenu(WMenu * menu, WWindow * wwin)
{
	WMenu *smenu = menu->cascades[menu->entries[MC_OPTIONS]->cascade];

	/* keep on top check */
	smenu->entries[WO_KEEP_ON_TOP]->clientdata = wwin;
	smenu->entries[WO_KEEP_ON_TOP]->flags.indicator_on =
	    (wwin->frame->core->stacking->window_level == WMFloatingLevel) ? 1 : 0;
	wMenuSetEnabled(smenu, WO_KEEP_ON_TOP, !wwin->flags.miniaturized);

	/* keep at bottom check */
	smenu->entries[WO_KEEP_AT_BOTTOM]->clientdata = wwin;
	smenu->entries[WO_KEEP_AT_BOTTOM]->flags.indicator_on =
	    (wwin->frame->core->stacking->window_level == WMSunkenLevel) ? 1 : 0;
	wMenuSetEnabled(smenu, WO_KEEP_AT_BOTTOM, !wwin->flags.miniaturized);

	/* omnipresent check */
	smenu->entries[WO_OMNIPRESENT]->clientdata = wwin;
	smenu->entries[WO_OMNIPRESENT]->flags.indicator_on = IS_OMNIPRESENT(wwin);

	smenu->flags.realized = 0;
	wMenuRealize(smenu);
}

static void updateMaximizeMenu(WMenu * menu, WWindow * wwin)
{
	WMenu *smenu = menu->cascades[menu->entries[MC_OTHERMAX]->cascade];

	smenu->entries[MAXC_V]->clientdata = wwin;
	smenu->entries[MAXC_H]->clientdata = wwin;
	smenu->entries[MAXC_LH]->clientdata = wwin;
	smenu->entries[MAXC_RH]->clientdata = wwin;
	smenu->entries[MAXC_TH]->clientdata = wwin;
	smenu->entries[MAXC_BH]->clientdata = wwin;
	smenu->entries[MAXC_LTC]->clientdata = wwin;
	smenu->entries[MAXC_RTC]->clientdata = wwin;
	smenu->entries[MAXC_LBC]->clientdata = wwin;
	smenu->entries[MAXC_RBC]->clientdata = wwin;
	smenu->entries[MAXC_MAXIMUS]->clientdata = wwin;

	smenu->flags.realized = 0;
	wMenuRealize(smenu);
}

static WMenu *makeWorkspaceMenu(WScreen * scr)
{
	WMenu *menu;

	menu = wMenuCreate(scr, NULL, False);
	if (!menu) {
		wwarning(_("could not create submenu for window menu"));
		return NULL;
	}

	updateWorkspaceMenu(menu);

	return menu;
}

static WMenu *makeMakeShortcutMenu(WMenu *menu)
{
	int i;

	for (i = 0; i < MAX_WINDOW_SHORTCUTS; i++) {
		WMenuEntry *entry;
		entry = wMenuAddCallback(menu, "", makeShortcutCommand, NULL);

		entry->flags.indicator = 1;
	}

	return menu;
}

static WMenu *makeOptionsMenu(WScreen * scr)
{
	WMenu *menu;
	WMenuEntry *entry;

	menu = wMenuCreate(scr, NULL, False);
	if (!menu) {
		wwarning(_("could not create submenu for window menu"));
		return NULL;
	}

	entry = wMenuAddCallback(menu, _("Keep on top"), execWindowOptionCommand, NULL);
	entry->flags.indicator = 1;
	entry->flags.indicator_type = MI_CHECK;

	entry = wMenuAddCallback(menu, _("Keep at bottom"), execWindowOptionCommand, NULL);
	entry->flags.indicator = 1;
	entry->flags.indicator_type = MI_CHECK;

	entry = wMenuAddCallback(menu, _("Omnipresent"), execWindowOptionCommand, NULL);
	entry->flags.indicator = 1;
	entry->flags.indicator_type = MI_CHECK;

	return menu;
}

static WMenu *makeMaximizeMenu(WScreen * scr)
{
	WMenu *menu;
	WMenuEntry *entry;

	menu = wMenuCreate(scr, NULL, False);
	if (!menu) {
		wwarning(_("could not create submenu for window menu"));
		return NULL;
	}

	entry = wMenuAddCallback(menu, _("Maximize vertically"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_VMAXIMIZE]);
	entry = wMenuAddCallback(menu, _("Maximize horizontally"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_HMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize left half"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_LHMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize right half"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_RHMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize top half"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_THMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize bottom half"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_BHMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize left top corner"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_LTCMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize right top corner"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_RTCMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize left bottom corner"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_LBCMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximize right bottom corner"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_RBCMAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Maximus: tiled maximization"), execMaximizeCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MAXIMUS]);

	return menu;
}

static WMenu *createWindowMenu(WScreen * scr)
{
	WMenu *menu;
	WMenuEntry *entry;

	menu = wMenuCreate(scr, NULL, False);
	/*
	 * Warning: If you make some change that affects the order of the
	 * entries, you must update the command enum in the top of
	 * this file.
	 */
	entry = wMenuAddCallback(menu, _("Maximize"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MAXIMIZE]);

	entry = wMenuAddCallback(menu, _("Other maximization"), NULL, NULL);
	wMenuEntrySetCascade(menu, entry, makeMaximizeMenu(scr));

	entry = wMenuAddCallback(menu, _("Miniaturize"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MINIATURIZE]);

	entry = wMenuAddCallback(menu, _("Shade"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_SHADE]);

	entry = wMenuAddCallback(menu, _("Hide"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_HIDE]);

	entry = wMenuAddCallback(menu, _("Resize/Move"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_MOVERESIZE]);

	entry = wMenuAddCallback(menu, _("Select"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_SELECT]);

	entry = wMenuAddCallback(menu, _("Move To"), NULL, NULL);
	w_global.workspace.submenu = makeWorkspaceMenu(scr);
	if (w_global.workspace.submenu)
		wMenuEntrySetCascade(menu, entry, w_global.workspace.submenu);

	entry = wMenuAddCallback(menu, _("Attributes..."), execMenuCommand, NULL);

	entry = wMenuAddCallback(menu, _("Options"), NULL, NULL);
	wMenuEntrySetCascade(menu, entry, makeMakeShortcutMenu(makeOptionsMenu(scr)));

	entry = wMenuAddCallback(menu, _("Launch"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_RELAUNCH]);

	entry = wMenuAddCallback(menu, _("Close"), execMenuCommand, NULL);
	entry->rtext = GetShortcutKey(wKeyBindings[WKBD_CLOSE]);

	entry = wMenuAddCallback(menu, _("Kill"), execMenuCommand, NULL);

	return menu;
}

void CloseWindowMenu(WScreen * scr)
{
	if (scr->window_menu) {
		if (scr->window_menu->flags.mapped)
			wMenuUnmap(scr->window_menu);

		if (scr->window_menu->entries[0]->clientdata) {
			WWindow *wwin = (WWindow *) scr->window_menu->entries[0]->clientdata;

			wwin->flags.menu_open_for_me = 0;
		}
		scr->window_menu->entries[0]->clientdata = NULL;
	}
}

static void updateMenuForWindow(WMenu * menu, WWindow * wwin)
{
	WApplication *wapp = wApplicationOf(wwin->main_window);
	int i;

	updateOptionsMenu(menu, wwin);
	updateMaximizeMenu(menu, wwin);

	updateMakeShortcutMenu(menu, wwin);

	wMenuSetEnabled(menu, MC_HIDE, wapp != NULL && !WFLAGP(wapp->main_window_desc, no_appicon));

	wMenuSetEnabled(menu, MC_CLOSE, (wwin->protocols.DELETE_WINDOW && !WFLAGP(wwin, no_closable)));

	if (wwin->flags.miniaturized) {
		static char *text = NULL;
		if (!text)
			text = _("Deminiaturize");

		menu->entries[MC_MINIATURIZE]->text = text;
	} else {
		static char *text = NULL;
		if (!text)
			text = _("Miniaturize");

		menu->entries[MC_MINIATURIZE]->text = text;
	}

	wMenuSetEnabled(menu, MC_MINIATURIZE, !WFLAGP(wwin, no_miniaturizable));

	if (wwin->flags.maximized) {
		static char *text = NULL;
		if (!text)
			text = _("Unmaximize");

		menu->entries[MC_MAXIMIZE]->text = text;
	} else {
		static char *text = NULL;
		if (!text)
			text = _("Maximize");

		menu->entries[MC_MAXIMIZE]->text = text;
	}
	wMenuSetEnabled(menu, MC_MAXIMIZE, IS_RESIZABLE(wwin));

	wMenuSetEnabled(menu, MC_MOVERESIZE, IS_RESIZABLE(wwin)
			&& !wwin->flags.miniaturized);

	if (wwin->flags.shaded) {
		static char *text = NULL;
		if (!text)
			text = _("Unshade");

		menu->entries[MC_SHADE]->text = text;
	} else {
		static char *text = NULL;
		if (!text)
			text = _("Shade");

		menu->entries[MC_SHADE]->text = text;
	}

	wMenuSetEnabled(menu, MC_SHADE, !WFLAGP(wwin, no_shadeable)
			&& !wwin->flags.miniaturized);

	if (wwin->flags.selected) {
		static char *text = NULL;
		if (!text)
			text = _("Deselect");

		menu->entries[MC_SELECT]->text = text;
	} else {
		static char *text = NULL;
		if (!text)
			text = _("Select");

		menu->entries[MC_SELECT]->text = text;
	}

	wMenuSetEnabled(menu, MC_DUMMY_MOVETO, !IS_OMNIPRESENT(wwin));

	if (!wwin->flags.inspector_open) {
		wMenuSetEnabled(menu, MC_PROPERTIES, True);
	} else {
		wMenuSetEnabled(menu, MC_PROPERTIES, False);
	}

	/* set the client data of the entries to the window */
	for (i = 0; i < menu->entry_no; i++) {
		menu->entries[i]->clientdata = wwin;
	}

	for (i = 0; i < w_global.workspace.submenu->entry_no; i++) {
		w_global.workspace.submenu->entries[i]->clientdata = wwin;

		if (i == w_global.workspace.current)
			wMenuSetEnabled(w_global.workspace.submenu, i, False);
		else
			wMenuSetEnabled(w_global.workspace.submenu, i, True);
	}

	menu->flags.realized = 0;
	wMenuRealize(menu);
}

static WMenu *open_window_menu_core(WWindow *wwin)
{
	WScreen *scr = wwin->screen_ptr;
	WMenu *menu;

	wwin->flags.menu_open_for_me = 1;

	if (!scr->window_menu) {
		scr->window_menu = createWindowMenu(scr);

		/* hack to save some memory allocation/deallocation */
		wfree(scr->window_menu->entries[MC_MINIATURIZE]->text);
		wfree(scr->window_menu->entries[MC_MAXIMIZE]->text);
		wfree(scr->window_menu->entries[MC_SHADE]->text);
		wfree(scr->window_menu->entries[MC_SELECT]->text);
	} else {
		updateWorkspaceMenu(w_global.workspace.submenu);
	}

	menu = scr->window_menu;
	if (menu->flags.mapped) {
		wMenuUnmap(menu);
		if (menu->entries[0]->clientdata == wwin)
			return NULL;
	}

	updateMenuForWindow(menu, wwin);

	return menu;
}

static void prepare_menu_position(WMenu *menu, int x, int y)
{
	WMRect rect;

	rect = wGetRectForHead(menu->frame->screen_ptr,
			       wGetHeadForPointerLocation(menu->frame->screen_ptr));
	if (x < rect.pos.x - menu->frame->core->width / 2)
		x = rect.pos.x - menu->frame->core->width / 2;
	if (y < rect.pos.y)
		y = rect.pos.y;
}

void OpenWindowMenu(WWindow *wwin, int x, int y, int keyboard)
{
	WMenu *menu;

	menu = open_window_menu_core(wwin);
	if (!menu)
		return;

	/* Specific menu position */
	x -= menu->frame->core->width / 2;
	if (x + menu->frame->core->width > wwin->frame_x + wwin->frame->core->width)
		x = wwin->frame_x + wwin->frame->core->width - menu->frame->core->width;
	if (x < wwin->frame_x)
		x = wwin->frame_x;

	/* Common menu position */
	prepare_menu_position(menu, x, y);

	if (!wwin->flags.internal_window)
		wMenuMapAt(menu, x, y, keyboard);
}

void OpenWindowMenu2(WWindow *wwin, int x, int y, int keyboard)
{
	int i;
	WMenu *menu;

	menu = open_window_menu_core(wwin);
	if (!menu)
		return;

	/* Specific menu position */
	for (i = 0; i < w_global.workspace.submenu->entry_no; i++) {
		w_global.workspace.submenu->entries[i]->clientdata = wwin;
		wMenuSetEnabled(w_global.workspace.submenu, i, True);
	}

	x -= menu->frame->core->width / 2;

	/* Common menu position */
	prepare_menu_position(menu, x, y);

	if (!wwin->flags.internal_window)
		wMenuMapAt(menu, x, y, keyboard);
}

void OpenMiniwindowMenu(WWindow * wwin, int x, int y)
{
	WMenu *menu;

	menu = open_window_menu_core(wwin);
	if (!menu)
		return;

	x -= menu->frame->core->width / 2;

	wMenuMapAt(menu, x, y, False);
}

void DestroyWindowMenu(WScreen *scr)
{
	if (scr->window_menu) {
		scr->window_menu->entries[MC_MINIATURIZE]->text = NULL;
		scr->window_menu->entries[MC_MAXIMIZE]->text = NULL;
		scr->window_menu->entries[MC_SHADE]->text = NULL;
		scr->window_menu->entries[MC_SELECT]->text = NULL;
		wMenuDestroy(scr->window_menu, True);
		scr->window_menu = NULL;
	}
}
