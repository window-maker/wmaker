



#include "WINGsP.h"

typedef struct ItemList {
    char *text;
    struct ItemList *nextPtr;
    unsigned int disabled:1;
} ItemList;

typedef struct W_PopUpButton {
    W_Class widgetClass;
    WMView *view;
    
    void *clientData;
    WMAction *action;

    char *caption;

    ItemList *items;
    short itemCount;

    short selectedItemIndex;
    
    short highlightedItem;

    ItemList *selectedItem;	       /* selected item if it is a menu btn */
    
    WMView *menuView;		       /* override redirect popup menu */

    struct {
	unsigned int pullsDown:1;

	unsigned int configured:1;
	
	unsigned int insideMenu:1;
    } flags;
} PopUpButton;


#define MENU_BLINK_DELAY	60000
#define MENU_BLINK_COUNT	2


W_ViewProcedureTable _PopUpButtonViewProcedures = {
    NULL,
	NULL,
	NULL
};


#define DEFAULT_WIDTH	60
#define DEFAULT_HEIGHT 	20
#define DEFAULT_CAPTION	""


static void destroyPopUpButton(PopUpButton *bPtr);
static void paintPopUpButton(PopUpButton *bPtr);

static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);

static void resizeMenu(PopUpButton *bPtr);

	      
WMPopUpButton*
WMCreatePopUpButton(WMWidget *parent)
{
    PopUpButton *bPtr;
    W_Screen *scr = W_VIEW(parent)->screen;

    
    bPtr = wmalloc(sizeof(PopUpButton));
    memset(bPtr, 0, sizeof(PopUpButton));

    bPtr->widgetClass = WC_PopUpButton;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;
    
    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, bPtr);


    W_ResizeView(bPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    bPtr->caption = wstrdup(DEFAULT_CAPTION);

    WMCreateEventHandler(bPtr->view, ButtonPressMask|ButtonReleaseMask,
			 handleActionEvents, bPtr);

    bPtr->menuView = W_CreateTopView(scr);
    bPtr->menuView->attribs.override_redirect = True;
    bPtr->menuView->attribFlags |= CWOverrideRedirect;

    W_ResizeView(bPtr->menuView, bPtr->view->size.width, 1);

    WMCreateEventHandler(bPtr->menuView, ButtonPressMask|ButtonReleaseMask
			 |EnterWindowMask|LeaveWindowMask|ButtonMotionMask,
			 handleActionEvents, bPtr);

    return bPtr;
}


void
WMSetPopUpButtonAction(WMPopUpButton *bPtr, WMAction *action, void *clientData)
{
    CHECK_CLASS(bPtr, WC_PopUpButton);
    
    bPtr->action = action;
    
    bPtr->clientData = clientData;
}


void
WMAddPopUpButtonItem(WMPopUpButton *bPtr, char *title)
{
    ItemList *itemPtr, *tmp;
    
    CHECK_CLASS(bPtr, WC_PopUpButton);

    itemPtr = wmalloc(sizeof(ItemList));
    memset(itemPtr, 0, sizeof(ItemList));
    itemPtr->text = wstrdup(title);
    
    /* append item to list */
    tmp = bPtr->items;
    if (!tmp)
	bPtr->items = itemPtr;
    else {
	while (tmp->nextPtr!=NULL) 
	    tmp = tmp->nextPtr;
	tmp->nextPtr = itemPtr;
    }
    
    bPtr->itemCount++;
    
    if (bPtr->menuView && bPtr->menuView->flags.realized)
	resizeMenu(bPtr);
}


void
WMInsertPopUpButtonItem(WMPopUpButton *bPtr, int index, char *title)
{
    ItemList *itemPtr;

    CHECK_CLASS(bPtr, WC_PopUpButton);
    
    if (index < 0)
	index = 0;
    if (index >= bPtr->itemCount) {
	WMAddPopUpButtonItem(bPtr, title);
	return;
    }
    
    itemPtr = wmalloc(sizeof(ItemList));
    memset(itemPtr, 0, sizeof(ItemList));
    itemPtr->text = wstrdup(title);

    if (index == 0) {
	itemPtr->nextPtr = bPtr->items;
	bPtr->items = itemPtr;
    } else {
	ItemList *tmp;
	int i = index;
	
	tmp = bPtr->items;
	/* insert item in list */
	while (--i > 0) {
	    tmp = tmp->nextPtr;
	}
	bPtr->items->nextPtr = tmp->nextPtr;
	tmp->nextPtr = bPtr->items;
    }
    
    bPtr->itemCount++;
  
    /* if there is an selected item, update it's index to match the new 
     * position */
    if (index < bPtr->selectedItemIndex)
	bPtr->selectedItemIndex++;
    
    if (bPtr->menuView && bPtr->menuView->flags.realized)
	resizeMenu(bPtr);
}


void
WMRemovePopUpButtonItem(WMPopUpButton *bPtr, int index)
{
    ItemList *tmp;

    CHECK_CLASS(bPtr, WC_PopUpButton);
    
    if (index < 0 || index >= bPtr->itemCount)
	return;


    if (index == 0) {
	free(bPtr->items->text);
	tmp = bPtr->items->nextPtr;
	free(bPtr->items);
	bPtr->items = tmp;
    } else {
	ItemList *next;
	int i = index;
	
	tmp = bPtr->items;
	while (--i > 0)
	    tmp = tmp->nextPtr;
	next = tmp->nextPtr->nextPtr;
	
	free(tmp->nextPtr->text);
	free(tmp->nextPtr);

	tmp->nextPtr = next;
    }

    bPtr->itemCount--;

    if (bPtr->selectedItem!=NULL && !bPtr->flags.pullsDown) {
	if (index < bPtr->selectedItemIndex)
	    bPtr->selectedItemIndex--;
	else if (index == bPtr->selectedItemIndex) {
	    /* reselect first item if the removed item is the 
	     * selected one */
	    bPtr->selectedItem = bPtr->items;
	    bPtr->selectedItemIndex = 0;
	    if (bPtr->view->flags.mapped)
		paintPopUpButton(bPtr);
	}
    }
    
    if (bPtr->menuView && bPtr->menuView->flags.realized)
	resizeMenu(bPtr);
}


void
WMSetPopUpButtonSelectedItem(WMPopUpButton *bPtr, int index)
{
    ItemList *itemPtr = bPtr->items;
    int i = index;
    
    if (index < 0) {
	bPtr->selectedItem = NULL;
	if (bPtr->view->flags.mapped)
	    paintPopUpButton(bPtr);
	return;
    }
    
    while (i-- > 0) {
	itemPtr = itemPtr->nextPtr;
    }
    bPtr->selectedItem = itemPtr;
    bPtr->selectedItemIndex = index;
    
    if (bPtr->view->flags.mapped)
	paintPopUpButton(bPtr);
}

int
WMGetPopUpButtonSelectedItem(WMPopUpButton *bPtr)
{
    if (!bPtr->flags.pullsDown && bPtr->selectedItem==NULL)
	return -1;
    else
	return bPtr->selectedItemIndex;
}


void
WMSetPopUpButtonText(WMPopUpButton *bPtr, char *text)
{
    if (bPtr->caption)
	free(bPtr->caption);
    if (text)
	bPtr->caption = wstrdup(text);
    else
	bPtr->caption = NULL;
    if (bPtr->view->flags.realized) {
	if (bPtr->flags.pullsDown || bPtr->selectedItemIndex < 0) {
	    paintPopUpButton(bPtr);
	}
    }
}



void 
WMSetPopUpButtonItemEnabled(WMPopUpButton *bPtr, int index, Bool flag)
{
    int i;
    ItemList *item = bPtr->items;
    
    if (index < 0 || index >= bPtr->itemCount)
	return;
    
    for (i = 0; i<index; i++)
	item=item->nextPtr;
    
    item->disabled = !flag;
}


void
WMSetPopUpButtonPullsDown(WMPopUpButton *bPtr, Bool flag)
{
    bPtr->flags.pullsDown = flag;
    if (!flag) {
	bPtr->selectedItem = bPtr->items;
	if (bPtr->selectedItem)
	    bPtr->selectedItemIndex = 0;
	else
	    bPtr->selectedItemIndex = -1;
    } else
	bPtr->selectedItemIndex = -1;

    if (bPtr->view->flags.mapped)
	paintPopUpButton(bPtr);
}


int
WMGetPopUpButtonNumberOfItems(WMPopUpButton *bPtr)
{
    return bPtr->itemCount;
}


char*
WMGetPopUpButtonItem(WMPopUpButton *bPtr, int index)
{
    ItemList *itemPtr = bPtr->items;
    
    if ((index < 0) || (index >= bPtr->itemCount))
	return NULL;
    
    while (index-->0)
	itemPtr = itemPtr->nextPtr;
    
    return itemPtr->text;
}


static void
paintPopUpButton(PopUpButton *bPtr)
{
    W_Screen *scr = bPtr->view->screen;
    char *caption;
    Pixmap pixmap;

    
    if (bPtr->flags.pullsDown) {
	caption = bPtr->caption;
    } else {
	if (bPtr->selectedItem == NULL) {
	    /* if no item selected, show the caption */
	    caption = bPtr->caption;
	} else {
	    caption = bPtr->selectedItem->text;
	}
    }

    pixmap = XCreatePixmap(scr->display, bPtr->view->window, 
			   bPtr->view->size.width, bPtr->view->size.height,
			   scr->depth);
    XFillRectangle(scr->display, pixmap, W_GC(scr->gray), 0, 0,
		   bPtr->view->size.width, bPtr->view->size.height);

    W_DrawRelief(scr, pixmap, 0, 0, bPtr->view->size.width,
		 bPtr->view->size.height, WRRaised);

    if (caption) {
	W_PaintText(bPtr->view, pixmap, scr->normalFont,
		    6, (bPtr->view->size.height-scr->normalFont->height)/2,
		    bPtr->view->size.width, WALeft, W_GC(scr->black), False,
		    caption, strlen(caption));
    }

    if (bPtr->flags.pullsDown) {
	XCopyArea(scr->display, scr->pullDownIndicator->pixmap, 
		  pixmap, scr->copyGC, 0, 0, scr->pullDownIndicator->width,
		  scr->pullDownIndicator->height, 
		  bPtr->view->size.width-scr->pullDownIndicator->width-4,
		  (bPtr->view->size.height-scr->pullDownIndicator->height)/2);
    } else {
	int x, y;
	
	x = bPtr->view->size.width - scr->popUpIndicator->width - 4;
	y = (bPtr->view->size.height-scr->popUpIndicator->height)/2;
	
	XSetClipOrigin(scr->display, scr->clipGC, x, y);
	XSetClipMask(scr->display, scr->clipGC, scr->popUpIndicator->mask);
	XCopyArea(scr->display, scr->popUpIndicator->pixmap, pixmap,
		  scr->clipGC, 0, 0, scr->popUpIndicator->width,
		  scr->popUpIndicator->height, x, y);
    }

    XCopyArea(scr->display, pixmap, bPtr->view->window, scr->copyGC, 0, 0,
	      bPtr->view->size.width, bPtr->view->size.height, 0, 0);
    
    XFreePixmap(scr->display, pixmap);
}



static void
handleEvents(XEvent *event, void *data)
{
    PopUpButton *bPtr = (PopUpButton*)data;

    CHECK_CLASS(data, WC_PopUpButton);


    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintPopUpButton(bPtr);
	break;
	
     case DestroyNotify:
	destroyPopUpButton(bPtr);
	break;
    }
}



static void
paintMenuEntry(PopUpButton *bPtr, int index, int highlight)
{
    W_Screen *scr = bPtr->view->screen;
    int i;
    int yo;
    ItemList *itemPtr;
    int width, height, itemHeight;
    
    itemHeight = bPtr->view->size.height;
    width = bPtr->view->size.width;
    height = itemHeight * bPtr->itemCount;
    yo = (itemHeight - scr->normalFont->height)/2;

    if (!highlight) {
	XClearArea(scr->display, bPtr->menuView->window, 0, index*itemHeight,
		   width, itemHeight, False);
	return;
    } else if (index < 0 && bPtr->flags.pullsDown) {
	return;
    }
	
    XFillRectangle(scr->display, bPtr->menuView->window, W_GC(scr->white),
		   1, index*itemHeight+1, width-3, itemHeight-3);
    
    itemPtr = bPtr->items;
    for (i = 0; i < index; i++)
	itemPtr = itemPtr->nextPtr;
    
    W_DrawRelief(scr, bPtr->menuView->window, 0, index*itemHeight, 
		 width, itemHeight, WRRaised);

    W_PaintText(bPtr->menuView, bPtr->menuView->window, scr->normalFont,  6, 
		index*itemHeight + yo, width, WALeft, W_GC(scr->black), False,
		itemPtr->text, strlen(itemPtr->text));
	
    if (!bPtr->flags.pullsDown && index == bPtr->selectedItemIndex) {
	XCopyArea(scr->display, scr->popUpIndicator->pixmap, 
		  bPtr->menuView->window, scr->copyGC, 0, 0, 
		  scr->popUpIndicator->width, scr->popUpIndicator->height,
		  width-scr->popUpIndicator->width-4, 
		  i*itemHeight+(itemHeight-scr->popUpIndicator->height)/2);
    }
}


Pixmap
makeMenuPixmap(PopUpButton *bPtr)
{
    Pixmap pixmap;
    W_Screen *scr = bPtr->view->screen;
    int i;
    int yo;
    ItemList *itemPtr;
    int width, height, itemHeight;
    
    itemHeight = bPtr->view->size.height;
    width = bPtr->view->size.width;
    height = itemHeight * bPtr->itemCount;
    yo = (itemHeight - scr->normalFont->height)/2;
    
    pixmap = XCreatePixmap(scr->display, bPtr->view->window, width, height, 
			   scr->depth);
    
    XFillRectangle(scr->display, pixmap, W_GC(scr->gray), 0, 0, width, height);
    
    itemPtr = bPtr->items;
    for (i = 0; i < bPtr->itemCount; i++) {
	GC gc;
	
	W_DrawRelief(scr, pixmap, 0, i*itemHeight, width, itemHeight, 
		     WRRaised);

        if (itemPtr->disabled)
	    gc = W_GC(scr->darkGray);
	else
	    gc = W_GC(scr->black);

	W_PaintText(bPtr->menuView, pixmap, scr->normalFont,  6, 
		    i*itemHeight + yo, width, WALeft, gc, False,
		    itemPtr->text, strlen(itemPtr->text));
	
	if (!bPtr->flags.pullsDown && i == bPtr->selectedItemIndex) {
	    XCopyArea(scr->display, scr->popUpIndicator->pixmap, pixmap, 
		      scr->copyGC, 0, 0, scr->popUpIndicator->width,
		      scr->popUpIndicator->height, 
		      width-scr->popUpIndicator->width-4,
		      i*itemHeight+(itemHeight-scr->popUpIndicator->height)/2);
	}	
	itemPtr = itemPtr->nextPtr;
    }
    
    return pixmap;
}


static void
resizeMenu(PopUpButton *bPtr)
{
    int height;
    
    height = bPtr->itemCount * bPtr->view->size.height;
    W_ResizeView(bPtr->menuView, bPtr->view->size.width, height);
}


static void
popUpMenu(PopUpButton *bPtr)
{
    W_Screen *scr = bPtr->view->screen;
    Window dummyW;
    int x, y;

    if (!bPtr->menuView->flags.realized) {
	W_RealizeView(bPtr->menuView);
	resizeMenu(bPtr);
    }
    
    if (bPtr->itemCount < 1)
	return;
    
    XTranslateCoordinates(scr->display, bPtr->view->window, scr->rootWin,
			  0, 0, &x, &y, &dummyW);

    if (bPtr->flags.pullsDown) {
	y += bPtr->view->size.height;
    } else {
	y -= bPtr->view->size.height*bPtr->selectedItemIndex;
    }
    W_MoveView(bPtr->menuView, x, y);
    
    XSetWindowBackgroundPixmap(scr->display, bPtr->menuView->window,
			       makeMenuPixmap(bPtr));
    XClearWindow(scr->display, bPtr->menuView->window);
    
    W_MapView(bPtr->menuView);
    
    bPtr->highlightedItem = 0;
    if (!bPtr->flags.pullsDown && bPtr->selectedItem != NULL)
	paintMenuEntry(bPtr, bPtr->selectedItemIndex, True);
}


static void
popDownMenu(PopUpButton *bPtr)
{
    W_UnmapView(bPtr->menuView);
    
    /* free the background pixmap used to draw the menu contents */
    XSetWindowBackgroundPixmap(bPtr->view->screen->display, 
			       bPtr->menuView->window, None);
}


static int
itemIsEnabled(PopUpButton *bPtr, int index)
{
    ItemList *item = bPtr->items;
     
    while (index-- > 0)
	item = item->nextPtr;

    return !item->disabled;
}

static void
handleActionEvents(XEvent *event, void *data)
{
    PopUpButton *bPtr = (PopUpButton*)data;
    int oldItem;

    CHECK_CLASS(data, WC_PopUpButton);

    if (bPtr->itemCount < 1)
	return;
    
    switch (event->type) {
	/* called for menuView */
     case LeaveNotify:
	bPtr->flags.insideMenu = 0;
	if (bPtr->menuView->flags.mapped)
	    paintMenuEntry(bPtr, bPtr->highlightedItem, False);
	bPtr->highlightedItem = -1;
	break;
	
     case EnterNotify:
	bPtr->flags.insideMenu = 1;
	break;
	
     case MotionNotify:
	if (bPtr->flags.insideMenu) {
	    oldItem = bPtr->highlightedItem;
	    bPtr->highlightedItem = event->xmotion.y / bPtr->view->size.height;
	    if (oldItem!=bPtr->highlightedItem) {
		paintMenuEntry(bPtr, oldItem, False);
		paintMenuEntry(bPtr, bPtr->highlightedItem, 
			       itemIsEnabled(bPtr, bPtr->highlightedItem));
	    }
	}
	break;
	
	/* called for bPtr->view */
     case ButtonPress:
	popUpMenu(bPtr);
	if (!bPtr->flags.pullsDown) {
	    bPtr->highlightedItem = bPtr->selectedItemIndex;
	    bPtr->flags.insideMenu = 1;
	} else {
	    bPtr->highlightedItem = -1;
	    bPtr->flags.insideMenu = 0;
	}
	XGrabPointer(bPtr->view->screen->display, bPtr->menuView->window,
		     False, ButtonReleaseMask|ButtonMotionMask|EnterWindowMask
		     |LeaveWindowMask, GrabModeAsync, GrabModeAsync, 
		     None, None, CurrentTime);
	break;
 
     case ButtonRelease:
	XUngrabPointer(bPtr->view->screen->display, event->xbutton.time);
	if (!bPtr->flags.pullsDown)
	    popDownMenu(bPtr);
	if (bPtr->flags.insideMenu && bPtr->highlightedItem>=0) {
            if (itemIsEnabled(bPtr, bPtr->highlightedItem)) {
		int i;
                WMSetPopUpButtonSelectedItem(bPtr, bPtr->highlightedItem);

		if (bPtr->flags.pullsDown) {
		    for (i=0; i<MENU_BLINK_COUNT; i++) {
			paintMenuEntry(bPtr, bPtr->highlightedItem, False);
			XSync(bPtr->view->screen->display, 0);
			wusleep(MENU_BLINK_DELAY);
			paintMenuEntry(bPtr, bPtr->highlightedItem, True);
			XSync(bPtr->view->screen->display, 0);
			wusleep(MENU_BLINK_DELAY);
		    }
		}
		paintMenuEntry(bPtr, bPtr->highlightedItem, False);
		popDownMenu(bPtr);
                if (bPtr->action)
                    (*bPtr->action)(bPtr, bPtr->clientData);
            }
	}
	if (bPtr->menuView->flags.mapped)
	    popDownMenu(bPtr);
	break;
    }    
}



static void
destroyPopUpButton(PopUpButton *bPtr)
{
    ItemList *itemPtr, *tmp;
    
    itemPtr = bPtr->items;
    while (itemPtr!=NULL) {
	free(itemPtr->text);
	tmp = itemPtr->nextPtr;
	free(itemPtr);
	itemPtr = tmp;
    }
    
    if (bPtr->caption)
	free(bPtr->caption);

    /* have to destroy explicitly because the popup is a toplevel */
    W_DestroyView(bPtr->menuView);

    free(bPtr);
}
