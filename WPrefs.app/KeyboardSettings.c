/* KeyboardSettings.c- keyboard options (equivalent to xset)
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

    WMFrame *delaF;
    WMButton *delaB[4];
    WMLabel *dmsL;
    WMTextField *dmsT;
    
    WMFrame *rateF;
    WMButton *rateB[4];
    WMLabel *rmsL;
    WMTextField *rmsT;
    
    WMTextField *testT;
} _Panel;


#define ICON_FILE	"keyboard"


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    int i;
    WMColor *color;
    WMFont *font;

    color = WMDarkGrayColor(scr);
    font = WMSystemFontOfSize(scr, 10);
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    /**************** Initial Key Repeat ***************/
    panel->delaF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->delaF, 495, 60);
    WMMoveWidget(panel->delaF, 15, 10);
    WMSetFrameTitle(panel->delaF, _("Initial Key Repeat"));
        
    for (i = 0; i < 4; i++) {
	panel->delaB[i] = WMCreateButton(panel->delaF, WBTOnOff);
	WMResizeWidget(panel->delaB[i], 60, 20);
	WMMoveWidget(panel->delaB[i], 70+i*60, 25);
	if (i>0)
	    WMGroupButtons(panel->delaB[0], panel->delaB[i]);
	switch (i) {
	 case 0:
	    WMSetButtonText(panel->delaB[i], "....a");
	    break;
	 case 1:
	    WMSetButtonText(panel->delaB[i], "...a");
	    break;
	 case 2:
	    WMSetButtonText(panel->delaB[i], "..a");
	    break;
	 case 3:
	    WMSetButtonText(panel->delaB[i], ".a");
	    break;
	}
    }
    panel->dmsT = WMCreateTextField(panel->delaF);
    WMResizeWidget(panel->dmsT, 50, 20);
    WMMoveWidget(panel->dmsT, 345, 25);
/*    WMSetTextFieldAlignment(panel->dmsT, WARight);*/

    panel->dmsL = WMCreateLabel(panel->delaF);
    WMResizeWidget(panel->dmsL, 30, 16);
    WMMoveWidget(panel->dmsL, 400, 30);
    WMSetLabelTextColor(panel->dmsL, color);
    WMSetLabelFont(panel->dmsL, font);
    WMSetLabelText(panel->dmsL, "msec");
    
    WMMapSubwidgets(panel->delaF);
    
    /**************** Key Repeat Rate ***************/
    panel->rateF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->rateF, 495, 60);
    WMMoveWidget(panel->rateF, 15, 95);
    WMSetFrameTitle(panel->rateF, _("Key Repeat Rate"));
    
    for (i = 0; i < 4; i++) {
	panel->rateB[i] = WMCreateButton(panel->rateF, WBTOnOff);
	WMResizeWidget(panel->rateB[i], 60, 20);
	WMMoveWidget(panel->rateB[i], 70+i*60, 25);
	if (i>0)
	    WMGroupButtons(panel->rateB[0], panel->rateB[i]);
	switch (i) {
	 case 0:
	    WMSetButtonText(panel->rateB[i], "a....a");
	    break;
	 case 1:
	    WMSetButtonText(panel->rateB[i], "a...a");
	    break;
	 case 2:
	    WMSetButtonText(panel->rateB[i], "a..a");
	    break;
	 case 3:
	    WMSetButtonText(panel->rateB[i], "a.a");
	    break;
	}
    }
    panel->rmsT = WMCreateTextField(panel->rateF);
    WMResizeWidget(panel->rmsT, 50, 20);
    WMMoveWidget(panel->rmsT, 345, 25);
/*    WMSetTextFieldAlignment(panel->rmsT, WARight);*/
	
    panel->rmsL = WMCreateLabel(panel->rateF);
    WMResizeWidget(panel->rmsL, 30, 16);
    WMMoveWidget(panel->rmsL, 400, 30);
    WMSetLabelTextColor(panel->rmsL, color);
    WMSetLabelFont(panel->rmsL, font);
    WMSetLabelText(panel->rmsL, "msec");
    
    WMMapSubwidgets(panel->rateF);

    panel->testT = WMCreateTextField(panel->frame);
    WMResizeWidget(panel->testT, 480, 20);
    WMMoveWidget(panel->testT, 20, 180);
    WMSetTextFieldText(panel->testT, _("Type here to test"));

    WMReleaseColor(color);
    WMReleaseFont(font);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
}



Panel*
InitKeyboardSettings(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Keyboard Preferences");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
        
    AddSection(panel, ICON_FILE);

    return panel;
}
