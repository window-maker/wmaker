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
    WMBox *box;

    char *sectionName;

    char *description;

    CallbackRec callbacks;
    
    WMWidget *parent;

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

    WMFrame *borderF;
    WMSlider *borderS;
    WMLabel *borderL;
    WMButton *lrB;
    WMButton *tbB;

} _Panel;



#define ICON_FILE	"ergonomic"


static void
borderCallback(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char buffer[64];
    int i;

    i = WMGetSliderValue(panel->borderS);

    if (i == 0)
	sprintf(buffer, _("OFF"));
    else if (i == 1)
	sprintf(buffer, _("1 pixel"));
    else if (i <= 4)
	/* 2-4 */
	sprintf(buffer, _("%i pixels"), i);
    else
	/* >4 */
	sprintf(buffer, _("%i pixels "), i);		/* note space! */
    WMSetLabelText(panel->borderL, buffer);
}


static void
showData(_Panel *panel)
{
    char *str;
    int x;

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

    x = GetIntegerForKey("WorkspaceBorderSize");
    x = x<0 ? 0 : x;
    x = x>5 ? 5 : x;
    WMSetSliderValue(panel->borderS, x);
    borderCallback(NULL, panel);

    str = GetStringForKey("WorkspaceBorder");
    if (!str)
        str = "none";
    if (strcasecmp(str, "LeftRight")==0) {
        WMSetButtonSelected(panel->lrB, True);
    } else if (strcasecmp(str, "TopBottom")==0) {
        WMSetButtonSelected(panel->tbB, True);
    } else if (strcasecmp(str, "AllDirections")==0) {
        WMSetButtonSelected(panel->tbB, True);
        WMSetButtonSelected(panel->lrB, True);
    }

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
    Bool lr, tb;

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

    lr = WMGetButtonSelected(panel->lrB);
    tb = WMGetButtonSelected(panel->tbB);
    if (lr && tb)
        str = "AllDirections";
    else if (lr)
        str = "LeftRight";
    else if (tb)
        str = "TopBottom";
    else
        str = "None";
    SetStringForKey(str, "WorkspaceBorder");
    SetIntegerForKey(WMGetSliderValue(panel->borderS), "WorkspaceBorderSize");

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
    
    panel->box = WMCreateBox(panel->parent);
    WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 0, 0);
    
    
    /***************** Size Display ****************/
    panel->sizeF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->sizeF, 240, 60);
    WMMoveWidget(panel->sizeF, 20, 10);
    WMSetFrameTitle(panel->sizeF, _("Size Display"));

    WMSetBalloonTextForView(_("The position or style of the window size\n"
                              "display that's shown when a window is resized."),
                            WMWidgetView(panel->sizeF));

    panel->sizeP = WMCreatePopUpButton(panel->sizeF);
    WMResizeWidget(panel->sizeP, 200, 20);
    WMMoveWidget(panel->sizeP, 22, 24);
    WMAddPopUpButtonItem(panel->sizeP, _("Corner of screen"));
    WMAddPopUpButtonItem(panel->sizeP, _("Center of screen"));
    WMAddPopUpButtonItem(panel->sizeP, _("Center of resized window"));
    WMAddPopUpButtonItem(panel->sizeP, _("Technical drawing-like"));

    WMMapSubwidgets(panel->sizeF);

    /***************** Position Display ****************/
    panel->posiF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->posiF, 240, 60);
    WMMoveWidget(panel->posiF, 20, 75);
    WMSetFrameTitle(panel->posiF, _("Position Display"));

    WMSetBalloonTextForView(_("The position or style of the window position\n"
                              "display that's shown when a window is moved."),
                            WMWidgetView(panel->posiF));

    panel->posiP = WMCreatePopUpButton(panel->posiF);
    WMResizeWidget(panel->posiP, 200, 20);
    WMMoveWidget(panel->posiP, 22, 24);
    WMAddPopUpButtonItem(panel->posiP, _("Corner of screen"));
    WMAddPopUpButtonItem(panel->posiP, _("Center of screen"));
    WMAddPopUpButtonItem(panel->posiP, _("Center of resized window"));

    WMMapSubwidgets(panel->posiF);

    /***************** Balloon Text ****************/
    panel->ballF = WMCreateFrame(panel->box);
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
    panel->optF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->optF, 235, 75);
    WMMoveWidget(panel->optF, 270, 145);

    panel->raisB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->raisB, 210, 30);
    WMMoveWidget(panel->raisB, 15, 7);
    WMSetButtonText(panel->raisB, _("Raise window when switching\nfocus with keyboard."));

#ifdef XKB_MODELOCK
    panel->modeB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->modeB, 210, 30);
    WMMoveWidget(panel->modeB, 15, 40);
    WMSetButtonText(panel->modeB, _("Enable keyboard language\nswitch button in window titlebars."));
#endif

    WMMapSubwidgets(panel->optF);
    
    /***************** Workspace border ****************/
    panel->borderF = WMCreateFrame(panel->box);
    WMResizeWidget(panel->borderF, 240, 75);
    WMMoveWidget(panel->borderF, 20, 145);
    WMSetFrameTitle(panel->borderF, _("Workspace border"));
    
    panel->borderS = WMCreateSlider(panel->borderF);
    WMResizeWidget(panel->borderS, 80, 15);
    WMMoveWidget(panel->borderS, 20, 20);
    WMSetSliderMinValue(panel->borderS, 0);
    WMSetSliderMaxValue(panel->borderS, 5);
    WMSetSliderAction(panel->borderS, borderCallback, panel);

    panel->borderL = WMCreateLabel(panel->borderF);
    WMResizeWidget(panel->borderL, 100, 15);
    WMMoveWidget(panel->borderL, 105, 20);

    panel->lrB = WMCreateSwitchButton(panel->borderF);
    WMMoveWidget(panel->lrB, 20, 40);
    WMResizeWidget(panel->lrB, 100, 30);
    WMSetButtonText(panel->lrB, _("Left/Right"));

    panel->tbB = WMCreateSwitchButton(panel->borderF);
    WMMoveWidget(panel->tbB, 120, 40);
    WMResizeWidget(panel->tbB, 100, 30);
    WMSetButtonText(panel->tbB, _("Top/Bottom"));


    WMMapSubwidgets(panel->borderF);
    
    WMRealizeWidget(panel->box);
    WMMapSubwidgets(panel->box);

    showData(panel);
}



Panel*
InitPreferences(WMScreen *scr, WMWidget *parent)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Miscellaneous Ergonomic Preferences");
    panel->description = _("Various settings like balloon text, geometry\n"
			   "displays etc.");

    panel->parent = parent;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
