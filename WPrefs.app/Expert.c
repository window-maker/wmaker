/* Expert.c- expert user options
 * 
 *  WPrefs - Window Maker Preferences Program
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


#include "WPrefs.h"

typedef struct _Panel {
    WMBox *box;
    char *sectionName;

    char *description;

    CallbackRec callbacks;
    
    WMWidget *parent;

    WMButton *swi[8];

} _Panel;



#define ICON_FILE	"expert"


static void
showData(_Panel *panel)
{
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    WMSetButtonSelected(panel->swi[0], GetBoolForKey("DisableMiniwindows"));
    WMSetButtonSelected(panel->swi[1], WMGetUDBoolForKey(udb, "NoXSetStuff"));
    WMSetButtonSelected(panel->swi[2], GetBoolForKey("SaveSessionOnExit"));
    WMSetButtonSelected(panel->swi[3], GetBoolForKey("UseSaveUnders"));
    WMSetButtonSelected(panel->swi[4], GetBoolForKey("WindozeCycling"));
    WMSetButtonSelected(panel->swi[5], GetBoolForKey("DontConfirmKill"));
    WMSetButtonSelected(panel->swi[6], GetBoolForKey("DisableBlinking"));
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    int i;

    panel->box = WMCreateBox(panel->parent);
    WMSetBoxExpandsToParent(panel->box, 2, 2, 0, 0);

    for (i=0; i<7; i++) {
	panel->swi[i] = WMCreateSwitchButton(panel->box);
	WMResizeWidget(panel->swi[i], FRAME_WIDTH-40, 25);
	WMMoveWidget(panel->swi[i], 20, 20+i*25);
    }
    
    WMSetButtonText(panel->swi[0], _("Disable miniwindows (icons for miniaturized windows). For use with KDE/GNOME."));
    WMSetButtonText(panel->swi[1], _("Do not set non-WindowMaker specific parameters (do not use xset)"));
    WMSetButtonText(panel->swi[2], _("Automatically save session when exiting Window Maker"));
    WMSetButtonText(panel->swi[3], _("Use SaveUnder in window frames, icons, menus and other objects"));
    WMSetButtonText(panel->swi[4], _("Use Windoze style cycling"));
    WMSetButtonText(panel->swi[5], _("Disable confirmation panel for the Kill command"));
    WMSetButtonText(panel->swi[6], _("Disable cycling color highlighting of icons"));

    WMRealizeWidget(panel->box);
    WMMapSubwidgets(panel->box);
    
    showData(panel);
}


static void
storeDefaults(_Panel *panel)
{
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    SetBoolForKey(WMGetButtonSelected(panel->swi[0]), "DisableMiniwindows");

    WMSetUDBoolForKey(udb, WMGetButtonSelected(panel->swi[1]), "NoXSetStuff");

    SetBoolForKey(WMGetButtonSelected(panel->swi[2]), "SaveSessionOnExit");
    SetBoolForKey(WMGetButtonSelected(panel->swi[3]), "UseSaveUnders");
    SetBoolForKey(WMGetButtonSelected(panel->swi[4]), "WindozeCycling");
    SetBoolForKey(WMGetButtonSelected(panel->swi[5]), "DontConfirmKill");
    SetBoolForKey(WMGetButtonSelected(panel->swi[6]), "DisableBlinking");
}


Panel*
InitExpert(WMScreen *scr, WMWidget *parent)
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
