/* imagebrowser.c- image browser widget
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 2000 Alfredo K. Kojima
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

#define FOR_WPREFS

#ifdef FOR_WPREFS
# include "WPrefs.h" /* only for _() */
#else
# define _(a) a
#endif

#include <WINGs/WINGs.h>
#include <WINGs/WINGsP.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "imagebrowser.h"



struct _ImageBrowser {
    WMWindow *win;

    WMPopUpButton *pathP;
    
    WMScrollView *sview;
    WMFrame *frame;

    
    WMWidget *auxWidget;
    
    
    WMButton *viewBtn;
    WMButton *okBtn;
    WMButton *cancelBtn;

    WMSize maxPreviewSize;
    
    ImageBrowserDelegate *delegate;
    
    WMArray *previews;
};





#define DEFAULT_WIDTH 300
#define DEFAULT_HEIGHT 200


ImageBrowser*
CreateImageBrowser(WMScreen *scr, char *title, char **paths, int pathN,
		   WMSize *maxSize, WMWidget *auxWidget)
{
    ImageBrowser *br;
    int i;
    int h;
    
    br = wmalloc(sizeof(ImageBrowser));
        
    br->win = WMCreateWindow(scr, "imageBrowser");
    WMResizeWidget(br->win, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    br->pathP = WMCreatePopUpButton(br->win);
    WMMoveWidget(br->pathP, (DEFAULT_WIDTH - 80)/2, 10);
    WMResizeWidget(br->pathP, DEFAULT_WIDTH - 80, 20);
    
    for (i = 0; i < pathN; i++) {
	WMAddPopUpButtonItem(br->pathP, paths[i]);
    }

    
    br->viewBtn = WMCreateCommandButton(br->win);
    WMSetButtonText(br->viewBtn, _("View"));
    WMResizeWidget(br->viewBtn, 80, 24);
    WMMoveWidget(br->viewBtn, 10, DEFAULT_HEIGHT - 29);

    br->cancelBtn = WMCreateCommandButton(br->win);
    WMSetButtonText(br->cancelBtn, _("Cancel"));
    WMResizeWidget(br->cancelBtn, 80, 24);
    WMMoveWidget(br->cancelBtn, DEFAULT_WIDTH - 10 - 80, DEFAULT_HEIGHT - 29);

    br->okBtn = WMCreateCommandButton(br->win);
    WMSetButtonText(br->okBtn, _("OK"));
    WMResizeWidget(br->okBtn, 80, 24);
    WMMoveWidget(br->okBtn, DEFAULT_WIDTH - 10 - 160 - 5, DEFAULT_HEIGHT - 29);

    
    
    br->auxWidget = auxWidget;
    
    
    h = DEFAULT_HEIGHT 
	- 20 /* top and bottom spacing */
	- 25 /* popup menu and spacing */
	- 29; /* button row and spacing */
    
    if (auxWidget != NULL) {
	h -= WMWidgetHeight(auxWidget) + 5;
	
	W_ReparentView(WMWidgetView(auxWidget), 
		       WMWidgetView(br->win),
		       10, 10 + 25 + h + 5);
    }

    br->sview = WMCreateScrollView(br->win);
    WMResizeWidget(br->sview, DEFAULT_WIDTH-20, h);
    WMMoveWidget(br->sview, 10, 5 + 20 + 5);
    
    
    
    WMMapSubwidgets(br->win);
    
    return br;
}


void
ShowImageBrowser(ImageBrowser *browser)
{
    WMMapWidget(browser->win);
}

void 
CloseImageBrowser(ImageBrowser *browser)
{
    WMUnmapWidget(browser->win);
}


void 
SetImageBrowserPathList(ImageBrowser *browser, char **paths, int pathN)
{
}


void
SetImageBrowserDelegate(ImageBrowser *browser, 
			ImageBrowserDelegate *delegate)
{
    
}





void
DestroyImageBrowser(ImageBrowser *browser)
{
    WMDestroyWidget(browser->win);
    
    /**/
    
    wfree(browser);
}



