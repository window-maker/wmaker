/* Icons.c- icon preferences
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
    
    WMFrame *posF;
    WMFrame *posVF;
    WMFrame *posV;

    WMButton *posB[8];

    WMFrame *animF;
    WMButton *animB[4];

    WMFrame *optF;
    WMButton *arrB;
    WMButton *omnB;
    
    WMFrame *sizeF;
    WMPopUpButton *sizeP;

    int iconPos;
} _Panel;



#define ICON_FILE	"iconprefs"


static void
showIconLayout(WMWidget *widget, void *data)
{
    _Panel *panel = (_Panel*)data;
    int w, h;
    int i;
    
    for (i=0; i<8; i++) {
	if (panel->posB[i] == widget) {
	    panel->iconPos = i;
	    break;
	}
    }

    if (panel->iconPos & 1) {
	w = 32;
	h = 8;
    } else {
	w = 8;
	h = 32;
    }
    WMResizeWidget(panel->posV, w, h);
    
    switch (panel->iconPos & ~1) {
     case 0:
	WMMoveWidget(panel->posV, 2, 2);
	break;
     case 2:
	WMMoveWidget(panel->posV, 95-2-w, 2);
	break;
     case 4:
	WMMoveWidget(panel->posV, 2, 70-2-h);
	break;
     default:
	WMMoveWidget(panel->posV, 95-2-w, 70-2-h);
	break;
    }
}


#define MIN(a,b)	((a) < (b) ? (a) : (b))

static void
showData(_Panel *panel)
{
    int i;
    char *str;
    char *def = "blh";
        
    WMSetButtonSelected(panel->arrB, GetBoolForKey("AutoArrangeIcons"));
    
    WMSetButtonSelected(panel->omnB, GetBoolForKey("StickyIcons"));
    
    str = GetStringForKey("IconPosition");
    if (!str)
	str = def;
    if (strlen(str)!=3) {
	wwarning("bad value %s for option IconPosition. Using default blh",
		 str);
	str = def;
    }

    if (str[0]=='t' || str[0]=='T') {
	i = 0;
    } else {
	i = 4;
    }
    if (str[1]=='r' || str[1]=='R') {
	i += 2;
    }
    if (str[2]=='v' || str[2]=='V') {
	i += 0;
    } else {
	i += 1;
    }
    panel->iconPos = i;
    WMPerformButtonClick(panel->posB[i]);
    
    i = GetIntegerForKey("IconSize");
    i = (i-24)/8;
    
    if (i<0)
	i = 0;
    else if (i>9)
	i = 9;
    WMSetPopUpButtonSelectedItem(panel->sizeP, i);
    
    str = GetStringForKey("IconificationStyle");
    if (!str)
	str = "zoom";
    if (strcasecmp(str, "none")==0)
	WMPerformButtonClick(panel->animB[3]);
    else if (strcasecmp(str, "twist")==0)
	WMPerformButtonClick(panel->animB[1]);
    else if (strcasecmp(str, "flip")==0)
	WMPerformButtonClick(panel->animB[2]);
    else {
	WMPerformButtonClick(panel->animB[0]);
    }
}




static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMColor *color;
    int i;
    char buf[16];

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    /***************** Positioning of Icons *****************/
    panel->posF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->posF, 260, 135);
    WMMoveWidget(panel->posF, 25, 10);
    WMSetFrameTitle(panel->posF, _("Icon Positioning"));

    for (i=0; i<8; i++) {
	panel->posB[i] = WMCreateButton(panel->posF, WBTOnOff);
	WMSetButtonAction(panel->posB[i], showIconLayout, panel);

	if (i>0)
	    WMGroupButtons(panel->posB[0], panel->posB[i]);
    }
    WMMoveWidget(panel->posB[1], 70, 23);
    WMResizeWidget(panel->posB[1], 47, 15);
    WMMoveWidget(panel->posB[3], 70+47, 23);
    WMResizeWidget(panel->posB[3], 47, 15);

    WMMoveWidget(panel->posB[0], 55, 38);
    WMResizeWidget(panel->posB[0], 15, 35);
    WMMoveWidget(panel->posB[4], 55, 38+35);
    WMResizeWidget(panel->posB[4], 15, 35);

    WMMoveWidget(panel->posB[5], 70, 38+70);
    WMResizeWidget(panel->posB[5], 47, 15);
    WMMoveWidget(panel->posB[7], 70+47, 38+70);
    WMResizeWidget(panel->posB[7], 47, 15);

    WMMoveWidget(panel->posB[2], 70+95, 38);
    WMResizeWidget(panel->posB[2], 15, 35);
    WMMoveWidget(panel->posB[6], 70+95, 38+35);
    WMResizeWidget(panel->posB[6], 15, 35);

    color = WMCreateRGBColor(WMWidgetScreen(panel->win), 0x5100, 0x5100, 
			     0x7100, True);
    panel->posVF = WMCreateFrame(panel->posF);
    WMResizeWidget(panel->posVF, 95, 70);
    WMMoveWidget(panel->posVF, 70, 38);
    WMSetFrameRelief(panel->posVF, WRSunken);
    WMSetWidgetBackgroundColor(panel->posVF, color);
    WMReleaseColor(color);

    panel->posV = WMCreateFrame(panel->posVF);
    WMSetFrameRelief(panel->posV, WRSimple);
    
    WMMapSubwidgets(panel->posF);
    
    /***************** Animation ****************/
    panel->animF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->animF, 205, 135);
    WMMoveWidget(panel->animF, 295, 10);
    WMSetFrameTitle(panel->animF, _("Iconification Animation"));

    for (i=0; i<4; i++) {
	panel->animB[i] = WMCreateRadioButton(panel->animF);
	WMResizeWidget(panel->animB[i], 170, 20);
	WMMoveWidget(panel->animB[i], 20, 24+i*25);
    }
    WMGroupButtons(panel->animB[0], panel->animB[1]);
    WMGroupButtons(panel->animB[0], panel->animB[2]);
    WMGroupButtons(panel->animB[0], panel->animB[3]);

    WMSetButtonText(panel->animB[0], _("Shrinking/Zooming"));
    WMSetButtonText(panel->animB[1], _("Spinning/Twisting"));
    WMSetButtonText(panel->animB[2], _("3D-flipping"));
    WMSetButtonText(panel->animB[3], _("None"));

    WMMapSubwidgets(panel->animF);

    /***************** Options ****************/
    panel->optF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->optF, 260, 70);
    WMMoveWidget(panel->optF, 25, 150);
/*    WMSetFrameTitle(panel->optF, _("Icon Display"));*/
    
    panel->arrB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->arrB, 235, 20);
    WMMoveWidget(panel->arrB, 15, 15);
    WMSetButtonText(panel->arrB, _("Auto-arrange icons"));

    WMSetBalloonTextForView(_("Keep icons and miniwindows arranged all the time."),
			    WMWidgetView(panel->arrB));

    panel->omnB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->omnB, 235, 20);
    WMMoveWidget(panel->omnB, 15, 40);
    WMSetButtonText(panel->omnB, _("Omnipresent miniwindows"));

    WMSetBalloonTextForView(_("Make miniwindows be present in all workspaces."),
			    WMWidgetView(panel->omnB));

    WMMapSubwidgets(panel->optF);
    
    /***************** Icon Size ****************/
    panel->sizeF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->sizeF, 205, 70);
    WMMoveWidget(panel->sizeF, 295, 150);
    WMSetFrameTitle(panel->sizeF, _("Icon Size"));

    WMSetBalloonTextForView(_("The size of the dock/application icon and miniwindows"),
			    WMWidgetView(panel->sizeF));

    panel->sizeP = WMCreatePopUpButton(panel->sizeF);
    WMResizeWidget(panel->sizeP, 156, 20);
    WMMoveWidget(panel->sizeP, 25, 30);
    for (i=24; i<=96; i+=8) {
	sprintf(buf, "%ix%i", i, i);
	WMAddPopUpButtonItem(panel->sizeP, buf);
    }
    
    WMMapSubwidgets(panel->sizeF);

    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    showData(panel);
}


static void
storeData(_Panel *panel)
{
    char buf[8];

    SetBoolForKey(WMGetButtonSelected(panel->arrB), "AutoArrangeIcons");
    SetBoolForKey(WMGetButtonSelected(panel->omnB), "StickyIcons");

    SetIntegerForKey(WMGetPopUpButtonSelectedItem(panel->sizeP)*8+24,
		     "IconSize");

    buf[3] = 0;

    if (panel->iconPos < 4) {
	buf[0] = 't';
    } else {
	buf[0] = 'b';
    }
    if (panel->iconPos & 2) {
	buf[1] = 'r';
    } else {
	buf[1] = 'l';
    }
    if (panel->iconPos & 1) {
	buf[2] = 'h';
    } else {
	buf[2] = 'v';
    }
    SetStringForKey(buf, "IconPosition");

    if (WMGetButtonSelected(panel->animB[0]))
	SetStringForKey("zoom", "IconificationStyle");
    else if (WMGetButtonSelected(panel->animB[1]))
	SetStringForKey("twist", "IconificationStyle");
    else if (WMGetButtonSelected(panel->animB[2]))
	SetStringForKey("flip", "IconificationStyle");
    else 
	SetStringForKey("none", "IconificationStyle");
}



Panel*
InitIcons(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Icon Preferences");

    panel->description = _("Icon/Miniwindow handling options. Icon positioning\n"
			   "area, sizes of icons, miniaturization animation style.");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
