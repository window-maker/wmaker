/* Text.c- text/font settings
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
    
    WMPopUpButton *secP;
    WMButton *setB;
    
    WMTextField *nameT;
    
    WMLabel *sampleL;
    
    WMFrame *alignF;
    WMButton *leftB;
    WMButton *centerB;
    WMButton *rightB;


    /**/
    WMFont *windowF;
    char *windowFont;
    WMFont *menuF;
    char *menuFont;
    WMFont *itemF;
    char *itemFont;
    WMFont *clipF;
    char *clipFont;
    WMFont *iconF;
    char *iconFont;
    WMFont *geoF;
    char *geoFont;
} _Panel;



#define ICON_FILE	"fonts"




static void
changePage(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int sect;
    
    sect = WMGetPopUpButtonSelectedItem(w);

    if (sect == 0) {
	WMMapWidget(panel->alignF);
    } else {
	WMUnmapWidget(panel->alignF);
    }

    switch (sect) {
     case 0:
	WMSetTextFieldText(panel->nameT, panel->windowFont);
	WMSetLabelFont(panel->sampleL, panel->windowF);
	break;
     case 1:
	WMSetTextFieldText(panel->nameT, panel->menuFont);
	WMSetLabelFont(panel->sampleL, panel->menuF);
	break;
     case 2:
	WMSetTextFieldText(panel->nameT, panel->itemFont);
	WMSetLabelFont(panel->sampleL, panel->itemF);
	break;
     case 3:
	WMSetTextFieldText(panel->nameT, panel->iconFont);
	WMSetLabelFont(panel->sampleL, panel->iconF);
	break;
     case 4:
	WMSetTextFieldText(panel->nameT, panel->clipFont);
	WMSetLabelFont(panel->sampleL, panel->clipF);
	break;
     case 5:
	WMSetTextFieldText(panel->nameT, panel->geoFont);
	WMSetLabelFont(panel->sampleL, panel->geoF);
	break;
    }
}


static void
showData(_Panel *panel)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    char *str;
    
    str = GetStringForKey("WindowTitleFont");
    if (!str)
	str = "-*-helvetica-bold-r-normal-*-12-*";
    panel->windowF = WMCreateFont(scr, str);
    panel->windowFont = wstrdup(str);
    
    str = GetStringForKey("MenuTitleFont");
    if (!str)
	str = "-*-helvetica-bold-r-normal-*-12-*";
    panel->menuF = WMCreateFont(scr, str);
    panel->menuFont = wstrdup(str);

    str = GetStringForKey("MenuTextFont");
    if (!str)
	str = "-*-helvetica-medium-r-normal-*-12-*";
    panel->itemF = WMCreateFont(scr, str);
    panel->itemFont = wstrdup(str);

    str = GetStringForKey("IconTitleFont");
    if (!str)
	str = "-*-helvetica-medium-r-normal-*-8-*";
    panel->iconF = WMCreateFont(scr, str);
    panel->iconFont = wstrdup(str);

    str = GetStringForKey("ClipTitleFont");
    if (!str)
	str = "-*-helvetica-medium-r-normal-*-10-*";
    panel->clipF = WMCreateFont(scr, str);
    panel->clipFont = wstrdup(str);

    str = GetStringForKey("DisplayFont");
    if (!str)
	str = "-*-helvetica-medium-r-normal-*-12-*";
    panel->geoF = WMCreateFont(scr, str);
    panel->geoFont = wstrdup(str);
    
    str = GetStringForKey("TitleJustify");
    if (strcasecmp(str,"left")==0)
	WMPerformButtonClick(panel->leftB);
    else if (strcasecmp(str,"center")==0)
	WMPerformButtonClick(panel->centerB);
    else if (strcasecmp(str,"right")==0)
	WMPerformButtonClick(panel->rightB);
    
    changePage(panel->secP, panel);
}


static void
editedName(void *data, WMNotification *notification)
{
    _Panel *panel = (_Panel*)data;
    
    if ((int)WMGetNotificationClientData(notification)==WMReturnTextMovement) {
	char *name;
	WMFont *font;
	char buffer[256];
	
	name = WMGetTextFieldText(panel->nameT);
	font = WMCreateFont(WMWidgetScreen(panel->win), name);
	if (!font) {
	    sprintf(buffer, _("Invalid font %s."), name);
	    WMRunAlertPanel(WMWidgetScreen(panel->win), panel->win,
			    _("Error"), buffer, _("OK"), NULL, NULL);
	    free(name);
	} else {
	    int sect;
    
	    sect = WMGetPopUpButtonSelectedItem(panel->secP);

	    switch (sect) {
	     case 0:
		if (panel->windowFont)
		    free(panel->windowFont);
		panel->windowFont = name;
		if (panel->windowF)
		    WMReleaseFont(panel->windowF);
		panel->windowF = font;
		break;
	     case 1:
		if (panel->menuFont)
		    free(panel->menuFont);
		panel->menuFont = name;
		if (panel->menuF)
		    WMReleaseFont(panel->menuF);
		panel->menuF = font;
		break;
	     case 2:
		if (panel->itemFont)
		    free(panel->itemFont);
		panel->itemFont = name;
		if (panel->itemF)
		    WMReleaseFont(panel->itemF);
		panel->itemF = font;
		break;
	     case 3:
		if (panel->iconFont)
		    free(panel->iconFont);
		panel->iconFont = name;
		if (panel->iconF)
		    WMReleaseFont(panel->iconF);
		panel->iconF = font;
		break;
	     case 4:
		if (panel->clipFont)
		    free(panel->clipFont);
		panel->clipFont = name;
		if (panel->clipF)
		    WMReleaseFont(panel->clipF);
		panel->clipF = font;
		break;
	     case 5:
		if (panel->geoFont)
		    free(panel->geoFont);
		panel->geoFont = name;
		if (panel->geoF)
		    WMReleaseFont(panel->geoF);
		panel->geoF = font;
		break;
	    }
	    changePage(panel->secP, panel);
	}
    }	
}


static void
createPanel(Panel *p)
{    
    _Panel *panel = (_Panel*)p;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    panel->setB = WMCreateCommandButton(panel->frame);
    WMResizeWidget(panel->setB, 145, 20);
    WMMoveWidget(panel->setB, 50, 25);
    WMSetButtonText(panel->setB, _("Set Font..."));
    
    panel->secP = WMCreatePopUpButton(panel->frame);
    WMResizeWidget(panel->secP, 260, 20);
    WMMoveWidget(panel->secP, 205, 25);
    WMSetPopUpButtonAction(panel->secP, changePage, panel);
    WMAddPopUpButtonItem(panel->secP, _("Window Title Font"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Title Font"));
    WMAddPopUpButtonItem(panel->secP, _("Menu Item Font"));
    WMAddPopUpButtonItem(panel->secP, _("Icon Title Font"));
    WMAddPopUpButtonItem(panel->secP, _("Clip Title Font"));
    WMAddPopUpButtonItem(panel->secP, _("Geometry Display Font"));
    WMSetPopUpButtonSelectedItem(panel->secP, 0);
    
    panel->nameT = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->nameT, 285, 24);
    WMMoveWidget(panel->nameT, 50, 80);
    WMAddNotificationObserver(editedName, panel, 
			      WMTextDidEndEditingNotification, panel->nameT);
    
    panel->sampleL = WMCreateLabel(panel->frame);
    WMResizeWidget(panel->sampleL, 285, 85);
    WMMoveWidget(panel->sampleL, 50, 135);
    WMSetLabelRelief(panel->sampleL, WRSunken);
    WMSetLabelText(panel->sampleL, _("Sample Text\nabcdefghijklmnopqrstuvxywz\nABCDEFGHIJKLMNOPQRSTUVXYWZ\n0123456789"));
    
    panel->alignF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->alignF, 120, 160);
    WMMoveWidget(panel->alignF, 345, 60);
    WMSetFrameTitle(panel->alignF, _("Alignment"));
    
    panel->leftB = WMCreateButton(panel->alignF, WBTOnOff);
    WMResizeWidget(panel->leftB, 100, 24);
    WMMoveWidget(panel->leftB, 10, 25);
    WMSetButtonText(panel->leftB, _("Left"));
    WMSetButtonTextAlignment(panel->leftB, WALeft);
    
    panel->centerB = WMCreateButton(panel->alignF, WBTOnOff);
    WMResizeWidget(panel->centerB, 100, 24);
    WMMoveWidget(panel->centerB, 10, 70);
    WMSetButtonText(panel->centerB, _("Center"));
    WMSetButtonTextAlignment(panel->centerB, WACenter);
    WMGroupButtons(panel->leftB, panel->centerB);
    
    panel->rightB = WMCreateButton(panel->alignF, WBTOnOff);
    WMResizeWidget(panel->rightB, 100, 24);
    WMMoveWidget(panel->rightB, 10, 115);
    WMSetButtonText(panel->rightB, _("Right"));
    WMSetButtonTextAlignment(panel->rightB, WARight);
    WMGroupButtons(panel->leftB, panel->rightB);
    
    WMMapSubwidgets(panel->alignF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    showData(panel);
}



Panel*
InitText(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Text Preferences");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    
    AddSection(panel, ICON_FILE);

    return panel;
}

