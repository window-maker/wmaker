/* WindowHandling.c- options for handling windows
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
    
    WMFrame *placF;
    WMPopUpButton *placP;
    WMLabel *porigL;
    WMLabel *porigvL;
    WMFrame *porigF;
    WMLabel *porigW;
    
    WMSlider *vsli;
    WMSlider *hsli;

    WMFrame *resF;
    WMSlider *resS;
    WMLabel *resL;

    WMFrame *maxiF;
    WMButton *miconB;
    WMButton *mdockB;
    
    WMFrame *opaqF;
    WMButton *opaqB;

    WMFrame *tranF;
    WMButton *tranB;
} _Panel;


#define ICON_FILE "whandling"

#define OPAQUE_MOVE_PIXMAP "opaque"

#define NON_OPAQUE_MOVE_PIXMAP "nonopaque"


#define THUMB_SIZE	16


static char *placements[] = {
    "auto",
	"random",
	"manual",
	"cascade"
};


static void
sliderCallback(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int x, y, rx, ry;
    char buffer[64];
    int swidth = WMGetSliderMaxValue(panel->hsli);
    int sheight = WMGetSliderMaxValue(panel->vsli);

    x = WMGetSliderValue(panel->hsli);
    y = WMGetSliderValue(panel->vsli);
    
    rx = x*(WMWidgetWidth(panel->porigF)-3)/swidth+2;
    ry = y*(WMWidgetHeight(panel->porigF)-3)/sheight+2;
    WMMoveWidget(panel->porigW, rx, ry);
    
    sprintf(buffer, "(%i,%i)", x, y);
    WMSetLabelText(panel->porigvL, buffer);
}



static void
resistanceCallback(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char buffer[64];
    int i;

    i = WMGetSliderValue(panel->resS);

    if (i == 0)
	WMSetLabelText(panel->resL, "OFF");
    else {
	sprintf(buffer, "%i", i);
	WMSetLabelText(panel->resL, buffer);
    }
}


static int
getPlacement(char *str) 
{
    if (!str)
	return 0;

    if (strcasecmp(str, "auto")==0 || strcasecmp(str, "smart")==0)
	return 0;
    else if (strcasecmp(str, "random")==0)
	return 1;
    else if (strcasecmp(str, "manual")==0)
	return 2;
    else if (strcasecmp(str, "cascade")==0)
	return 3;
    else
	wwarning(_("bad option value %s in WindowPlacement. Using default value"),
		 str);
    return 0;
}


static void
showData(_Panel *panel)
{
    char *str;
    proplist_t arr;
    int x, y;

    str = GetStringForKey("WindowPlacement");

    WMSetPopUpButtonSelectedItem(panel->placP, getPlacement(str));

    arr = GetObjectForKey("WindowPlaceOrigin");

    x = 0; 
    y = 0;
    if (arr && (!PLIsArray(arr) || PLGetNumberOfElements(arr)!=2)) {
	wwarning(_("invalid data in option WindowPlaceOrigin. Using default (0,0)"));
    } else {
	if (arr) {
	    x = atoi(PLGetString(PLGetArrayElement(arr, 0)));
	    y = atoi(PLGetString(PLGetArrayElement(arr, 1)));
	}
    }

    WMSetSliderValue(panel->hsli, x);
    WMSetSliderValue(panel->vsli, y);

    sliderCallback(NULL, panel);

    x = GetIntegerForKey("EdgeResistance");
    WMSetSliderValue(panel->resS, x);
    resistanceCallback(NULL, panel);

    WMSetButtonSelected(panel->tranB, GetBoolForKey("OpenTransientOnOwnerWorkspace"));
    
    WMSetButtonSelected(panel->opaqB, GetBoolForKey("OpaqueMove"));
    
    WMSetButtonSelected(panel->miconB, GetBoolForKey("NoWindowOverIcons"));

    WMSetButtonSelected(panel->mdockB, GetBoolForKey("NoWindowOverDock"));
}


static void
storeData(_Panel *panel)
{
    proplist_t arr;
    char x[16], y[16];
    
    SetBoolForKey(WMGetButtonSelected(panel->miconB), "NoWindowOverIcons");
    SetBoolForKey(WMGetButtonSelected(panel->mdockB), "NoWindowOverDock");
    SetBoolForKey(WMGetButtonSelected(panel->opaqB), "OpaqueMove");
    SetBoolForKey(WMGetButtonSelected(panel->tranB), "OpenTransientOnOwnerWorkspace");
    SetStringForKey(placements[WMGetPopUpButtonSelectedItem(panel->placP)],
		    "WindowPlacement");
    sprintf(x, "%i", WMGetSliderValue(panel->hsli));
    sprintf(y, "%i", WMGetSliderValue(panel->vsli));
    arr = PLMakeArrayFromElements(PLMakeString(x), PLMakeString(y), NULL);
    SetObjectForKey(arr, "WindowPlaceOrigin");
    SetIntegerForKey(WMGetSliderValue(panel->resS), "EdgeResistance");
    PLRelease(arr);
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMColor *color;
    WMPixmap *pixmap;
    int width, height;
    int swidth, sheight;
    char *path;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    /************** Window Placement ***************/
    panel->placF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->placF, 270, 110);
    WMMoveWidget(panel->placF, 20, 10);
    WMSetFrameTitle(panel->placF, _("Window Placement"));
    WMSetBalloonTextForView(_("How to place windows when they are first put\n"
			       "on screen."), WMWidgetView(panel->placF));

    panel->placP = WMCreatePopUpButton(panel->placF);
    WMResizeWidget(panel->placP, 105, 20);
    WMMoveWidget(panel->placP, 15, 20);
    WMAddPopUpButtonItem(panel->placP, _("Automatic"));
    WMAddPopUpButtonItem(panel->placP, _("Random"));
    WMAddPopUpButtonItem(panel->placP, _("Manual"));
    WMAddPopUpButtonItem(panel->placP, _("Cascade"));
    
    panel->porigL = WMCreateLabel(panel->placF);
    WMResizeWidget(panel->porigL, 120, 32);
    WMMoveWidget(panel->porigL, 5, 45);
    WMSetLabelTextAlignment(panel->porigL, WACenter);
    WMSetLabelText(panel->porigL, _("Placement Origin"));
    
    panel->porigvL = WMCreateLabel(panel->placF);
    WMResizeWidget(panel->porigvL, 80, 20);
    WMMoveWidget(panel->porigvL, 30, 75);
    WMSetLabelTextAlignment(panel->porigvL, WACenter);    

    color = WMCreateRGBColor(scr, 0x5100, 0x5100, 0x7100, True);
    panel->porigF = WMCreateFrame(panel->placF);
    WMSetWidgetBackgroundColor(panel->porigF, color);
    WMReleaseColor(color);
    WMSetFrameRelief(panel->porigF, WRSunken);
    
    swidth = WidthOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));
    sheight = HeightOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));
    
    if (sheight > swidth) {
	height = 70;
	width = 70*swidth/sheight;
	if (width > 115)
	    width = 115;
	height = 115*sheight/swidth;
    } else {
	width = 115;
	height = 115*sheight/swidth;
	if (height > 70)
	    height = 70;
	width = 70*swidth/sheight;
    }
    WMResizeWidget(panel->porigF, width, height);
    WMMoveWidget(panel->porigF, 130+(115-width)/2, 20+(70-height)/2);

    panel->porigW = WMCreateLabel(panel->porigF);
    WMResizeWidget(panel->porigW, THUMB_SIZE, THUMB_SIZE);
    WMMoveWidget(panel->porigW, 2, 2);
    WMSetLabelRelief(panel->porigW, WRRaised);

    
    panel->hsli = WMCreateSlider(panel->placF);
    WMResizeWidget(panel->hsli, width, 12);
    WMMoveWidget(panel->hsli, 130+(115-width)/2, 20+(70-height)/2+height+2);
    WMSetSliderAction(panel->hsli, sliderCallback, panel);
    WMSetSliderMinValue(panel->hsli, 0);
    WMSetSliderMaxValue(panel->hsli, swidth);

    panel->vsli = WMCreateSlider(panel->placF);
    WMResizeWidget(panel->vsli, 12, height);
    WMMoveWidget(panel->vsli, 130+(115-width)/2+width+2, 20+(70-height)/2);
    WMSetSliderAction(panel->vsli, sliderCallback, panel);
    WMSetSliderMinValue(panel->vsli, 0);
    WMSetSliderMaxValue(panel->vsli, sheight);

    WMMapSubwidgets(panel->porigF);

    WMMapSubwidgets(panel->placF);

    /************** Opaque Move ***************/
    panel->opaqF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->opaqF, 205, 110);
    WMMoveWidget(panel->opaqF, 300, 10);
    WMSetFrameTitle(panel->opaqF, _("Opaque Move"));
    WMSetBalloonTextForView(_("Whether the window contents should be moved\n"
			       "when dragging windows aroung or if only a\n"
			       "frame should be displayed.\n"),
			     WMWidgetView(panel->opaqF));

    panel->opaqB = WMCreateButton(panel->opaqF, WBTToggle);
    WMResizeWidget(panel->opaqB, 64, 64);
    WMMoveWidget(panel->opaqB, 70, 25);
    WMSetButtonImagePosition(panel->opaqB, WIPImageOnly);

    path = LocateImage(NON_OPAQUE_MOVE_PIXMAP);
    if (path) {
	pixmap = WMCreatePixmapFromFile(scr, path);
	if (pixmap) {
	    WMSetButtonImage(panel->opaqB, pixmap);
	    WMReleasePixmap(pixmap);
	} else {
	    wwarning(_("could not load icon %s"), path);
	}
	free(path);
    }
    
    path = LocateImage(OPAQUE_MOVE_PIXMAP);
    if (path) {
	pixmap = WMCreatePixmapFromFile(scr, path);
	if (pixmap) {
	    WMSetButtonAltImage(panel->opaqB, pixmap);
	    WMReleasePixmap(pixmap);
	} else {
	    wwarning(_("could not load icon %s"), path);
	}
	free(path);
    }
    WMMapSubwidgets(panel->opaqF);
    
    /**************** Account for Icon/Dock ***************/
    panel->maxiF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->maxiF, 205, 95);
    WMMoveWidget(panel->maxiF, 300, 125);
    WMSetFrameTitle(panel->maxiF, _("When maximizing..."));
    
    panel->miconB = WMCreateSwitchButton(panel->maxiF);
    WMResizeWidget(panel->miconB, 185, 30);
    WMMoveWidget(panel->miconB, 10, 18);
    WMSetButtonText(panel->miconB, _("...do not cover icons"));

    panel->mdockB = WMCreateSwitchButton(panel->maxiF);
    WMResizeWidget(panel->mdockB, 185, 30);
    WMMoveWidget(panel->mdockB, 10, 53);

    WMSetButtonText(panel->mdockB, _("...do not cover dock"));

    WMMapSubwidgets(panel->maxiF);

    /**************** Edge Resistance  ****************/

    panel->resF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->resF, 270, 45);
    WMMoveWidget(panel->resF, 20, 125);
    WMSetFrameTitle(panel->resF, _("Edge Resistance"));

    WMSetBalloonTextForView(_("Edge resistance will make windows `resist'\n"
			       "being moved further for the defined threshold\n"
			       "when moved against other windows or the edges\n"
			       "of the screen."), WMWidgetView(panel->resF));

    panel->resS = WMCreateSlider(panel->resF);
    WMResizeWidget(panel->resS, 200, 15);
    WMMoveWidget(panel->resS, 20, 20);
    WMSetSliderMinValue(panel->resS, 0);
    WMSetSliderMaxValue(panel->resS, 200);
    WMSetSliderAction(panel->resS, resistanceCallback, panel);

    panel->resL = WMCreateLabel(panel->resF);
    WMResizeWidget(panel->resL, 30, 15);
    WMMoveWidget(panel->resL, 230, 20);

    WMMapSubwidgets(panel->resF);

    /**************** Transients on Parent Workspace ****************/
    
    panel->tranF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->tranF, 270, 40);
    WMMoveWidget(panel->tranF, 20, 180);
    
    panel->tranB = WMCreateSwitchButton(panel->tranF);
    WMMoveWidget(panel->tranB, 10, 5);
    WMResizeWidget(panel->tranB, 250, 30);
    WMSetButtonText(panel->tranB, _("Open dialogs in same workspace as their owners"));
    
    WMMapSubwidgets(panel->tranF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
 
    /* show the config data */
    showData(panel);
}


static void
undo(_Panel *panel)
{
    showData(panel);
}


Panel*
InitWindowHandling(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Window Handling Preferences");

    panel->description = _("Window handling options. Initial placement style\n"
			   "edge resistance, opaque move etc.");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    panel->callbacks.undoChanges = undo;
        
    AddSection(panel, ICON_FILE);
    
    return panel;
}
