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
    
    CallbackRec callbacks;

    WMWindow *win;

    WMFrame *pixF;
    WMList *pixL;
    WMButton *pixaB;
    WMButton *pixrB;
    WMTextField *pixT;

    WMFrame *icoF;
    WMList *icoL;
    WMButton *icoaB;
    WMButton *icorB;
    WMTextField *icoT;

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
    if (w == panel->icoaB) {
	char *text = WMGetTextFieldText(panel->icoT);

	if (text && strlen(text) > 0) {
	    i = WMGetListSelectedItemRow(panel->icoL);
	    if (i >= 0) i++;
	    addPathToList(panel->icoL, i, text);
	    WMSetListBottomPosition(panel->icoL, 
				    WMGetListNumberOfRows(panel->icoL));
	}
	if (text)
	    free(text);

	WMSetTextFieldText(panel->icoT, NULL);
    } else if (w == panel->icorB) {
	i = WMGetListSelectedItemRow(panel->icoL);

	if (i>=0)
	    WMRemoveListItem(panel->icoL, i);
    }

    /* pixmap paths */
    if (w == panel->pixaB) {
	char *text = WMGetTextFieldText(panel->pixT);

	if (text && strlen(text) > 0) {
	    i = WMGetListSelectedItemRow(panel->pixL);
	    if (i >= 0) i++;
	    addPathToList(panel->pixL, i, text);
	    WMSetListBottomPosition(panel->pixL, 
				    WMGetListNumberOfRows(panel->pixL));
	}
	if (text)
	    free(text);
	WMSetTextFieldText(panel->pixT, NULL);
    } else if (w == panel->pixrB) {
	i = WMGetListSelectedItemRow(panel->pixL);
	
	if (i>=0)
	    WMRemoveListItem(panel->pixL, i);
    }
}


static void
textEditedObserver(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;

    switch ((int)WMGetNotificationClientData(notification)) {
     case WMReturnTextMovement:
	if (WMGetNotificationObject(notification) == panel->icoT)
	    WMPerformButtonClick(panel->icoaB);
	else
	    WMPerformButtonClick(panel->pixaB);
	break;

     case WMIllegalTextMovement:
	if (WMGetNotificationObject(notification) == panel->icoT) {
	    WMSetButtonImage(panel->icoaB, NULL);
	    WMSetButtonImage(panel->icoaB, NULL);
	} else {	
	    WMSetButtonImage(panel->pixaB, NULL);
	    WMSetButtonImage(panel->pixaB, NULL);
	}
	break;
    } 
}


static void
textBeginObserver(void *observerData, WMNotification *notification)
{
    _Panel *panel = (_Panel*)observerData;
    WMScreen *scr = WMWidgetScreen(panel->win);
    WMPixmap *arrow1 = WMGetSystemPixmap(scr, WSIReturnArrow);
    WMPixmap *arrow2 = WMGetSystemPixmap(scr, WSIHighlightedReturnArrow);

    if (WMGetNotificationObject(notification)==panel->icoT) {
	WMSetButtonImage(panel->icoaB, arrow1);
	WMSetButtonAltImage(panel->icoaB, arrow2);
    } else {	
	WMSetButtonImage(panel->pixaB, arrow1);
	WMSetButtonAltImage(panel->pixaB, arrow2);    
    }
}



static void
listClick(WMWidget *w, void *data)
{
    _Panel *panel = (_Panel*)data;
    char *t;
    
    if (w == panel->icoL) {
	t = WMGetListSelectedItem(panel->icoL)->text;
	WMSetTextFieldText(panel->icoT, t);
    } else {
	t = WMGetListSelectedItem(panel->pixL)->text;
	WMSetTextFieldText(panel->pixT, t);
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

    panel->white = WMWhiteColor(scr);
    panel->black = WMBlackColor(scr);
    panel->red = WMCreateRGBColor(scr, 0xffff, 0, 0, True);
    panel->font = WMSystemFontOfSize(scr, 12);

    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);

    /* icon path */
    panel->icoF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->icoF, 230, 210);
    WMMoveWidget(panel->icoF, 25, 10);
    WMSetFrameTitle(panel->icoF, _("Icon Search Paths"));
    
    panel->icoL = WMCreateList(panel->icoF);
    WMResizeWidget(panel->icoL, 200, 120);
    WMMoveWidget(panel->icoL, 15, 20);
    WMSetListAction(panel->icoL, listClick, panel);
    WMSetListUserDrawProc(panel->icoL, paintItem);
    WMHangData(panel->icoL, panel);
    
    panel->icoaB = WMCreateCommandButton(panel->icoF);
    WMResizeWidget(panel->icoaB, 90, 24);
    WMMoveWidget(panel->icoaB, 125, 145);
    WMSetButtonText(panel->icoaB, _("Add"));
    WMSetButtonAction(panel->icoaB, pushButton, panel);
    WMSetButtonImagePosition(panel->icoaB, WIPRight);

    panel->icorB = WMCreateCommandButton(panel->icoF);
    WMResizeWidget(panel->icorB, 90, 24);
    WMMoveWidget(panel->icorB, 15, 145);
    WMSetButtonText(panel->icorB, _("Remove"));
    WMSetButtonAction(panel->icorB, pushButton, panel);

    panel->icoT = WMCreateTextField(panel->icoF);
    WMResizeWidget(panel->icoT, 200, 20);
    WMMoveWidget(panel->icoT, 15, 175);
    WMAddNotificationObserver(textEditedObserver, panel, 
			      WMTextDidEndEditingNotification, panel->icoT);
    WMAddNotificationObserver(textBeginObserver, panel,
			      WMTextDidBeginEditingNotification, panel->icoT);

    WMMapSubwidgets(panel->icoF);

    /* pixmap path */
    panel->pixF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->pixF, 230, 210);
    WMMoveWidget(panel->pixF, 270, 10);
    WMSetFrameTitle(panel->pixF, _("Pixmap Search Paths"));
    
    panel->pixL = WMCreateList(panel->pixF);
    WMResizeWidget(panel->pixL, 200, 120);
    WMMoveWidget(panel->pixL, 15, 20);
    WMSetListAction(panel->pixL, listClick, panel);
    WMSetListUserDrawProc(panel->pixL, paintItem);
    WMHangData(panel->pixL, panel);
    
    panel->pixaB = WMCreateCommandButton(panel->pixF);
    WMResizeWidget(panel->pixaB, 90, 24);
    WMMoveWidget(panel->pixaB, 125, 145);
    WMSetButtonText(panel->pixaB, _("Add"));
    WMSetButtonAction(panel->pixaB, pushButton, panel);
    WMSetButtonImagePosition(panel->pixaB, WIPRight);
    
    panel->pixrB = WMCreateCommandButton(panel->pixF);
    WMResizeWidget(panel->pixrB, 90, 24);
    WMMoveWidget(panel->pixrB, 15, 145);
    WMSetButtonText(panel->pixrB, _("Remove"));
    WMSetButtonAction(panel->pixrB, pushButton, panel);
    
    panel->pixT= WMCreateTextField(panel->pixF);
    WMResizeWidget(panel->pixT, 200, 20);
    WMMoveWidget(panel->pixT, 15, 175);
    WMAddNotificationObserver(textEditedObserver, panel, 
			      WMTextDidEndEditingNotification, panel->pixT);
    WMAddNotificationObserver(textBeginObserver, panel,
			      WMTextDidBeginEditingNotification, panel->pixT);

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

    panel->win = win;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    
    AddSection(panel, ICON_FILE);

    return panel;
}
