



#include "WINGsP.h"

char *WMListDidScrollNotification = "WMListDidScrollNotification";
char *WMListSelectionDidChangeNotification = "WMListSelectionDidChangeNotification";

typedef struct W_List {
    W_Class widgetClass;
    W_View *view;

    WMListItem *items;	       /* array of items */
    short itemCount;
    
    short selectedItem;
    
    short itemHeight;
    
    short topItem;		       /* index of first visible item */
    
    short fullFitLines;		       /* no of lines that fit entirely */

    void *clientData;
    WMAction *action;
    void *doubleClientData;
    WMAction *doubleAction;
    
    WMListDrawProc *draw;
    
    WMHandlerID	*idleID;	       /* for updating the scroller after
					* adding elements */

    WMScroller *vScroller;
    /*
    WMScroller *hScroller;
     */

    struct {
	unsigned int allowMultipleSelection:1;
	unsigned int userDrawn:1;
	unsigned int userItemHeight:1;
	/* */
	unsigned int dontFitAll:1;     /* 1 = last item won't be fully visible */
	unsigned int redrawPending:1;
	unsigned int buttonPressed:1;
	unsigned int buttonWasPressed:1;
    } flags;
} List;



#define DEFAULT_WIDTH	150
#define DEFAULT_HEIGHT	150


static void destroyList(List *lPtr);
static void paintList(List *lPtr);


static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);
static void updateScroller(List *lPtr);

static void vScrollCallBack(WMWidget *scroller, void *self);

static void updateGeometry(WMList *lPtr);
static void didResizeList();


W_ViewDelegate _ListViewDelegate = {
    NULL,
	NULL,
	didResizeList,
	NULL,
	NULL
};


	      
WMList*
WMCreateList(WMWidget *parent)
{
    List *lPtr;
    W_Screen *scrPtr = W_VIEW(parent)->screen;

    lPtr = wmalloc(sizeof(List));
    memset(lPtr, 0, sizeof(List));

    lPtr->widgetClass = WC_List;

    lPtr->view = W_CreateView(W_VIEW(parent));
    if (!lPtr->view) {
	free(lPtr);
	return NULL;
    }
    lPtr->view->self = lPtr;

    lPtr->view->delegate = &_ListViewDelegate;

    WMCreateEventHandler(lPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, lPtr);

    WMCreateEventHandler(lPtr->view, ButtonPressMask|ButtonReleaseMask
			 |EnterWindowMask|LeaveWindowMask|ButtonMotionMask,
			 handleActionEvents, lPtr);

    lPtr->itemHeight = WMFontHeight(scrPtr->normalFont) + 1;
        
    /* create the vertical scroller */
    lPtr->vScroller = WMCreateScroller(lPtr);
    WMMoveWidget(lPtr->vScroller, 1, 1);
    WMSetScrollerArrowsPosition(lPtr->vScroller, WSAMaxEnd);

    WMSetScrollerAction(lPtr->vScroller, vScrollCallBack, lPtr);

    /* make the scroller map itself when it's realized */
    WMMapWidget(lPtr->vScroller);

    W_ResizeView(lPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    lPtr->selectedItem = -1;

    return lPtr;
}



WMListItem*
WMAddSortedListItem(WMList *lPtr, char *text)
{
    WMListItem *item;
    WMListItem *tmp;
    int index;

    item = wmalloc(sizeof(WMListItem));
    memset(item, 0, sizeof(WMListItem));
    item->text = wstrdup(text);
    if (!lPtr->items) {
	lPtr->items = item;
	index = 0;
    } else if (strcmp(lPtr->items->text, text) > 0) {
	item->nextPtr = lPtr->items;
	lPtr->items = item;
	index = 1;
    } else {	
	int added = 0;
	
	index = 0;
	tmp = lPtr->items;
	while (tmp->nextPtr) {
	    if (strcmp(tmp->nextPtr->text, text) >= 0) {
		item->nextPtr = tmp->nextPtr;
		tmp->nextPtr = item;
		added = 1;
		break;
	    }
	    index++;
	    tmp = tmp->nextPtr;
	}
	if (!added) {
	    tmp->nextPtr = item;
	}
    }

    if (index < lPtr->fullFitLines+lPtr->flags.dontFitAll-lPtr->topItem) {
	paintList(lPtr);
    }

    lPtr->itemCount++;
    
    if (lPtr->selectedItem >= index)
	lPtr->selectedItem++;

    /* update the scroller when idle, so that we don't waste time
     * updating it when another item is going to be added later */
    if (!lPtr->idleID) {
	lPtr->idleID = WMAddIdleHandler((WMCallback*)updateScroller, lPtr);
    } 

    return item;
}



WMListItem*
WMInsertListItem(WMList *lPtr, int row, char *text)
{
    WMListItem *item;
    WMListItem *tmp = lPtr->items;

    CHECK_CLASS(lPtr, WC_List);

    item = wmalloc(sizeof(WMListItem));
    memset(item, 0, sizeof(WMListItem));
    item->text = wstrdup(text);


    if (lPtr->selectedItem >= row && lPtr->selectedItem >= 0
	&& row >= 0)
	lPtr->selectedItem++;

    if (lPtr->items==NULL) {
	lPtr->items = item;
    } else if (row == 0) {
	item->nextPtr = lPtr->items;
	lPtr->items = item;
    } else if (row < 0) {
	while (tmp->nextPtr)
	    tmp = tmp->nextPtr;

	tmp->nextPtr = item;
	row = lPtr->itemCount;
    } else {
	while (--row > 0)
	    tmp = tmp->nextPtr;

	item->nextPtr = tmp->nextPtr;
	tmp->nextPtr = item;
    }

    if (row < lPtr->fullFitLines+lPtr->flags.dontFitAll-lPtr->topItem) {
	paintList(lPtr);
    }

    lPtr->itemCount++;

    /* update the scroller when idle, so that we don't waste time
     * updating it when another item is going to be added later */
    if (!lPtr->idleID) {
	lPtr->idleID = WMAddIdleHandler((WMCallback*)updateScroller, lPtr);
    } 

    return item;
}


void
WMRemoveListItem(WMList *lPtr, int row)
{
    WMListItem *llist;
    WMListItem *tmp;
    int topItem = lPtr->topItem;
    int selNotify = 0;

    CHECK_CLASS(lPtr, WC_List);
    
    if (row < 0 || row >= lPtr->itemCount)
	return;

    if (lPtr->selectedItem == row) {
        lPtr->selectedItem = -1;
        selNotify = 1;
    } else if (lPtr->selectedItem > row) {
        lPtr->selectedItem--;
    }
    
    if (row <= lPtr->topItem+lPtr->fullFitLines+lPtr->flags.dontFitAll)
	lPtr->topItem--;
    if (lPtr->topItem < 0)
	lPtr->topItem = 0;

    if (row == 0) {	
	if (lPtr->items->text)
	    free(lPtr->items->text);
	
	tmp = lPtr->items->nextPtr;
	free(lPtr->items);
	
	lPtr->items = tmp;
    } else {
	llist = lPtr->items;
	while (--row > 0)
	    llist = llist->nextPtr;
	tmp = llist->nextPtr;
	llist->nextPtr = llist->nextPtr->nextPtr;

	if (tmp->text)
	    free(tmp->text);

	free(tmp);
    }
    
    lPtr->itemCount--;

    if (!lPtr->idleID) {
	lPtr->idleID = WMAddIdleHandler((WMCallback*)updateScroller, lPtr);
    }
    if (lPtr->topItem != topItem)
    	WMPostNotificationName(WMListDidScrollNotification, lPtr, NULL);
    if (selNotify)
        WMPostNotificationName(WMListSelectionDidChangeNotification, lPtr,
                               (void*)((int)lPtr->selectedItem));
}



WMListItem*
WMGetListItem(WMList *lPtr, int row)
{
    WMListItem *listPtr;

    listPtr = lPtr->items;
    
    while (row-- > 0) 
	listPtr = listPtr->nextPtr;
    
    return listPtr;
}



void
WMSetListUserDrawProc(WMList *lPtr, WMListDrawProc *proc)
{
    lPtr->flags.userDrawn = 1;
    lPtr->draw = proc;
}


void
WMSetListUserDrawItemHeight(WMList *lPtr, unsigned short height)
{
    assert(height > 0);
    
    lPtr->flags.userItemHeight = 1;
    lPtr->itemHeight = height;
    
    updateGeometry(lPtr);
}


void
WMClearList(WMList *lPtr)
{
    WMListItem *item, *tmp;
    int oldSelected = lPtr->selectedItem;

    item = lPtr->items;
    while (item) {
	free(item->text);
	tmp = item->nextPtr;
	free(item);
	item = tmp;
    }
    lPtr->items = NULL;
    lPtr->itemCount = 0;
    lPtr->topItem = 0;
    lPtr->selectedItem = -1;

    if (!lPtr->idleID) {
	WMDeleteIdleHandler(lPtr->idleID);
	lPtr->idleID = NULL;
    }
    if (lPtr->view->flags.realized) {
	updateScroller(lPtr);
    }
    if (oldSelected != -1)
        WMPostNotificationName(WMListSelectionDidChangeNotification, lPtr,
                               (void*)-1);
}


void
WMSetListAction(WMList *lPtr, WMAction *action, void *clientData)
{
    lPtr->action = action;
    lPtr->clientData = clientData;
}


void
WMSetListDoubleAction(WMList *lPtr, WMAction *action, void *clientData)
{
    lPtr->doubleAction = action;
    lPtr->doubleClientData = clientData;
}


WMListItem*
WMGetListSelectedItem(WMList *lPtr)
{    
    int i = lPtr->selectedItem;
    WMListItem *item;

    if (lPtr->selectedItem < 0)
	return NULL;
    
    item = lPtr->items;
    while (i-- > 0) {
	item = item->nextPtr;
    }
    return item;
}


int
WMGetListSelectedItemRow(WMList *lPtr)
{    
    return lPtr->selectedItem;
}


int
WMGetListItemHeight(WMList *lPtr)
{    
    return lPtr->itemHeight;
}


void
WMSetListPosition(WMList *lPtr, int row)
{
    lPtr->topItem = row;
    if (lPtr->topItem + lPtr->fullFitLines > lPtr->itemCount)
	lPtr->topItem = lPtr->itemCount - lPtr->fullFitLines;

    if (lPtr->topItem < 0)
	lPtr->topItem = 0;
    
    if (lPtr->view->flags.realized)
	updateScroller(lPtr);
}


void
WMSetListBottomPosition(WMList *lPtr, int row)
{
    if (lPtr->itemCount > lPtr->fullFitLines) {
	lPtr->topItem = row - lPtr->fullFitLines;
	if (lPtr->topItem < 0)
	    lPtr->topItem = 0;
	if (lPtr->view->flags.realized)
	    updateScroller(lPtr);
    }
}


int
WMGetListNumberOfRows(WMList *lPtr)
{
    return lPtr->itemCount;
}

int
WMGetListPosition(WMList *lPtr)
{
    return lPtr->topItem;
}


static void
vScrollCallBack(WMWidget *scroller, void *self)
{
    WMList *lPtr = (WMList*)self;
    WMScroller *sPtr = (WMScroller*)scroller;
    int height;
    int topItem = lPtr->topItem;
    
    height = lPtr->view->size.height - 4;

    switch (WMGetScrollerHitPart(sPtr)) {
     case WSDecrementLine:
	if (lPtr->topItem > 0) {
	    lPtr->topItem--;

	    updateScroller(lPtr);
	}
	break;
	
     case WSDecrementPage:
	if (lPtr->topItem > 0) {
	    lPtr->topItem -= lPtr->fullFitLines-(1-lPtr->flags.dontFitAll)-1;
	    if (lPtr->topItem < 0)
		lPtr->topItem = 0;

	    updateScroller(lPtr);
	}
	break;

	
     case WSIncrementLine:
	if (lPtr->topItem + lPtr->fullFitLines < lPtr->itemCount) {
	    lPtr->topItem++;
	    
	    updateScroller(lPtr);
	}
	break;
	
     case WSIncrementPage:
	if (lPtr->topItem + lPtr->fullFitLines < lPtr->itemCount) {
	    lPtr->topItem += lPtr->fullFitLines-(1-lPtr->flags.dontFitAll)-1;

	    if (lPtr->topItem + lPtr->fullFitLines > lPtr->itemCount)
		lPtr->topItem = lPtr->itemCount - lPtr->fullFitLines;

	    updateScroller(lPtr);
	}
	break;
	
     case WSKnob:
	{
	    int oldTopItem = lPtr->topItem;
	    
	    lPtr->topItem = WMGetScrollerValue(lPtr->vScroller) * 
		(float)(lPtr->itemCount - lPtr->fullFitLines);

	    if (oldTopItem != lPtr->topItem)
		paintList(lPtr);
	}
	break;

     case WSKnobSlot:
     case WSNoPart:
	/* do nothing */
	break;
    }

    if (lPtr->topItem != topItem)
	WMPostNotificationName(WMListDidScrollNotification, lPtr, NULL);
}


static void
paintItem(List *lPtr, int index)
{
    WMView *view = lPtr->view;
    W_Screen *scr = view->screen;
    int width, height, x, y;
    WMListItem *itemPtr;
    int i;

    i = index;
    itemPtr = lPtr->items;
    while (i-- > 0)
	itemPtr = itemPtr->nextPtr;

    width = lPtr->view->size.width - 2 - 19;
    height = lPtr->itemHeight;
    x = 19;
    y = 2 + (index-lPtr->topItem) * lPtr->itemHeight + 1;

    if (lPtr->flags.userDrawn) {
	WMRect rect;
	int flags;

	rect.size.width = width;
	rect.size.height = height;
	rect.pos.x = x;
	rect.pos.y = y;

	flags = itemPtr->uflags;
	if (itemPtr->disabled)
	    flags |= WLDSDisabled;
	if (itemPtr->selected)
	    flags |= WLDSSelected;
	if (itemPtr->isBranch)
	    flags |= WLDSIsBranch;

	if (lPtr->draw)
	    (*lPtr->draw)(lPtr, index, view->window, itemPtr->text, flags, 
			  &rect);
    } else {
	if (itemPtr->selected)
	    XFillRectangle(scr->display, view->window, WMColorGC(scr->white), x, y,
			   width, height);
	else
	    XClearArea(scr->display, view->window, x, y, width, height, False);
	
	W_PaintText(view, view->window, scr->normalFont,  x+4, y, width,
		    WALeft, WMColorGC(scr->black), False, 
		    itemPtr->text, strlen(itemPtr->text));
    }
}



static void
paintList(List *lPtr)
{
    W_Screen *scrPtr = lPtr->view->screen;
    int i, lim;

    if (!lPtr->view->flags.mapped)
	return;

    if (lPtr->itemCount>0) {
	if (lPtr->topItem+lPtr->fullFitLines+lPtr->flags.dontFitAll > lPtr->itemCount) {
	    lim = lPtr->itemCount - lPtr->topItem;
	    XClearArea(scrPtr->display, lPtr->view->window, 19,
		       2+lim*lPtr->itemHeight, lPtr->view->size.width-21,
		       lPtr->view->size.height-lim*lPtr->itemHeight-3, False);
	} else {
	    lim = lPtr->fullFitLines + lPtr->flags.dontFitAll;
	}
	for (i = lPtr->topItem; i < lPtr->topItem + lim; i++) {
	    paintItem(lPtr, i);
	}
    } else {
	XClearWindow(scrPtr->display, lPtr->view->window);
    }
    W_DrawRelief(scrPtr, lPtr->view->window, 0, 0, lPtr->view->size.width,
		 lPtr->view->size.height, WRSunken);
}

#if 0
static void
scrollTo(List *lPtr, int newTop)
{
    
}
#endif

static void
updateScroller(List *lPtr)
{
    float knobProportion, floatValue, tmp;

    if (lPtr->idleID)
 	WMDeleteIdleHandler(lPtr->idleID);
    lPtr->idleID = NULL;
        
    paintList(lPtr);
    
    if (lPtr->itemCount == 0 || lPtr->itemCount <= lPtr->fullFitLines)
	WMSetScrollerParameters(lPtr->vScroller, 0, 1);
    else {
	tmp = lPtr->fullFitLines;
	knobProportion = tmp/(float)lPtr->itemCount;
    
	floatValue = (float)lPtr->topItem/(float)(lPtr->itemCount - lPtr->fullFitLines);
    
	WMSetScrollerParameters(lPtr->vScroller, floatValue, knobProportion);
    }
}


static void
handleEvents(XEvent *event, void *data)
{
    List *lPtr = (List*)data;

    CHECK_CLASS(data, WC_List);


    switch (event->type) {	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintList(lPtr);
	break;
	
     case DestroyNotify:
	destroyList(lPtr);
	break;
	
    }
}


int
WMFindRowOfListItemWithTitle(WMList *lPtr, char *title)
{
    WMListItem *item;
    int i;
    int ok = 0;
    
    for (i=0, item=lPtr->items; item!=NULL; item=item->nextPtr, i++) {
	if (strcmp(item->text, title)==0) {
	    ok = 1;
	    break;
	}
    }

    return ok ? i : -1;
}


void
WMSelectListItem(WMList *lPtr, int row)
{
    WMListItem *itemPtr;
    int i, notify = 0;

    if (row >= lPtr->itemCount)
	return;

    /* the check below must be changed when the multiple selection is
     * implemented. -Dan
     */
    if (!lPtr->flags.allowMultipleSelection && row == lPtr->selectedItem)
        notify = 0;
    else
        notify = 1;

    if (!lPtr->flags.allowMultipleSelection) {
	/* unselect previous selected item */
	if (lPtr->selectedItem >= 0) {
	    itemPtr = lPtr->items;
	    for (i=0; i<lPtr->selectedItem; i++)
		itemPtr = itemPtr->nextPtr;
	    
	    if (itemPtr->selected) {
		itemPtr->selected = 0;
		if (lPtr->view->flags.mapped && i>=lPtr->topItem 
		    && i<=lPtr->topItem+lPtr->fullFitLines)
		    paintItem(lPtr, i);
	    }
	}
    }
    
    if (row < 0) {
        if (!lPtr->flags.allowMultipleSelection) {
	    lPtr->selectedItem = -1;
            if (notify)
                WMPostNotificationName(WMListSelectionDidChangeNotification,
                                       lPtr, (void*)((int)lPtr->selectedItem));
        }
	return;
    }

    /* select item */
    itemPtr = lPtr->items;
    for (i=0; i<row; i++)
	itemPtr = itemPtr->nextPtr;
    if (lPtr->flags.allowMultipleSelection)
	itemPtr->selected = !itemPtr->selected;
    else
	itemPtr->selected = 1;

    if (lPtr->view->flags.mapped) {
	paintItem(lPtr, row);

	if ((row-lPtr->topItem+lPtr->fullFitLines)*lPtr->itemHeight
	    > lPtr->view->size.height-2)
	    W_DrawRelief(lPtr->view->screen, lPtr->view->window, 0, 0,
			 lPtr->view->size.width, lPtr->view->size.height,
			 WRSunken);
    }
    lPtr->selectedItem = row;
    if (notify)
        WMPostNotificationName(WMListSelectionDidChangeNotification, lPtr,
                               (void*)((int)lPtr->selectedItem));
}


static int
getItemIndexAt(List *lPtr, int clickY)
{
    int index;

    index = (clickY - 2) / lPtr->itemHeight + lPtr->topItem;

    if (index < 0 || index >= lPtr->itemCount)
	return -1;

    return index;
}


static void
handleActionEvents(XEvent *event, void *data)
{
    List *lPtr = (List*)data;
    int tmp;
    int topItem = lPtr->topItem;

    CHECK_CLASS(data, WC_List);

    switch (event->type) {
     case ButtonRelease:
	lPtr->flags.buttonPressed = 0;
	tmp = getItemIndexAt(lPtr, event->xbutton.y);

	if (tmp == lPtr->selectedItem && tmp >= 0) {
	    if (lPtr->action)
		(*lPtr->action)(lPtr, lPtr->clientData);
	}
	break;

     case EnterNotify:
	lPtr->flags.buttonPressed = lPtr->flags.buttonWasPressed;
	lPtr->flags.buttonWasPressed = 0;
	break;

     case LeaveNotify:
	lPtr->flags.buttonWasPressed = lPtr->flags.buttonPressed;
	lPtr->flags.buttonPressed = 0;
	break;

     case ButtonPress:
	if (event->xbutton.x > WMWidgetWidth(lPtr->vScroller)) {
	    tmp = getItemIndexAt(lPtr, event->xbutton.y);
	    lPtr->flags.buttonPressed = 1;

	    if (tmp >= 0) {
		if (tmp == lPtr->selectedItem && WMIsDoubleClick(event)) {
		    WMSelectListItem(lPtr, tmp);
		    if (lPtr->doubleAction)
			(*lPtr->doubleAction)(lPtr, lPtr->doubleClientData);
		} else {
		    WMSelectListItem(lPtr, tmp);
		}
	    }
	}
	break;
	
     case MotionNotify:
	if (lPtr->flags.buttonPressed) {
	    tmp = getItemIndexAt(lPtr, event->xmotion.y);
	    if (tmp>=0 && tmp != lPtr->selectedItem) {
		WMSelectListItem(lPtr, tmp);
	    }
	}
	break;
    }
    if (lPtr->topItem != topItem)
    	WMPostNotificationName(WMListDidScrollNotification, lPtr, NULL);
}


static void
updateGeometry(WMList *lPtr)
{    
    lPtr->fullFitLines = (lPtr->view->size.height - 4) / lPtr->itemHeight;
    if (lPtr->fullFitLines * lPtr->itemHeight < lPtr->view->size.height - 4) {
	lPtr->flags.dontFitAll = 1;
    } else {
	lPtr->flags.dontFitAll = 0;
    }

    if (lPtr->itemCount - lPtr->topItem <= lPtr->fullFitLines) {
	lPtr->topItem = lPtr->itemCount - lPtr->fullFitLines;
	if (lPtr->topItem < 0)
	    lPtr->topItem = 0;
    }

    updateScroller(lPtr);
}


static void
didResizeList(W_ViewDelegate *self, WMView *view)
{    
    WMList *lPtr = (WMList*)view->self;

    WMResizeWidget(lPtr->vScroller, 1, view->size.height-2);

    updateGeometry(lPtr);
}


static void
destroyList(List *lPtr)
{
    WMListItem *itemPtr;
    
    if (lPtr->idleID)
 	WMDeleteIdleHandler(lPtr->idleID);
    lPtr->idleID = NULL;

    while (lPtr->items!=NULL) {
	itemPtr = lPtr->items;
	if (itemPtr->text)
	    free(itemPtr->text);
	
	lPtr->items = itemPtr->nextPtr;
	free(itemPtr);
    }
    
    free(lPtr);
}
