/* dockedapp.c- docked application settings panel
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "dock.h"
#include "dialog.h"
#include "funcs.h"
#include "defaults.h"
#include "framewin.h"


/**** Global variables ****/
extern WPreferences wPreferences;



typedef struct _AppSettingsPanel {
    WMWindow *win;
    WAppIcon *editedIcon;
    
    WWindow *wwin;

    WMLabel *iconLabel;
    WMLabel *nameLabel;
    
    WMFrame *commandFrame;
    WMTextField *commandField;

    WMFrame *dndCommandFrame;
    WMTextField *dndCommandField;
    WMLabel *dndCommandLabel;

    WMFrame *iconFrame;
    WMTextField *iconField;
    WMButton *browseBtn;

    WMButton *autoLaunchBtn;

    WMButton *okBtn;
    WMButton *cancelBtn;

    Window parent;

    /* kluge */
    unsigned int destroyed:1;
    unsigned int choosingIcon:1;
} AppSettingsPanel;


void DestroyDockAppSettingsPanel(AppSettingsPanel *panel);


static void
updateCommand(WAppIcon *icon, char *command)
{
    if (icon->command)
	free(icon->command);
    if (command && (command[0]==0 || (command[0]=='-' && command[1]==0))) {
	free(command);
	command = NULL;
    }
    icon->command = command;

    if (!icon->wm_class && !icon->wm_instance && icon->command 
	&& strlen(icon->command)>0) {
	icon->forced_dock = 1;
    }
}


#ifdef OFFIX_DND
static void
updateDNDCommand(WAppIcon *icon, char *command)
{
    if (icon->dnd_command)
	free(icon->dnd_command);
    if (command && (command[0]==0 || (command[0]=='-' && command[1]==0))) {
	free(command);
	command = NULL;
    }
    icon->dnd_command = command;
}
#endif /* OFFIX_DND */


static void
updateSettingsPanelIcon(AppSettingsPanel *panel)
{
    char *file;
    
    file = WMGetTextFieldText(panel->iconField);
    if (!file)
	WMSetLabelImage(panel->iconLabel, NULL);
    else {
	char *path;
	
	path = FindImage(wPreferences.icon_path, file);
	if (!path) {
	    wwarning(_("could not find icon %s, used in a docked application"),
		     file);
	    free(file);
	    WMSetLabelImage(panel->iconLabel, NULL);
	    return;
	} else {
	    WMPixmap *pixmap;
	    RColor color;

	    color.red = 0xae;
	    color.green = 0xaa;
	    color.blue = 0xae;
	    color.alpha = 0;
	    pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
						   path, &color);
	    if (!pixmap) {
		WMSetLabelImage(panel->iconLabel, NULL);
	    } else {
		WMSetLabelImage(panel->iconLabel, pixmap);
		WMReleasePixmap(pixmap);
	    }
	}
	free(file);
	free(path);
    }
}


static void
chooseIconCallback(WMWidget *self, void *clientData)
{
    char *file;
    AppSettingsPanel *panel = (AppSettingsPanel*)clientData;
    int result;

    panel->choosingIcon = 1;

    WMSetButtonEnabled(panel->browseBtn, False);
    
    result = wIconChooserDialog(panel->wwin->screen_ptr, &file, 
				panel->editedIcon->wm_instance,
				panel->editedIcon->wm_class);

    panel->choosingIcon = 0;
    if (!panel->destroyed) {
	if (result) {
	    WMSetTextFieldText(panel->iconField, file);
	    free(file);
	    updateSettingsPanelIcon(panel);
	}
    
	WMSetButtonEnabled(panel->browseBtn, True);
    } else {
	/* kluge for the case, the user asked to close the panel before
	 * the icon chooser */
	DestroyDockAppSettingsPanel(panel);
    }
}


static void
panelBtnCallback(WMWidget *self, void *data)
{
    WMButton *btn = self;
    AppSettingsPanel *panel = (AppSettingsPanel*)data;
    char *text;
    int done;
    
    done = 1;
    if (panel->okBtn == btn) {
	text = WMGetTextFieldText(panel->iconField);
	if (text[0]==0) {
	    free(text);
	    text = NULL;
	}
	if (!wIconChangeImageFile(panel->editedIcon->icon, text)) {
	    char *buf;
	    
	    buf = wmalloc(strlen(text) + 64);
	    sprintf(buf, _("Could not open specified icon file:%s"), text);
	    wMessageDialog(panel->wwin->screen_ptr, _("Error"), buf, _("OK"),
			   NULL, NULL);
	    free(buf);
	    done = 0;
	    return;
	} else {
	    WAppIcon *aicon = panel->editedIcon;

	    if (aicon == aicon->icon->core->screen_ptr->clip_icon)
		wClipIconPaint(aicon);
	    else
		wAppIconPaint(aicon);

	    wDefaultChangeIcon(panel->wwin->screen_ptr, aicon->wm_instance,
			       aicon->wm_class, text);
	}
	if (text)
	    free(text);

	/* cannot free text from this, because it will be not be duplicated
	 * in updateCommand */
	text = WMGetTextFieldText(panel->commandField);
	if (text[0]==0) {
	    free(text);
	    text = NULL;
	}
	updateCommand(panel->editedIcon, text);
#ifdef OFFIX_DND
	/* cannot free text from this, because it will be not be duplicated
	 * in updateDNDCommand */
	text = WMGetTextFieldText(panel->dndCommandField);
	updateDNDCommand(panel->editedIcon, text);
#endif

	panel->editedIcon->auto_launch = 
	    WMGetButtonSelected(panel->autoLaunchBtn);
    }

    if (done)
	DestroyDockAppSettingsPanel(panel);
}


#define PWIDTH	295
#define PHEIGHT	345


void 
ShowDockAppSettingsPanel(WAppIcon *aicon)
{
    AppSettingsPanel *panel;
    WScreen *scr = aicon->icon->core->screen_ptr;
    Window parent;
    WMFont *font;
    int x, y;
    
    panel = wmalloc(sizeof(AppSettingsPanel));
    memset(panel, 0, sizeof(AppSettingsPanel));

    panel->editedIcon = aicon;
    
    aicon->panel = panel;
    aicon->editing = 1;
    
    panel->win = WMCreateWindow(scr->wmscreen, "applicationSettings");
    WMResizeWidget(panel->win, PWIDTH, PHEIGHT);

    panel->iconLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconLabel, 64, 64);
    WMMoveWidget(panel->iconLabel, 10, 10);
    WMSetLabelImagePosition(panel->iconLabel, WIPImageOnly);
    
    panel->nameLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->nameLabel, 190, 18);
    WMMoveWidget(panel->nameLabel, 80, 35);
    WMSetLabelTextAlignment(panel->nameLabel, WALeft);
    font = WMBoldSystemFontOfSize(scr->wmscreen, 14);
    WMSetLabelFont(panel->nameLabel, font);
    WMReleaseFont(font);
    WMSetLabelText(panel->nameLabel, aicon->wm_class);

    panel->autoLaunchBtn = WMCreateSwitchButton(panel->win);
    WMResizeWidget(panel->autoLaunchBtn, PWIDTH-30, 20);
    WMMoveWidget(panel->autoLaunchBtn, 15, 80);
    WMSetButtonText(panel->autoLaunchBtn, 
		    _("Start when WindowMaker is started"));
    WMSetButtonSelected(panel->autoLaunchBtn, aicon->auto_launch);

    
    panel->commandFrame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->commandFrame, 275, 50);
    WMMoveWidget(panel->commandFrame, 10, 105);
    WMSetFrameTitle(panel->commandFrame, _("Application path and arguments"));

    panel->commandField = WMCreateTextField(panel->commandFrame);
    WMResizeWidget(panel->commandField, 256, 20);
    WMMoveWidget(panel->commandField, 10, 20);
    WMSetTextFieldText(panel->commandField, aicon->command);
    
    panel->dndCommandFrame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->dndCommandFrame, 275, 70);
    WMMoveWidget(panel->dndCommandFrame, 10, 165);
    WMSetFrameTitle(panel->dndCommandFrame, 
		   _("Command for files dropped with DND"));
    
    panel->dndCommandField = WMCreateTextField(panel->dndCommandFrame);
    WMResizeWidget(panel->dndCommandField, 256, 20);
    WMMoveWidget(panel->dndCommandField, 10, 20);
    
    panel->dndCommandLabel = WMCreateLabel(panel->dndCommandFrame);
    WMResizeWidget(panel->dndCommandLabel, 256, 18);
    WMMoveWidget(panel->dndCommandLabel, 10, 45);
#ifdef OFFIX_DND
    WMSetTextFieldText(panel->dndCommandField, aicon->dnd_command);
    WMSetLabelText(panel->dndCommandLabel, 
		   _("%d will be replaced with the file name"));
#else
    WMSetTextFieldEditable(panel->dndCommandField, False);
    WMSetLabelText(panel->dndCommandLabel,
		   _("DND support was not compiled in"));    
#endif

    panel->iconFrame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->iconFrame, 275, 50);
    WMMoveWidget(panel->iconFrame, 10, 245);
    WMSetFrameTitle(panel->iconFrame, _("Icon Image"));
    
    panel->iconField = WMCreateTextField(panel->iconFrame);
    WMResizeWidget(panel->iconField, 176, 20);
    WMMoveWidget(panel->iconField, 10, 20);
    WMSetTextFieldText(panel->iconField, 
		       wDefaultGetIconFile(scr, aicon->wm_instance, 
					   aicon->wm_class, True));
    
    panel->browseBtn = WMCreateCommandButton(panel->iconFrame);
    WMResizeWidget(panel->browseBtn, 70, 24);
    WMMoveWidget(panel->browseBtn, 195, 18);
    WMSetButtonText(panel->browseBtn, _("Browse..."));
    WMSetButtonAction(panel->browseBtn, chooseIconCallback, panel);

    
    panel->okBtn = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->okBtn, 80, 26);
    WMMoveWidget(panel->okBtn, 200, 308);
    WMSetButtonText(panel->okBtn, _("OK"));
    WMSetButtonAction(panel->okBtn, panelBtnCallback, panel);
    
    panel->cancelBtn = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancelBtn, 80, 26);
    WMMoveWidget(panel->cancelBtn, 110, 308);
    WMSetButtonText(panel->cancelBtn, _("Cancel"));
    WMSetButtonAction(panel->cancelBtn, panelBtnCallback, panel);
    
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);
    WMMapSubwidgets(panel->commandFrame);
    WMMapSubwidgets(panel->dndCommandFrame);
    WMMapSubwidgets(panel->iconFrame);

    updateSettingsPanelIcon(panel);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, PWIDTH, PHEIGHT,
				 0, 0, 0);
    XSelectInput(dpy, parent, KeyPressMask|KeyReleaseMask);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);
    
    y = aicon->y_pos;
    if (y < 0)
	y = 0;
    else if (y + PWIDTH > scr->scr_height)
	y = scr->scr_height - PHEIGHT - 30;

    if (aicon->dock && aicon->dock->type == WM_DOCK) {
	if (aicon->dock->on_right_side)
	    x = scr->scr_width/2;
	else
	    x = scr->scr_width/2 - PWIDTH;
    } else {
	x = (scr->scr_width - PWIDTH)/2;
    }
    panel->wwin = wManageInternalWindow(scr, parent, None, 
					_("Docked Application Settings"),
					x, y, PWIDTH, PHEIGHT);
    
    panel->wwin->client_leader = WMWidgetXID(panel->win);

    panel->parent = parent;

    WMMapWidget(panel->win);
    
    wWindowMap(panel->wwin);
}


void
DestroyDockAppSettingsPanel(AppSettingsPanel *panel)
{
    if (!panel->destroyed) {
	XUnmapWindow(dpy, panel->wwin->client_win);
	XReparentWindow(dpy, panel->wwin->client_win, 
			panel->wwin->screen_ptr->root_win, 0, 0);
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

    free(panel);
}
