/* dockedapp.c- docked application settings panel
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "WindowMaker.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "dock.h"
#include "dockedapp.h"
#include "dialog.h"
#include "misc.h"
#include "defaults.h"
#include "framewin.h"
#include "xinerama.h"


static void updateCommand(WAppIcon * icon, char *command)
{
	if (icon->command)
		wfree(icon->command);
	if (command && (command[0] == 0 || (command[0] == '-' && command[1] == 0))) {
		wfree(command);
		command = NULL;
	}
	icon->command = command;

	if (!icon->wm_class && !icon->wm_instance && icon->command && strlen(icon->command) > 0) {
		icon->forced_dock = 1;
	}
}

static void updatePasteCommand(WAppIcon * icon, char *command)
{
	if (icon->paste_command)
		wfree(icon->paste_command);
	if (command && (command[0] == 0 || (command[0] == '-' && command[1] == 0))) {
		wfree(command);
		command = NULL;
	}
	icon->paste_command = command;
}

#ifdef USE_DOCK_XDND
static void updateDNDCommand(WAppIcon * icon, char *command)
{
	if (icon->dnd_command)
		wfree(icon->dnd_command);
	if (command && (command[0] == 0 || (command[0] == '-' && command[1] == 0))) {
		wfree(command);
		command = NULL;
	}
	icon->dnd_command = command;
}
#endif	/* USE_DOCK_XDND */

static void updateSettingsPanelIcon(AppSettingsPanel * panel)
{
	char *file;
	int size;

	file = WMGetTextFieldText(panel->iconField);
	if (!file)
		WMSetLabelImage(panel->iconLabel, NULL);
	else {
		char *path;

		path = FindImage(wPreferences.icon_path, file);
		if (!path) {
			wwarning(_("could not find icon %s, used in a docked application"), file);
			wfree(file);
			WMSetLabelImage(panel->iconLabel, NULL);
			return;
		} else {
			WMPixmap *pixmap;
			RColor color;

			color.red = 0xae;
			color.green = 0xaa;
			color.blue = 0xae;
			color.alpha = 0;
			size = wPreferences.icon_size;
			pixmap = WMCreateScaledBlendedPixmapFromFile(WMWidgetScreen(panel->win), path, &color, size, size);
			if (!pixmap) {
				WMSetLabelImage(panel->iconLabel, NULL);
			} else {
				WMSetLabelImage(panel->iconLabel, pixmap);
				WMReleasePixmap(pixmap);
			}
		}
		wfree(file);
		wfree(path);
	}
}

static void chooseIconCallback(WMWidget * self, void *clientData)
{
	char *file;
	AppSettingsPanel *panel = (AppSettingsPanel *) clientData;
	int result;

	/* Parameter not used, but tell the compiler that it is ok */
	(void) self;

	panel->choosingIcon = 1;

	WMSetButtonEnabled(panel->browseBtn, False);

	result = wIconChooserDialog(panel->wwin->screen_ptr, &file,
				    panel->editedIcon->wm_instance, panel->editedIcon->wm_class);

	panel->choosingIcon = 0;
	if (!panel->destroyed) {
		if (result) {
			WMSetTextFieldText(panel->iconField, file);
			updateSettingsPanelIcon(panel);
		}

		WMSetButtonEnabled(panel->browseBtn, True);
	} else {
		/* kluge for the case, the user asked to close the panel before
		 * the icon chooser */
		DestroyDockAppSettingsPanel(panel);
	}
	if (result)
		wfree(file);
}

static void panelBtnCallback(WMWidget * self, void *data)
{
	WMButton *btn = self;
	AppSettingsPanel *panel = (AppSettingsPanel *) data;
	char *text;

	if (panel->okBtn == btn) {
		text = WMGetTextFieldText(panel->iconField);
		if (text[0] == 0) {
			wfree(text);
			text = NULL;
		}

		if (!wIconChangeImageFile(panel->editedIcon->icon, text)) {
			char *buf;
			int len = strlen(text) + 64;

			buf = wmalloc(len);
			snprintf(buf, len, _("Could not open specified icon file: %s"), text);
			if (wMessageDialog(panel->wwin->screen_ptr, _("Error"), buf,
					   _("OK"), _("Ignore"), NULL) == WAPRDefault) {
				wfree(text);
				wfree(buf);
				return;
			}
			wfree(buf);
		} else {
			WAppIcon *aicon = panel->editedIcon;

			// Cf dock.c:dockIconPaint(WAppIcon *aicon)?
			if (aicon == aicon->icon->core->screen_ptr->clip_icon)
				wClipIconPaint(aicon);
			else if (wIsADrawer(aicon))
				wDrawerIconPaint(aicon);
			else
				wAppIconPaint(aicon);

			wDefaultChangeIcon(aicon->wm_instance, aicon->wm_class, text);
		}
		if (text)
			wfree(text);

		/* cannot free text from this, because it will be not be duplicated
		 * in updateCommand */
		text = WMGetTextFieldText(panel->commandField);
		if (text[0] == 0) {
			wfree(text);
			text = NULL;
		}
		updateCommand(panel->editedIcon, text);
#ifdef USE_DOCK_XDND
		/* cannot free text from this, because it will be not be duplicated
		 * in updateDNDCommand */
		text = WMGetTextFieldText(panel->dndCommandField);
		updateDNDCommand(panel->editedIcon, text);
#endif
		text = WMGetTextFieldText(panel->pasteCommandField);
		updatePasteCommand(panel->editedIcon, text);

		panel->editedIcon->auto_launch = WMGetButtonSelected(panel->autoLaunchBtn);

		panel->editedIcon->lock = WMGetButtonSelected(panel->lockBtn);
	}

	DestroyDockAppSettingsPanel(panel);
}

/* These macros are used to scale the coordinates in ShowDockAppSettingsPanel.
 * They are based on the width of "abcdefghijklmnopqrstuvwxyz" and the height
 * of the default system font (Sans, size 11): 164x14.
 */
#define SCALEX(value) ((int)((double)value / 164.0 * (double)fw + 0.5))
#define SCALEY(value) ((int)((double)value / 14.0 * (double)fh + 0.5))

void ShowDockAppSettingsPanel(WAppIcon * aicon)
{
	AppSettingsPanel *panel;
	WScreen *scr = aicon->icon->core->screen_ptr;
	Window parent;
	WMFont *font;
	int x, y;
	WMBox *vbox;
	int fw, fh;
	int pwidth, pheight;
	int iconSize;

	/* get the width and height values of the system font for use in the SCALE macros */
	font = WMDefaultSystemFont(scr->wmscreen);
	fw = WMWidthOfString(font, "abcdefghijklmnopqrstuvwxyz", 26);
	fh = WMFontHeight(font);
	WMReleaseFont(font);

	/* calculate the required width and height for the panel */
	iconSize = wPreferences.icon_size;
	pwidth = SCALEX(300);
	pheight = SCALEY(10)                /* upper margin */
		+ iconSize                  /* icon and its label */
		+ SCALEY(10)                /* padding */
		+ SCALEY(20) + SCALEY(2)    /* start option */
		+ SCALEY(20) + SCALEY(5)    /* lock option */
		+ SCALEY(50) + SCALEY(5)    /* app path and arguments */
		+ SCALEY(70) + SCALEY(5)    /* middle-click command */
		+ SCALEY(70) + SCALEY(5)    /* drag&drop command */
		+ SCALEY(50) + SCALEY(10)   /* icon file */
		+ SCALEY(24)                /* buttons */
		+ SCALEY(10);               /* lower margin */

	panel = wmalloc(sizeof(AppSettingsPanel));

	panel->editedIcon = aicon;

	aicon->panel = panel;
	aicon->editing = 1;

	panel->win = WMCreateWindow(scr->wmscreen, "applicationSettings");
	WMResizeWidget(panel->win, pwidth, pheight);

	panel->iconLabel = WMCreateLabel(panel->win);
	WMResizeWidget(panel->iconLabel, iconSize, iconSize);
	WMMoveWidget(panel->iconLabel, SCALEX(10), SCALEY(10));
	WMSetLabelImagePosition(panel->iconLabel, WIPImageOnly);

	panel->nameLabel = WMCreateLabel(panel->win);
	font = WMBoldSystemFontOfSize(scr->wmscreen, SCALEY(14));
	WMResizeWidget(panel->nameLabel, SCALEX(190), SCALEY(18));
	WMMoveWidget(panel->nameLabel, 2 * SCALEX(10) + iconSize, SCALEY(10) + ((iconSize - WMFontHeight(font)) / 2));
	WMSetLabelTextAlignment(panel->nameLabel, WALeft);
	WMSetLabelFont(panel->nameLabel, font);
	WMReleaseFont(font);
	if (aicon->wm_class && strcmp(aicon->wm_class, "DockApp") == 0)
		WMSetLabelText(panel->nameLabel, aicon->wm_instance);
	else
		WMSetLabelText(panel->nameLabel, aicon->wm_class);

	vbox = WMCreateBox(panel->win);
	WMResizeWidget(vbox, pwidth - 2 * SCALEX(10), pheight - iconSize - 3 * SCALEY(10));
	WMMoveWidget(vbox, SCALEX(10), iconSize + 2 * SCALEY(10));

	panel->autoLaunchBtn = WMCreateSwitchButton(vbox);
	WMAddBoxSubview(vbox, WMWidgetView(panel->autoLaunchBtn), False, True, SCALEY(20), SCALEY(20), SCALEY(2));
	WMSetButtonText(panel->autoLaunchBtn, _("Start when Window Maker is started"));
	WMSetButtonSelected(panel->autoLaunchBtn, aicon->auto_launch);

	panel->lockBtn = WMCreateSwitchButton(vbox);
	WMAddBoxSubview(vbox, WMWidgetView(panel->lockBtn), False, True, SCALEY(20), SCALEY(20), SCALEY(5));
	WMSetButtonText(panel->lockBtn, _("Lock (prevent accidental removal)"));
	WMSetButtonSelected(panel->lockBtn, aicon->lock);

	panel->commandFrame = WMCreateFrame(vbox);
	WMSetFrameTitle(panel->commandFrame, _("Application path and arguments"));
	WMAddBoxSubview(vbox, WMWidgetView(panel->commandFrame), False, True, SCALEY(50), SCALEY(50), SCALEY(5));

	panel->commandField = WMCreateTextField(panel->commandFrame);
	WMResizeWidget(panel->commandField, SCALEX(260), SCALEY(20));
	WMMoveWidget(panel->commandField, SCALEX(10), SCALEY(20));
	WMSetTextFieldText(panel->commandField, aicon->command);

	WMMapSubwidgets(panel->commandFrame);

	panel->pasteCommandFrame = WMCreateFrame(vbox);
	WMSetFrameTitle(panel->pasteCommandFrame, _("Command for middle-click launch"));
	WMAddBoxSubview(vbox, WMWidgetView(panel->pasteCommandFrame), False, True, SCALEY(70), SCALEY(70), SCALEY(5));

	panel->pasteCommandField = WMCreateTextField(panel->pasteCommandFrame);
	WMResizeWidget(panel->pasteCommandField, SCALEX(260), SCALEY(20));
	WMMoveWidget(panel->pasteCommandField, SCALEX(10), SCALEY(20));

	panel->pasteCommandLabel = WMCreateLabel(panel->pasteCommandFrame);
	WMResizeWidget(panel->pasteCommandLabel, SCALEX(260), SCALEY(18));
	WMMoveWidget(panel->pasteCommandLabel, SCALEX(10), SCALEY(45));

	WMSetTextFieldText(panel->pasteCommandField, aicon->paste_command);
	WMSetLabelText(panel->pasteCommandLabel, _("%s will be replaced with current selection"));
	WMMapSubwidgets(panel->pasteCommandFrame);

	panel->dndCommandFrame = WMCreateFrame(vbox);
	WMSetFrameTitle(panel->dndCommandFrame, _("Command for dragged and dropped files"));
	WMAddBoxSubview(vbox, WMWidgetView(panel->dndCommandFrame), False, True, SCALEY(70), SCALEY(70), SCALEY(5));

	panel->dndCommandField = WMCreateTextField(panel->dndCommandFrame);
	WMResizeWidget(panel->dndCommandField, SCALEX(260), SCALEY(20));
	WMMoveWidget(panel->dndCommandField, SCALEX(10), SCALEY(20));

	panel->dndCommandLabel = WMCreateLabel(panel->dndCommandFrame);
	WMResizeWidget(panel->dndCommandLabel, SCALEX(260), SCALEY(18));
	WMMoveWidget(panel->dndCommandLabel, SCALEX(10), SCALEY(45));
#ifdef USE_DOCK_XDND
	WMSetTextFieldText(panel->dndCommandField, aicon->dnd_command);
	WMSetLabelText(panel->dndCommandLabel, _("%d will be replaced with the file name"));
#else
	WMSetTextFieldEditable(panel->dndCommandField, False);
	WMSetLabelText(panel->dndCommandLabel, _("XDnD support was not compiled in"));

	WMSetFrameTitleColor(panel->dndCommandFrame, WMDarkGrayColor(scr->wmscreen));
	WMSetLabelTextColor(panel->dndCommandLabel, WMDarkGrayColor(scr->wmscreen));
#endif
	WMMapSubwidgets(panel->dndCommandFrame);

	panel->iconFrame = WMCreateFrame(vbox);
	WMSetFrameTitle(panel->iconFrame, _("Icon Image"));
	WMAddBoxSubview(vbox, WMWidgetView(panel->iconFrame), False, True, SCALEY(50), SCALEY(50), SCALEY(10));

	panel->iconField = WMCreateTextField(panel->iconFrame);
	WMResizeWidget(panel->iconField, SCALEX(180), SCALEY(20));
	WMMoveWidget(panel->iconField, SCALEX(10), SCALEY(20));
	WMSetTextFieldText(panel->iconField, wDefaultGetIconFile(aicon->wm_instance, aicon->wm_class, False));

	panel->browseBtn = WMCreateCommandButton(panel->iconFrame);
	WMResizeWidget(panel->browseBtn, SCALEX(70), SCALEY(24));
	WMMoveWidget(panel->browseBtn, SCALEX(200), SCALEY(18));
	WMSetButtonText(panel->browseBtn, _("Browse..."));
	WMSetButtonAction(panel->browseBtn, chooseIconCallback, panel);

	{
		WMBox *hbox;

		hbox = WMCreateBox(vbox);
		WMSetBoxHorizontal(hbox, True);
		WMAddBoxSubview(vbox, WMWidgetView(hbox), False, True, SCALEY(24), SCALEY(24), 0);

		panel->okBtn = WMCreateCommandButton(hbox);
		WMSetButtonText(panel->okBtn, _("OK"));
		WMSetButtonAction(panel->okBtn, panelBtnCallback, panel);
		WMAddBoxSubviewAtEnd(hbox, WMWidgetView(panel->okBtn), False, True, SCALEX(80), SCALEX(80), 0);

		panel->cancelBtn = WMCreateCommandButton(hbox);
		WMSetButtonText(panel->cancelBtn, _("Cancel"));
		WMSetButtonAction(panel->cancelBtn, panelBtnCallback, panel);
		WMAddBoxSubviewAtEnd(hbox, WMWidgetView(panel->cancelBtn), False, True, SCALEX(80), SCALEX(80), 5);

		WMMapSubwidgets(hbox);
	}

	WMRealizeWidget(panel->win);
	WMMapSubwidgets(panel->win);
	WMMapSubwidgets(vbox);
	WMMapSubwidgets(panel->iconFrame);

	updateSettingsPanelIcon(panel);

	parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, pwidth, pheight, 0, 0, 0);
	XSelectInput(dpy, parent, KeyPressMask | KeyReleaseMask);

	XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

	/*
	 * make things relative to head
	 */
	{
		WMRect rect = wGetRectForHead(scr, wGetHeadForPointerLocation(scr));

		y = aicon->y_pos;
		if (y < 0)
			y = 0;
		else if (y + pheight > rect.pos.y + rect.size.height)
			y = rect.pos.y + rect.size.height - pheight - 3 * SCALEY(10);

		if (aicon->dock && aicon->dock->type == WM_DOCK) {
			if (aicon->dock->on_right_side)
				x = rect.pos.x + rect.size.width / 2;
			else
				x = rect.pos.x + rect.size.width / 2 - pwidth - SCALEX(2);
		} else {
			x = rect.pos.x + (rect.size.width - pwidth) / 2;
		}
	}

	panel->wwin = wManageInternalWindow(scr, parent, None,
					    _("Docked Application Settings"), x, y, pwidth, pheight);

	panel->wwin->client_leader = WMWidgetXID(panel->win);

	panel->parent = parent;

	WMMapWidget(panel->win);

	wWindowMap(panel->wwin);
}

void DestroyDockAppSettingsPanel(AppSettingsPanel * panel)
{
	if (!panel->destroyed) {
		XUnmapWindow(dpy, panel->wwin->client_win);
		XReparentWindow(dpy, panel->wwin->client_win, panel->wwin->screen_ptr->root_win, 0, 0);
		wUnmanageWindow(panel->wwin, False, False);
	}

	panel->destroyed = 1;

	/*
	 * kluge. If we destroy the panel before the icon chooser is closed,
	 * we will crash when it does close, trying to access something in the
	 * destroyed panel. Could use wretain()/wrelease() in the panel,
	 * but it is not working for some reason.
	 */
	if (panel->choosingIcon)
		return;

	WMDestroyWidget(panel->win);

	XDestroyWindow(dpy, panel->parent);

	panel->editedIcon->panel = NULL;

	panel->editedIcon->editing = 0;

	wfree(panel);
}
