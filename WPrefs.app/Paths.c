/* Paths.c- pixmap/icon paths
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
#include <unistd.h>

typedef struct _Panel {
    WMFrame *frame;
    char *sectionName;

    char *description;
    
    CallbackRec callbacks;

    WMWindow *win;
    
    WMTabView *tabv;

    WMFrame *pixF;
    WMList *pixL;
    WMButton *pixaB;
    WMButton *pixrB;

    WMFrame *icoF;
    WMList *icoL;
    WMButton *icoaB;
    WMButton *icorB;

    WMColor *red;
    WMColor *black;
    WMColor *white;
    WMFont *font;
} _Panel;



#define ICON_FILE	"paths"


static void
addPathToList(WMList *list, int index, char *path)
{
    char *fpath = wexpandpath(path);
    WMListItem *item;

    item = WMInsertListItem(list, index, path);

    if (access(fpath, X_OK)!=0) {
	item->uflags = 1;
    }
    free(fpath);
}


static void
showData(_Panel *panel)
{
    proplist_t array, val;
    int i;
    
    array = GetObjectForKey("IconPath");
    if (!array || !PLIsArray(array)) {
	if (array)
	    wwarning(_("bad value in option IconPath. Using default path list"));
	addPathToList(panel->icoL, -1, "~/pixmaps");
	addPathToList(panel->icoL, -1, "~/GNUstep/Library/Icons");
	addPathToList(panel->icoL, -1, "/usr/include/X11/pixmaps");
	addPathToList(panel->icoL, -1, "/usr/local/share/WindowMaker/Icons");
	addPathToList(panel->icoL, -1, "/usr/local/share/WindowMaker/Pixmaps");
	addPathToList(panel->icoL, -1, "/usr/share/WindowMaker/Icons");
    } else {
	for (i=0; i<PLGetNumberOfElements(array); i++) {
	    val = PLGetArrayElement(array, i);
	    addPathToList(panel->icoL, -1, PLGetString(val));
	}
    }

    array = GetObjectForKey("PixmapPath");
    if (!array || !PLIsArray(array)) {
	if (array)
	    wwarning(_("bad value in option PixmapPath. Using default path list"));
	addPathToList(panel->pixL, -1, "~/pixmaps");
	addPathToList(panel->pixL, -1, "~/GNUstep/Library/WindowMaker/Pixmaps");
	addPathToList(panel->pixL, -1, "/usr/local/share/WindowMaker/Pixmaps");
    } else {
	for (i=0; i<PLGetNumberOfElements(array); i++) {
	    val = PLGetArrayElement(array, i);
	    addPathToList(panel->pixL, -1, PLGetString(val));
	}
    }
}


static void
pushButton(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    int i;

    /* icon paths */
    if (w == panel->icorB) {
	i = WMGetListSelectedItemRow(panel->icoL);

	if (i>=0)
	    WMRemoveListItem(panel->icoL, i);
    }

    /* pixmap paths */
    if (w == panel->pixrB) {
	i = WMGetListSelectedItemRow(panel->pixL);

	if (i>=0)
	    WMRemoveListItem(panel->pixL, i);
    }
}


static void
browseForFile(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    WMFilePanel *filePanel;

    filePanel = WMGetOpenPanel(WMWidgetScreen(w));

    WMSetFilePanelCanChooseFiles(filePanel, False);

    if (WMRunModalFilePanelForDirectory(filePanel, panel->win, "/",
                                        _("Select path"), NULL) == True) {
        char *str = WMGetFilePanelFileName(filePanel);

        if (str) {
            int len = strlen(str);

            /* Remove the trailing '/' except if the path is exactly / */
            if (len > 1 && str[len-1] == '/') {
                str[len-1] = '\0';
                len--;
            }
            if (len > 0) {
                WMList *lPtr;
                int i;

                if (w == panel->icoaB)
                    lPtr = panel->icoL;
                else if (w == panel->pixaB)
                    lPtr = panel->pixL;

                i = WMGetListSelectedItemRow(lPtr);
                if (i >= 0) i++;
                addPathToList(lPtr, i, str);
                WMSetListBottomPosition(lPtr, WMGetListNumberOfRows(lPtr));

                free(str);
            }
        }
    }
}



static void
paintItem(WMList *lPtr, int index, Drawable d, char *text, int state, 
	  WMRect *rect)
{
    int width, height, x, y;
    _Panel *panel = (_Panel*)WMGetHangedData(lPtr);
    WMScreen *scr = WMWidgetScreen(lPtr);
    Display *dpy = WMScreenDisplay(scr);

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
	XFillRectangle(dpy, d, WMColorGC(panel->white), x, y, width, 
		       height);
    else 
	XClearArea(dpy, d, x, y, width, height, False);

    if (state & 1) {
	WMDrawString(scr, d, WMColorGC(panel->red), panel->font, x+4, y, 
		     text, strlen(text));
    } else {
	WMDrawString(scr, d, WMColorGC(panel->black), panel->font, x+4, y, 
		     text, strlen(text));
    }
}



static void
storeData(_Panel *panel)
{
    proplist_t list;
    proplist_t tmp;
    int i;
    char *p;
    
    list = PLMakeArrayFromElements(NULL, NULL);
    for (i=0; i<WMGetListNumberOfRows(panel->icoL); i++) {
	p = WMGetListItem(panel->icoL, i)->text;
	tmp = PLMakeString(p);
	PLAppendArrayElement(list, tmp);
    }
    SetObjectForKey(list, "IconPath");
    
    list = PLMakeArrayFromElements(NULL, NULL);
    for (i=0; i<WMGetListNumberOfRows(panel->pixL); i++) {
	p = WMGetListItem(panel->pixL, i)->text;
	tmp = PLMakeString(p);
	PLAppendArrayElement(list, tmp);
    }
    SetObjectForKey(list, "PixmapPath");
}



static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMTabViewItem *tab;

    panel->white = WMWhiteColor(scr);
    panel->black = WMBlackColor(scr);
    panel->red = WMCreateRGBColor(scr, 0xffff, 0, 0, True);
    panel->font = WMSystemFontOfSize(scr, 12);
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    panel->tabv = WMCreateTabView(panel->frame);
    WMMoveWidget(panel->tabv, 10, 10);
    WMResizeWidget(panel->tabv, 500, 215);

    

    /* icon path */
    panel->icoF = WMCreateFrame(panel->frame);
    WMSetFrameRelief(panel->icoF, WRFlat);
    WMResizeWidget(panel->icoF, 230, 210);

    tab = WMCreateTabViewItemWithIdentifier(0);
    WMSetTabViewItemView(tab, WMWidgetView(panel->icoF));
    WMAddItemInTabView(panel->tabv, tab);
    WMSetTabViewItemLabel(tab, "Icon Search Paths");
    
    panel->icoL = WMCreateList(panel->icoF);
    WMResizeWidget(panel->icoL, 480, 147);
    WMMoveWidget(panel->icoL, 10, 10);
    WMSetListUserDrawProc(panel->icoL, paintItem);
    WMHangData(panel->icoL, panel);
    
    panel->icoaB = WMCreateCommandButton(panel->icoF);
    WMResizeWidget(panel->icoaB, 95, 24);
    WMMoveWidget(panel->icoaB, 110, 165);
    WMSetButtonText(panel->icoaB, _("Add"));
    WMSetButtonAction(panel->icoaB, browseForFile, panel);
    WMSetButtonImagePosition(panel->icoaB, WIPRight);

    panel->icorB = WMCreateCommandButton(panel->icoF);
    WMResizeWidget(panel->icorB, 95, 24);
    WMMoveWidget(panel->icorB, 10, 165);
    WMSetButtonText(panel->icorB, _("Remove"));
    WMSetButtonAction(panel->icorB, pushButton, panel);

    WMMapSubwidgets(panel->icoF);

    /* pixmap path */
    panel->pixF = WMCreateFrame(panel->frame);
    WMSetFrameRelief(panel->pixF, WRFlat);
    WMResizeWidget(panel->pixF, 230, 210);
    
    tab = WMCreateTabViewItemWithIdentifier(0);
    WMSetTabViewItemView(tab, WMWidgetView(panel->pixF));
    WMAddItemInTabView(panel->tabv, tab);
    WMSetTabViewItemLabel(tab, "Pixmap Search Paths");
    
    panel->pixL = WMCreateList(panel->pixF);
    WMResizeWidget(panel->pixL, 480, 147);
    WMMoveWidget(panel->pixL, 10, 10);
    WMSetListUserDrawProc(panel->pixL, paintItem);
    WMHangData(panel->pixL, panel);
    
    panel->pixaB = WMCreateCommandButton(panel->pixF);
    WMResizeWidget(panel->pixaB, 95, 24);
    WMMoveWidget(panel->pixaB, 110, 165);
    WMSetButtonText(panel->pixaB, _("Add"));
    WMSetButtonAction(panel->pixaB, browseForFile, panel);
    WMSetButtonImagePosition(panel->pixaB, WIPRight);
    
    panel->pixrB = WMCreateCommandButton(panel->pixF);
    WMResizeWidget(panel->pixrB, 95, 24);
    WMMoveWidget(panel->pixrB, 10, 165);
    WMSetButtonText(panel->pixrB, _("Remove"));
    WMSetButtonAction(panel->pixrB, pushButton, panel);


    WMMapSubwidgets(panel->pixF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);
    
    showData(panel);
}



Panel*
InitPaths(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Search Path Configuration");

    panel->description = _("Search paths to use when looking for pixmaps\n"
			   "and icons.");

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    
    AddSection(panel, ICON_FILE);

    return panel;
}
