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

    CallbackRec callbacks;
    
    WMWindow *win;
    
    WMFrame *posF;
    WMFrame *posVF;
    WMFrame *posV;

    WMButton *nwB;
    WMButton *neB;
    WMButton *swB;
    WMButton *seB;
    WMButton *verB;
    WMButton *horB;

    WMFrame *animF;
    WMButton *animB[4];

    WMFrame *optF;
    WMButton *arrB;
    WMButton *omnB;
    
    WMFrame *sizeF;
    WMPopUpButton *sizeP;

} _Panel;



#define ICON_FILE	"iconprefs"


static void
showIconLayout(WMWidget *widget, void *data)
{
    _Panel *panel = (_Panel*)data;
    int w, h;
    
    if (WMGetButtonSelected(panel->horB)) {
	w = 32;
	h = 8;
    } else {
	w = 8;
	h = 32;
    }
    WMResizeWidget(panel->posV, w, h);
    
    if (WMGetButtonSelected(panel->nwB)) {
	WMMoveWidget(panel->posV, 2, 2);
    } else if (WMGetButtonSelected(panel->neB)) {
	WMMoveWidget(panel->posV, 95-2-w, 2);
    } else if (WMGetButtonSelected(panel->swB)) {
	WMMoveWidget(panel->posV, 2, 70-2-h);
    } else {
	WMMoveWidget(panel->posV, 95-2-w, 70-2-h);
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
	if (str[1]=='r' || str[1]=='R') {
	    WMPerformButtonClick(panel->neB);
	} else {
	    WMPerformButtonClick(panel->nwB);
	}
    } else {
	if (str[1]=='r' || str[1]=='R') {
	    WMPerformButtonClick(panel->seB);
	} else {
	    WMPerformButtonClick(panel->swB);
	}
    }
    if (str[2]=='v' || str[2]=='V') {
	WMPerformButtonClick(panel->verB);
    } else {
	WMPerformButtonClick(panel->horB);
    }
    
    i = GetIntegerForKey("IconSize");
    i = (i-24)/8;
    
    if (i<0)
	i = 0;
    else if (i>9)
	i = 9;
    WMSetPopUpButtonSelectedItem(panel->sizeP, i);
    
    str = GetStringForKey("IconificationStyle");
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

    panel->nwB = WMCreateButton(panel->posF, WBTOnOff);
    WMResizeWidget(panel->nwB, 24, 24);
    WMMoveWidget(panel->nwB, 15, 25);
    WMSetButtonAction(panel->nwB, showIconLayout, panel);

    panel->neB = WMCreateButton(panel->posF, WBTOnOff);
    WMResizeWidget(panel->neB, 24, 24);
    WMMoveWidget(panel->neB, 115, 25);
    WMSetButtonAction(panel->neB, showIconLayout, panel);
    
    panel->swB = WMCreateButton(panel->posF, WBTOnOff);
    WMResizeWidget(panel->swB, 24, 24);
    WMMoveWidget(panel->swB, 15, 100);
    WMSetButtonAction(panel->swB, showIconLayout, panel);

    panel->seB = WMCreateButton(panel->posF, WBTOnOff);
    WMResizeWidget(panel->seB, 24, 24);
    WMMoveWidget(panel->seB, 115, 100);
    WMSetButtonAction(panel->seB, showIconLayout, panel);

    WMGroupButtons(panel->nwB, panel->neB);
    WMGroupButtons(panel->nwB, panel->seB);
    WMGroupButtons(panel->nwB, panel->swB);

    color = WMCreateRGBColor(WMWidgetScreen(panel->win), 0x5100, 0x5100, 
			     0x7100, True);
    panel->posVF = WMCreateFrame(panel->posF);
    WMResizeWidget(panel->posVF, 95, 70);
    WMMoveWidget(panel->posVF, 30, 38);
    WMSetFrameRelief(panel->posVF, WRSunken);
    WMSetWidgetBackgroundColor(panel->posVF, color);
    WMReleaseColor(color);

    panel->posV = WMCreateFrame(panel->posVF);
    WMSetFrameRelief(panel->posV, WRSimple);

    panel->verB = WMCreateRadioButton(panel->posF);
    WMResizeWidget(panel->verB, 105, 20);
    WMMoveWidget(panel->verB, 150, 45);
    WMSetButtonText(panel->verB, "Vertical");
    WMSetButtonAction(panel->verB, showIconLayout, panel);

    panel->horB = WMCreateRadioButton(panel->posF);
    WMResizeWidget(panel->horB, 105, 20);
    WMMoveWidget(panel->horB, 150, 75);
    WMSetButtonText(panel->horB, "Horizontal");   
    WMSetButtonAction(panel->horB, showIconLayout, panel);
    

    WMGroupButtons(panel->horB, panel->verB);
    
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
    WMResizeWidget(panel->optF, 260, 65);
    WMMoveWidget(panel->optF, 25, 155);
    
    panel->arrB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->arrB, 235, 20);
    WMMoveWidget(panel->arrB, 15, 10);
    WMSetButtonText(panel->arrB, _("Auto-arrange icons"));

    panel->omnB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->omnB, 235, 20);
    WMMoveWidget(panel->omnB, 15, 35);
    WMSetButtonText(panel->omnB, _("Omnipresent miniwindows"));

    WMMapSubwidgets(panel->optF);
    
    /***************** Icon Size ****************/
    panel->sizeF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->sizeF, 205, 70);
    WMMoveWidget(panel->sizeF, 295, 150);
    WMSetFrameTitle(panel->sizeF, _("Icon Size"));


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
    if (WMGetButtonSelected(panel->nwB)) {
	buf[0] = 't';
	buf[1] = 'l';
    } else if (WMGetButtonSelected(panel->neB)) {
	buf[0] = 't';
	buf[1] = 'r';
    } else if (WMGetButtonSelected(panel->swB)) {
	buf[0] = 'b';
	buf[1] = 'l';
    } else {
	buf[0] = 'b';
	buf[1] = 'r';
    }

    if (WMGetButtonSelected(panel->horB)) {
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
    
    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
