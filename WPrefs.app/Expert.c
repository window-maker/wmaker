/* Expert.c- expert user options
 * 
 *  WPrefs - WindowMaker Preferences Program
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
    WMFrame *frame;
    char *sectionName;   

    CallbackRec callbacks;
    
    WMWindow *win;

    WMButton *swi[4];

} _Panel;



#define ICON_FILE	"expert"


static void
showData(_Panel *panel)
{
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    WMSetButtonSelected(panel->swi[0], WMGetUDBoolForKey(udb, "NoXSetStuff"));
    WMSetButtonSelected(panel->swi[1], GetBoolForKey("SaveSessionOnExit"));
    WMSetButtonSelected(panel->swi[2], GetBoolForKey("UseSaveUnders"));
    WMSetButtonSelected(panel->swi[3], GetBoolForKey("DisableBlinking"));
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    int i;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    for (i=0; i<4; i++) {
	panel->swi[i] = WMCreateSwitchButton(panel->frame);
	WMResizeWidget(panel->swi[i], FRAME_WIDTH-40, 25);
	WMMoveWidget(panel->swi[i], 20, 20+i*25);
    }
    WMSetButtonText(panel->swi[0], _("Do not set non-WindowMaker specific parameters (do not use xset)"));
    WMSetButtonText(panel->swi[1], _("Automatically save session when exiting WindowMaker"));
    WMSetButtonText(panel->swi[2], _("Use SaveUnder in window frames, icons, menus and other objects"));
    WMSetButtonText(panel->swi[3], _("Disable cycling color highlighting of icons."));

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    showData(panel);
}


static void
storeDefaults(_Panel *panel)
{
    WMUserDefaults *udb = WMGetStandardUserDefaults();

    WMSetUDBoolForKey(udb, WMGetButtonSelected(panel->swi[0]), "NoXSetStuff");

    SetBoolForKey(WMGetButtonSelected(panel->swi[1]), "SaveSessionOnExit");
    SetBoolForKey(WMGetButtonSelected(panel->swi[2]), "UseSaveUnders");
    SetBoolForKey(WMGetButtonSelected(panel->swi[3]), "DisableBlinking");
}


Panel*
InitExpert(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Expert User Preferences");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeDefaults;
    
    AddSection(panel, ICON_FILE);

    return panel;
}
