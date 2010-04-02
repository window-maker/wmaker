/* Expert.c- expert user options
 *
 *  WPrefs - Window Maker Preferences Program
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#include "WPrefs.h"

typedef struct _Panel {
	WMBox *box;
	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;

	WMButton *swi[9];

} _Panel;

#define ICON_FILE	"expert"

static void showData(_Panel * panel)
{
	WMUserDefaults *udb = WMGetStandardUserDefaults();

	WMSetButtonSelected(panel->swi[0], GetBoolForKey("DisableMiniwindows"));
	WMSetButtonSelected(panel->swi[1], WMGetUDBoolForKey(udb, "NoXSetStuff"));
	WMSetButtonSelected(panel->swi[2], GetBoolForKey("SaveSessionOnExit"));
	WMSetButtonSelected(panel->swi[3], GetBoolForKey("UseSaveUnders"));
	WMSetButtonSelected(panel->swi[4], GetBoolForKey("DontConfirmKill"));
	WMSetButtonSelected(panel->swi[5], GetBoolForKey("DisableBlinking"));
	WMSetButtonSelected(panel->swi[6], GetBoolForKey("AntialiasedText"));
	WMSetButtonSelected(panel->swi[7], GetBoolForKey("SingleClickLaunch"));
	WMSetButtonSelected(panel->swi[8], GetBoolForKey("CycleActiveHeadOnly"));
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	WMScrollView *sv;
	WMFrame *f;
	int i;

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

	sv = WMCreateScrollView(panel->box);
	WMResizeWidget(sv, 500, 215);
	WMMoveWidget(sv, 12, 10);
	WMSetScrollViewRelief(sv, WRSunken);
	WMSetScrollViewHasVerticalScroller(sv, True);
	WMSetScrollViewHasHorizontalScroller(sv, False);

	f = WMCreateFrame(panel->box);
	WMResizeWidget(f, 495, sizeof(panel->swi) / sizeof(char *) * 25 + 8);
	WMSetFrameRelief(f, WRFlat);

	for (i = 0; i < sizeof(panel->swi) / sizeof(char *); i++) {
		panel->swi[i] = WMCreateSwitchButton(f);
		WMResizeWidget(panel->swi[i], FRAME_WIDTH - 40, 25);
		WMMoveWidget(panel->swi[i], 5, 5 + i * 25);
	}

	WMSetButtonText(panel->swi[0],
			_("Disable miniwindows (icons for minimized windows). For use with KDE/GNOME."));
	WMSetButtonText(panel->swi[1], _("Do not set non-WindowMaker specific parameters (do not use xset)."));
	WMSetButtonText(panel->swi[2], _("Automatically save session when exiting Window Maker."));
	WMSetButtonText(panel->swi[3], _("Use SaveUnder in window frames, icons, menus and other objects."));
	WMSetButtonText(panel->swi[4], _("Disable confirmation panel for the Kill command."));
	WMSetButtonText(panel->swi[5], _("Disable selection animation for selected icons."));
	WMSetButtonText(panel->swi[6], _("Smooth font edges (needs restart)."));
	WMSetButtonText(panel->swi[7], _("Launch applications and restore windows with a single click."));
	WMSetButtonText(panel->swi[8], _("Cycle windows only on the active head."));

	WMSetButtonEnabled(panel->swi[6], True);

	WMMapSubwidgets(panel->box);
	WMSetScrollViewContentView(sv, WMWidgetView(f));
	WMRealizeWidget(panel->box);

	showData(panel);
}

static void storeDefaults(_Panel * panel)
{
	WMUserDefaults *udb = WMGetStandardUserDefaults();

	SetBoolForKey(WMGetButtonSelected(panel->swi[0]), "DisableMiniwindows");

	WMSetUDBoolForKey(udb, WMGetButtonSelected(panel->swi[1]), "NoXSetStuff");

	SetBoolForKey(WMGetButtonSelected(panel->swi[2]), "SaveSessionOnExit");
	SetBoolForKey(WMGetButtonSelected(panel->swi[3]), "UseSaveUnders");
	SetBoolForKey(WMGetButtonSelected(panel->swi[4]), "DontConfirmKill");
	SetBoolForKey(WMGetButtonSelected(panel->swi[5]), "DisableBlinking");
	SetBoolForKey(WMGetButtonSelected(panel->swi[6]), "AntialiasedText");
	SetBoolForKey(WMGetButtonSelected(panel->swi[7]), "SingleClickLaunch");
	SetBoolForKey(WMGetButtonSelected(panel->swi[8]), "CycleActiveHeadOnly");
}

Panel *InitExpert(WMScreen * scr, WMWidget * parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));
	memset(panel, 0, sizeof(_Panel));

	panel->sectionName = _("Expert User Preferences");

	panel->description = _("Options for people who know what they're doing...\n"
			       "Also have some other misc. options.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeDefaults;

	AddSection(panel, ICON_FILE);

	return panel;
}
