/* NoMenuAlert.c - warn user that menu can't be edited with WPrefs
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1999 Alfredo K. Kojima
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


#include "WPrefs.h"


typedef struct NoMenuPanel {
    WMWindow *wwin;

    WMLabel *text;

    WMButton *copyBtn;
    WMButton *keepBtn;

    int finished;
    int copy;
} NoMenuPanel;


#define MESSAGE_TEXT \
	"     The menu that is being used now could not be opened. "\
	"This either means that there is a syntax error in it or that "\
	"the menu is in a format not supported by WPrefs (WPrefs only "\
	"supports property list menus).\n"\
	"     If you want to keep using the current menu, please read "\
	"the '%s/%s' file, press 'Keep Current Menu' and edit it with a "\
	"text editor.\n"\
	"     If you want to use this editor, press 'Copy Default Menu'. "\
	"It will copy the default menu and will instruct Window Maker "\
	"to use it instead of the current one.\n"\
	"     If you want more flexibility, keep using the current one "\
	"as it allows you to use C preprocessor (cpp) macros, while being "\
	"easy to edit. Window Maker supports both formats."


static void
closeCallback(WMWidget *self, void *data)
{
    NoMenuPanel *panel = (NoMenuPanel*)data;

    panel->finished = True;
}


static void
buttonCallback(WMWidget *self, void *data)
{
    NoMenuPanel *panel = (NoMenuPanel*)data;

    panel->finished = True;
    if (self == panel->keepBtn)
	panel->copy = False;
    else
	panel->copy = True;
}


Bool
AskMenuCopy(WMWindow *wwin)
{
    NoMenuPanel panel;
    char buffer[2048];

    panel.wwin = WMCreatePanelForWindow(wwin, "noMenuAlert");
    WMResizeWidget(panel.wwin, 430, 260);
    WMSetWindowTitle(panel.wwin, "Warning");
    WMSetWindowCloseAction(panel.wwin, closeCallback, &panel);

    panel.text = WMCreateLabel(panel.wwin);
    WMResizeWidget(panel.text, 370, 200);
    WMMoveWidget(panel.text, 30, 20);

    sprintf(buffer, 
	    _("     The menu that is being used now could not be opened. "
	"This either means that there is a syntax error in it or that "
	"the menu is in a format not supported by WPrefs (WPrefs only "
	"supports property list menus).\n"
	"     If you want to keep using the current menu, please read "
	"the '%s/%s' file, press 'Keep Current Menu' and edit it with a "
	"text editor.\n"
	"     If you want to use this editor, press 'Copy Default Menu'. "
	"It will copy the default menu and will instruct Window Maker "
	"to use it instead of the current one.\n"
	"     If you want more flexibility, keep using the current one "
	"as it allows you to use C preprocessor (cpp) macros, while being "
	"easy to edit. Window Maker supports both formats."),
	    wusergnusteppath(), "Library/WindowMaker/README");
    WMSetLabelText(panel.text, buffer);

    panel.copyBtn = WMCreateCommandButton(panel.wwin);
    WMResizeWidget(panel.copyBtn, 180, 24);
    WMMoveWidget(panel.copyBtn, 30, 225);
    WMSetButtonText(panel.copyBtn, _("Copy Default Menu"));
    WMSetButtonAction(panel.copyBtn, buttonCallback, &panel);

    panel.keepBtn = WMCreateCommandButton(panel.wwin);
    WMResizeWidget(panel.keepBtn, 180, 24);
    WMMoveWidget(panel.keepBtn, 225, 225);
    WMSetButtonText(panel.keepBtn, _("Keep Current Menu"));
    WMSetButtonAction(panel.keepBtn, buttonCallback, &panel);

    WMMapSubwidgets(panel.wwin);
    WMRealizeWidget(panel.wwin);
    WMMapWidget(panel.wwin);

    panel.finished = False;
    panel.copy = False;

    while (!panel.finished) {
	XEvent event;

	WMNextEvent(WMScreenDisplay(WMWidgetScreen(panel.wwin)), &event);
	WMHandleEvent(&event);
    }

    WMDestroyWidget(panel.wwin);

    return panel.copy;
}


