/* editmenu.c - editable menus
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1999 Alfredo K. Kojima
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

#if 0
#include <WINGsP.h>
#include <WUtil.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "editmenu.h"

typedef struct W_EditMenuItem {
    W_Class widgetClass;
    WMView *view;

    struct W_EditMenu *menu;

    char *label;

    WMTextField *textField;

    struct W_EditMenu *submenu;	       /* if it's a cascade, NULL otherwise */
} EditMenuItem;


typedef struct W_EditMenu {
    W_Class widgetClass;
    WMView *view;

    struct W_EditMenu *parent;

    char *label;

    int itemCount;
    int itemsAlloced;
    struct W_EditMenuItem **items;

    int titleHeight;
    int itemHeight;

    struct W_EditMenu *next;
    struct W_EditMenu *prev;

    /* item dragging */
    int draggedItem;
    int dragX, dragY;
} EditMenu;



/******************** WEditMenuItem ********************/

static void destroyEditMenuItem(WEditMenuItem *iPtr);
static void paintEditMenuItem(WEditMenuItem *iPtr);

static void handleItemEvents(XEvent *event, void *data);
static void handleItemActionEvents(XEvent *event, void *data);


static W_ViewProcedureTable WEditMenuItemViewProcedures = {
	NULL,
	NULL,
	NULL
};


static W_Class EditMenuItemClass = 0;


W_Class
InitEditMenuItem(WMScreen *scr)
{
    /* register our widget with WINGs and get our widget class ID */
    if (!EditMenuItemClass) {
	EditMenuItemClass = W_RegisterUserWidget(&WEditMenuItemViewProcedures);
    }

    return EditMenuItemClass;
}


WEditMenuItem*
WCreateEditMenuItem(WMWidget *parent, char *title)
{
    WEditMenuItem *iPtr;

    if (!EditMenuItemClass)
	InitEditMenuItem(WMWidgetScreen(parent));


    iPtr = wmalloc(sizeof(WEditMenuItem));

    memset(iPtr, 0, sizeof(WEditMenuItem));

    iPtr->widgetClass = EditMenuItemClass;
    
    iPtr->view = W_CreateView(W_VIEW(parent));
    if (!iPtr->view) {
	free(iPtr);
	return NULL;
    }
    iPtr->view->self = iPtr;

    iPtr->label = wstrdup(title);

    WMCreateEventHandler(iPtr->view, ExposureMask|StructureNotifyMask,
			 handleItemEvents, iPtr);

    WMCreateEventHandler(iPtr->view, ButtonPressMask, handleItemActionEvents,
			 iPtr);

    return iPtr;
}



static void
paintEditMenuItem(WEditMenuItem *iPtr)
{
    WMScreen *scr = WMWidgetScreen(iPtr);
    WMColor *black = scr->black;
    Window win = W_VIEW(iPtr)->window;
    int w = W_VIEW(iPtr)->size.width;
    int h = WMFontHeight(scr->normalFont) + 6;

    if (!iPtr->view->flags.realized)
	return;

    XClearWindow(scr->display, win);

    W_DrawRelief(scr, win, 0, 0, w+1, h, WRRaised);

    WMDrawString(scr, win, WMColorGC(black), scr->normalFont, 5, 3, iPtr->label,
		 strlen(iPtr->label));
}


static void
handleItemEvents(XEvent *event, void *data)
{
    WEditMenuItem *iPtr = (WEditMenuItem*)data;


    switch (event->type) {	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintEditMenuItem(iPtr);
	break;
	
     case DestroyNotify:
	destroyEditMenuItem(iPtr);
	break;
	
    }
}


static void
handleItemActionEvents(XEvent *event, void *data)
{
    WEditMenuItem *iPtr = (WEditMenuItem*)data;

    switch (event->type) {
     case ButtonPress:
	break;
    }
}



static void
destroyEditMenuItem(WEditMenuItem *iPtr)
{
    if (iPtr->label)
	free(iPtr->label);

    free(iPtr);
}



/******************** WEditMenu *******************/


static WEditMenu *EditMenuList = NULL;

static void destroyEditMenu(WEditMenu *mPtr);
static void paintEditMenu(WEditMenu *mPtr, int y);

static void updateMenuContents(WEditMenu *mPtr);

static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);

static void handleItemDrag(XEvent *event, void *data);


static W_ViewProcedureTable WEditMenuViewProcedures = {
	NULL,
	NULL,
	NULL
};


static W_Class EditMenuClass = 0;


W_Class
InitEditMenu(WMScreen *scr)
{
    /* register our widget with WINGs and get our widget class ID */
    if (!EditMenuClass) {

	EditMenuClass = W_RegisterUserWidget(&WEditMenuViewProcedures);
    }

    return EditMenuClass;
}



typedef struct {
    int flags;
    int window_style;
    int window_level;
    int reserved;
    Pixmap miniaturize_pixmap;         /* pixmap for miniaturize button */
    Pixmap close_pixmap;               /* pixmap for close button */
    Pixmap miniaturize_mask;           /* miniaturize pixmap mask */
    Pixmap close_mask;                 /* close pixmap mask */
    int extra_flags;
} GNUstepWMAttributes;


#define GSWindowStyleAttr       (1<<0)
#define GSWindowLevelAttr       (1<<1)


static void
writeGNUstepWMAttr(WMScreen *scr, Window window, GNUstepWMAttributes *attr)
{
    unsigned long data[9];
        
    /* handle idiot compilers where array of CARD32 != struct of CARD32 */
    data[0] = attr->flags;
    data[1] = attr->window_style;
    data[2] = attr->window_level;
    data[3] = 0;                       /* reserved */
    /* The X protocol says XIDs are 32bit */
    data[4] = attr->miniaturize_pixmap;
    data[5] = attr->close_pixmap;
    data[6] = attr->miniaturize_mask;
    data[7] = attr->close_mask;
    data[8] = attr->extra_flags;
    XChangeProperty(scr->display, window, scr->attribsAtom, scr->attribsAtom,
                    32, PropModeReplace,  (unsigned char *)data, 9);
}


static void
realizeObserver(void *self, WMNotification *not)
{
    WEditMenu *menu = (WEditMenu*)self;
    GNUstepWMAttributes attribs;
    
    memset(&attribs, 0, sizeof(GNUstepWMAttributes));
    attribs.flags = GSWindowStyleAttr|GSWindowLevelAttr;
    attribs.window_style = WMBorderlessWindowMask;
    attribs.window_level = WMSubmenuWindowLevel;

    writeGNUstepWMAttr(WMWidgetScreen(menu), menu->view->window, &attribs);
}


WEditMenu*
WCreateEditMenu(WMScreen *scr, char *title)
{
    WEditMenu *mPtr;

    if (!EditMenuClass)
	InitEditMenu(scr);


    mPtr = wmalloc(sizeof(WEditMenu));

    memset(mPtr, 0, sizeof(WEditMenu));

    mPtr->widgetClass = EditMenuClass;

    mPtr->view = W_CreateTopView(scr);
    if (!mPtr->view) {
	free(mPtr);
	return NULL;
    }
    mPtr->view->self = mPtr;

    WMAddNotificationObserver(realizeObserver, mPtr,
                              WMViewRealizedNotification, mPtr->view);

    W_SetViewBackgroundColor(mPtr->view, mPtr->view->screen->darkGray);

    mPtr->label = wstrdup(title);

    mPtr->itemsAlloced = 10;
    mPtr->items = wmalloc(sizeof(WEditMenuItem*)*mPtr->itemsAlloced);

    WMCreateEventHandler(mPtr->view, ExposureMask|StructureNotifyMask,
			 handleEvents, mPtr);

    WMCreateEventHandler(mPtr->view, ButtonPressMask,handleActionEvents, mPtr);

    updateMenuContents(mPtr);


    mPtr->itemHeight = WMFontHeight(scr->normalFont) + 6;
    mPtr->titleHeight = WMFontHeight(scr->boldFont) + 8;

    mPtr->draggedItem = -1;

    mPtr->next = EditMenuList;
    if (EditMenuList)
	EditMenuList->prev = mPtr;
    EditMenuList = mPtr;

    return mPtr;
}


WEditMenuItem*
WInsertMenuItemWithTitle(WEditMenu *mPtr, char *title, int index)
{
    WEditMenuItem *item;

    item = WCreateEditMenuItem(mPtr, title);
    item->menu = mPtr;

    WMCreateEventHandler(item->view, ButtonPressMask|ButtonReleaseMask
			 |Button1MotionMask, handleItemDrag, item);

    WMMapWidget(item);

    if (index < 0)
	index = 0;
    else if (index > mPtr->itemCount)
	index = mPtr->itemCount;

    if (mPtr->itemCount == mPtr->itemsAlloced) {
	WEditMenuItem **newList;

	newList = wmalloc(sizeof(WEditMenuItem*)*(mPtr->itemsAlloced+10));
	memset(newList, 0, sizeof(WEditMenuItem*)*(mPtr->itemsAlloced+10));

	memcpy(newList, mPtr->items, mPtr->itemsAlloced*sizeof(WEditMenuItem*));

	mPtr->itemsAlloced += 10;

	free(mPtr->items);

	mPtr->items = newList;
    }

    if (index < mPtr->itemCount) {
	memmove(&mPtr->items[index+1], &mPtr->items[index], 
		sizeof(WEditMenuItem*));
	mPtr->items[index] = item;
	mPtr->itemCount++;
    } else {
	mPtr->items[mPtr->itemCount++] = item;
    }

    updateMenuContents(mPtr);

    return item;
}


void
WSetMenuSubmenu(WEditMenu *mPtr, WEditMenu *submenu, WEditMenuItem *item)
{
    item->submenu = submenu;
    submenu->parent = mPtr;

    paintEditMenuItem(item);
}


void
WRemoveMenuItem(WEditMenu *mPtr, WEditMenuItem *item)
{
    
}


static void
updateMenuContents(WEditMenu *mPtr)
{
    WMScreen *scr = WMWidgetScreen(mPtr);
    int i;
    int newW, newH;
    int w;
    int iheight = mPtr->itemHeight;

    newW = WMWidthOfString(scr->boldFont, mPtr->label,
			   strlen(mPtr->label)) + 12 + iheight;

    newH = mPtr->titleHeight;

    for (i = 0; i < mPtr->itemCount; i++) {
	w = WMWidthOfString(scr->normalFont, mPtr->items[i]->label,
			    strlen(mPtr->items[i]->label)) + 5;
	if (w > newW)
	    newW = w;

	W_MoveView(mPtr->items[i]->view, 0, newH);
	newH += iheight;
    }

    newH--;

    W_ResizeView(mPtr->view, newW, newH);

    for (i = 0; i < mPtr->itemCount; i++) {
	W_ResizeView(mPtr->items[i]->view, newW, iheight);
    }

    paintEditMenu(mPtr, -1);
}


static void
paintMenuTitle(WEditMenu *mPtr)
{
    WMScreen *scr = WMWidgetScreen(mPtr);
    WMColor *black = scr->black;
    WMColor *white = scr->white;
    Window win = W_VIEW(mPtr)->window;
    int w = W_VIEW(mPtr)->size.width;
    int h = mPtr->titleHeight;

    XFillRectangle(scr->display, win, WMColorGC(black), 0, 0, w, h);

    W_DrawRelief(scr, win, 0, 0, w+1, h, WRRaised);

    WMDrawString(scr, win, WMColorGC(white), scr->boldFont, 5, 4, mPtr->label,
		 strlen(mPtr->label));
}


static void
paintEditMenu(WEditMenu *mPtr, int y)
{
    if (!mPtr->view->flags.realized)
	return;

    if (y < mPtr->titleHeight || y < 0)
	paintMenuTitle(mPtr);
}



static void
handleEvents(XEvent *event, void *data)
{
    WEditMenu *mPtr = (WEditMenu*)data;


    switch (event->type) {	
     case Expose:
	paintEditMenu(mPtr, event->xexpose.y);
	break;
	
     case DestroyNotify:
	destroyEditMenu(mPtr);
	break;
	
    }
}




static void
handleActionEvents(XEvent *event, void *data)
{
    WEditMenu *mPtr = (WEditMenu*)data;

    switch (event->type) {
     case ButtonPress:
	break;
    }
}


static void
editItemLabel(WEditMenuItem *iPtr)
{
    WMTextField *tPtr;

    tPtr = WMCreateTextField(iPtr);
    WMResizeWidget(tPtr, iPtr->view->size.width - 20, 
		   iPtr->view->size.height - 3);
    WMMoveWidget(tPtr, 4, 1);
    WMSetTextFieldBeveled(tPtr, False);
    WMMapWidget(tPtr);

    WMRealizeWidget(tPtr);

    iPtr->textField = tPtr;
}


static void
handleItemDrag(XEvent *event, void *data)
{
    WEditMenuItem *iPtr = (WEditMenuItem*)data;
    WEditMenu *mPtr = iPtr->menu;
    WMScreen *scr = WMWidgetScreen(mPtr);
    Bool done = False;
    int y;
    int i;
    int newIdx, oldIdx;
    int newY;

    switch (event->type) {
     case ButtonPress:
	if (WMIsDoubleClick(event)) {

	    editItemLabel(iPtr);

	} else if (event->xbutton.button == Button1) {
	    mPtr->draggedItem = 1;
	    mPtr->dragX = event->xbutton.x;
	    mPtr->dragY = event->xbutton.y;
	}
	return;
     case ButtonRelease:
	if (event->xbutton.button == Button1) {
	    mPtr->draggedItem = -1;
	}
	return;
     case MotionNotify:
	if (mPtr->draggedItem >= 0) {
	    if (abs(event->xmotion.y - mPtr->dragY) > 3
		|| abs(event->xmotion.x - mPtr->dragX) > 3) {
		mPtr->draggedItem = -1;
	    } else {
		return;
	    }
	} else {
	    return;
	}
	break;
     default:
	return;
    }

    XRaiseWindow(scr->display, iPtr->view->window);

    XGrabPointer(scr->display, mPtr->view->window, False,
		 PointerMotionMask|ButtonReleaseMask|ButtonPressMask
		 |ButtonPressMask,
		 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    y = iPtr->view->pos.y;

    while (!done) {
	XEvent ev;

	WMMaskEvent(scr->display, ButtonReleaseMask|PointerMotionMask
		    |ExposureMask, &ev);

	switch (ev.type) {
	 case ButtonRelease:
	    if (ev.xbutton.button == Button1)
		done = True;
	    break;

	 case MotionNotify:
	    y = ev.xbutton.y - mPtr->dragY;

	    if (y < mPtr->titleHeight)
		y = mPtr->titleHeight;
	    else if (y > mPtr->view->size.height - mPtr->itemHeight + 1)
		y = mPtr->view->size.height - mPtr->itemHeight + 1;

	    W_MoveView(iPtr->view, 0, y);
	    break;

	 default:
	    WMHandleEvent(&ev);
	    break;
	}
    }
    XUngrabPointer(scr->display, CurrentTime);

    for (oldIdx = 0; oldIdx < mPtr->itemCount; oldIdx++) {
	if (mPtr->items[oldIdx] == iPtr) {
	    break;
	}
    }
    assert(oldIdx < mPtr->itemCount);

    newIdx = (y - mPtr->titleHeight + mPtr->itemHeight/2) / mPtr->itemHeight;

    if (newIdx < 0)
	newIdx = 0;
    else if (newIdx >= mPtr->itemCount)
	newIdx = mPtr->itemCount - 1;

    newY = mPtr->titleHeight + newIdx * mPtr->itemHeight;
    for (i = 0; i <= 15; i++) {
	W_MoveView(iPtr->view, 0, ((newY*i)/15 + (y - (y*i)/15)));
	XFlush(scr->display);
    }

    if (oldIdx != newIdx) {
	WEditMenuItem *item;

	item = mPtr->items[oldIdx];
	mPtr->items[oldIdx] = mPtr->items[newIdx];
	mPtr->items[newIdx] = item;

	updateMenuContents(mPtr);
    }
}


static void
destroyEditMenu(WEditMenu *mPtr)
{
    WMRemoveNotificationObserver(mPtr);

    if (mPtr->next)
	mPtr->next->prev = mPtr->prev;
    if (mPtr->prev)
	mPtr->prev->next = mPtr->next;
    if (EditMenuList == mPtr)
	EditMenuList = mPtr->next;

    if (mPtr->label)
	free(mPtr->label);

    if (mPtr->items)
	free(mPtr->items);

    free(mPtr);
}
#endif
