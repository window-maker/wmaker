/* Preferences.c- misc personal preferences
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
    WMFrame *frame;

    char *sectionName;

    char *description;

    CallbackRec callbacks;
    
    WMWindow *win;

    WMFrame *sizeF;
    WMPopUpButton *sizeP;
    
    WMFrame *posiF;
    WMPopUpButton *posiP;

    WMFrame *ballF;
    WMButton *ballB[4];

    WMFrame *optF;
    WMButton *raisB;
#ifdef XKB_MODELOCK
    WMButton *modeB;
#endif /* XKB_MODELOCK */

} _Panel;



#define ICON_FILE	"ergonomic"


static void
showData(_Panel *panel)
{
    char *str;
    
    str = GetStringForKey("ResizeDisplay");
    if (!str)
	str = "corner";
    if (strcasecmp(str, "corner")==0)
	WMSetPopUpButtonSelectedItem(panel->sizeP, 0);
    else if (strcasecmp(str, "center")==0)
	WMSetPopUpButtonSelectedItem(panel->sizeP, 1);
    else if (strcasecmp(str, "floating")==0)
	WMSetPopUpButtonSelectedItem(panel->sizeP, 2);
    else if (strcasecmp(str, "line")==0)
	WMSetPopUpButtonSelectedItem(panel->sizeP, 3);
    
    str = GetStringForKey("MoveDisplay");
    if (!str)
	str = "corner";
    if (strcasecmp(str, "corner")==0)
	WMSetPopUpButtonSelectedItem(panel->posiP, 0);
    else if (strcasecmp(str, "center")==0)
	WMSetPopUpButtonSelectedItem(panel->posiP, 1);
    else if (strcasecmp(str, "floating")==0)
	WMSetPopUpButtonSelectedItem(panel->posiP, 2);
    

    WMSetButtonSelected(panel->raisB, GetBoolForKey("CirculateRaise"));
#ifdef XKB_MODELOCK
    WMSetButtonSelected(panel->modeB, GetBoolForKey("KbdModeLock"));
#endif /* XKB_MODELOCK */

    WMSetButtonSelected(panel->ballB[0], GetBoolForKey("WindowTitleBalloons"));
    WMSetButtonSelected(panel->ballB[1], GetBoolForKey("MiniwindowTitleBalloons"));
    WMSetButtonSelected(panel->ballB[2], GetBoolForKey("AppIconBalloons"));
    WMSetButtonSelected(panel->ballB[3], GetBoolForKey("HelpBalloons"));
}


static void
storeData(_Panel *panel)
{
    char *str;
    
    switch (WMGetPopUpButtonSelectedItem(panel->sizeP)) {
     case 0:
	str = "corner";
	break;
     case 1:
	str = "center";
	break;
     case 2:
	str = "floating";
	break;
     default:
	str = "line";
	break;
    }
    SetStringForKey(str, "ResizeDisplay");
    
    switch (WMGetPopUpButtonSelectedItem(panel->posiP)) {
     case 0:
	str = "corner";
	break;
     case 1:
	str = "center";
	break;
     default:
	str = "floating";
	break;     
    }
    SetStringForKey(str, "MoveDisplay");

    SetBoolForKey(WMGetButtonSelected(panel->raisB), "CirculateRaise");
#ifdef XKB_MODELOCK
    SetBoolForKey(WMGetButtonSelected(panel->modeB), "KbdModeLock");
#endif /* XKB_MODELOCK */
    SetBoolForKey(WMGetButtonSelected(panel->ballB[0]), "WindowTitleBalloons");
    SetBoolForKey(WMGetButtonSelected(panel->ballB[1]), "MiniwindowTitleBalloons");
    SetBoolForKey(WMGetButtonSelected(panel->ballB[2]), "AppIconBalloons");
    SetBoolForKey(WMGetButtonSelected(panel->ballB[3]), "HelpBalloons");
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    int i;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    
    /***************** Size Display ****************/
    panel->sizeF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->sizeF, 240, 60);
    WMMoveWidget(panel->sizeF, 20, 10);
    WMSetFrameTitle(panel->sizeF, _("Size Display"));
    
    panel->sizeP = WMCreatePopUpButton(panel->sizeF);
    WMResizeWidget(panel->sizeP, 180, 20);
    WMMoveWidget(panel->sizeP, 32, 24);
    WMAddPopUpButtonItem(panel->sizeP, _("Corner of screen"));
    WMAddPopUpButtonItem(panel->sizeP, _("Center of screen"));
    WMAddPopUpButtonItem(panel->sizeP, _("Center of resized window"));
    WMAddPopUpButtonItem(panel->sizeP, _("Technical drawing-like"));

    WMMapSubwidgets(panel->sizeF);

    /***************** Position Display ****************/
    panel->posiF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->posiF, 240, 60);
    WMMoveWidget(panel->posiF, 20, 75);
    WMSetFrameTitle(panel->posiF, _("Position Display"));

    panel->posiP = WMCreatePopUpButton(panel->posiF);
    WMResizeWidget(panel->posiP, 180, 20);
    WMMoveWidget(panel->posiP, 32, 24);
    WMAddPopUpButtonItem(panel->posiP, _("Corner of screen"));
    WMAddPopUpButtonItem(panel->posiP, _("Center of screen"));
    WMAddPopUpButtonItem(panel->posiP, _("Center of resized window"));

    WMMapSubwidgets(panel->posiF);

    /***************** Balloon Text ****************/
    panel->ballF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->ballF, 235, 125);
    WMMoveWidget(panel->ballF, 270, 10);
    WMSetFrameTitle(panel->ballF, _("Show balloon text for..."));
    
    for (i=0; i<4; i++) {
	panel->ballB[i] = WMCreateSwitchButton(panel->ballF);
	WMResizeWidget(panel->ballB[i], 205, 20);
	WMMoveWidget(panel->ballB[i], 15, 20+i*25);
    }
    WMSetButtonText(panel->ballB[0], _("incomplete window titles"));
    WMSetButtonText(panel->ballB[1], _("miniwindow titles"));
    WMSetButtonText(panel->ballB[2], _("application/dock icons"));
    WMSetButtonText(panel->ballB[3], _("internal help"));

    WMMapSubwidgets(panel->ballF);

    /***************** Options ****************/
    panel->optF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->optF, 485, 75);
    WMMoveWidget(panel->optF, 20, 145);

    panel->raisB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->raisB, 440, 20);
    WMMoveWidget(panel->raisB, 20, 15);
    WMSetButtonText(panel->raisB, _("Raise window when switching focus with keyboard (CirculateRaise)."));

#ifdef XKB_MODELOCK
    panel->modeB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->modeB, 440, 20);
    WMMoveWidget(panel->modeB, 20, 40);
    WMSetButtonText(panel->modeB, _("Keep keyboard language status for each window."));
#endif

    WMMapSubwidgets(panel->optF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    showData(panel);
}



Panel*
InitPreferences(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Miscellaneous Ergonomic Preferences");
    panel->description = _("Various settings like balloon text, geometry\n"
			   "displays etc.");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
