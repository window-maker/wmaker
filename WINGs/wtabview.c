
#include "WINGsP.h"


typedef struct W_TabView {
    W_Class widgetClass;
    W_View *view;

    struct W_TabViewItem **items;
    int itemCount;
    int maxItems;	       /* size of items array, can be increased */

    int selectedItem;
    int firstVisible;
    
    int visibleTabs;

    WMFont *font;

    WMColor *lightGray;
    WMColor *tabColor;

    WMTabViewDelegate *delegate;

    short tabWidth;
    short tabHeight;

    struct {
	WMReliefType relief:4;
	WMTitlePosition titlePosition:4;
	WMTabViewType type:2;

	unsigned tabbed:1;
	unsigned dontFitAll:1;
    } flags;
} TabView;



#define DEFAULT_WIDTH	40
#define DEFAULT_HEIGHT	40

#define NORMAL_SIDE_OFFSET 8
#define BUTTONED_SIDE_OFFSET 20


static void destroyTabView(TabView *tPtr);	
static void paintTabView(TabView *tPtr);


static void W_SetTabViewItemParent(WMTabViewItem *item, WMTabView *parent);

static void W_DrawLabel(WMTabViewItem *item, Drawable d, WMRect rect);

static void W_UnmapTabViewItem(WMTabViewItem *item);

static void W_MapTabViewItem(WMTabViewItem *item);

static WMView *W_TabViewItemView(WMTabViewItem *item);

static void recalcTabWidth(TabView *tPtr);


static void
handleEvents(XEvent *event, void *data)
{
    TabView *tPtr = (TabView*)data;

    CHECK_CLASS(data, WC_TabView);

    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintTabView(tPtr);
	break;

     case ButtonPress:
	{
	    WMTabViewItem *item = WMTabViewItemAtPoint(tPtr,
						       event->xbutton.x,
						       event->xbutton.y);
	    if (item) {
		WMSelectTabViewItem(tPtr, item);
	    } else if (tPtr->flags.dontFitAll) {
		int redraw;
		if (event->xbutton.x < BUTTONED_SIDE_OFFSET) {
		    if (tPtr->firstVisible > 0) {
			redraw = 1;
			tPtr->firstVisible--;
		    }
		} else if (event->xbutton.x - BUTTONED_SIDE_OFFSET
			   > tPtr->visibleTabs*(tPtr->tabWidth-10)) {

		    if (tPtr->firstVisible + tPtr->visibleTabs
			< tPtr->itemCount) {
			redraw = 1;
			tPtr->firstVisible++;
		    }
		}

		if (redraw) {
		    paintTabView(tPtr);
		}
	    }
	}
	break;

     case DestroyNotify:
	destroyTabView(tPtr);
	break;
    }
}



WMTabView*
WMCreateTabView(WMWidget *parent)
{
    TabView *tPtr;
    WMScreen *scr = WMWidgetScreen(parent);

    tPtr = wmalloc(sizeof(TabView));
    memset(tPtr, 0, sizeof(TabView));

    tPtr->widgetClass = WC_TabView;

    tPtr->view = W_CreateView(W_VIEW(parent));
    if (!tPtr->view) {
	wfree(tPtr);
	return NULL;
    }
    tPtr->view->self = tPtr;

    tPtr->lightGray = WMCreateRGBColor(scr, 0xd9d9, 0xd9d9, 0xd9d9, False);
    tPtr->tabColor = WMCreateRGBColor(scr, 0x8420, 0x8420, 0x8420, False);

    tPtr->font = WMRetainFont(scr->normalFont);

    WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
			 |ButtonPressMask, handleEvents, tPtr);

    WMResizeWidget(tPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    tPtr->tabHeight = WMFontHeight(tPtr->font) + 3;

    return tPtr;
}


void
WMSetTabViewDelegate(WMTabView *tPtr, WMTabViewDelegate *delegate)
{
    tPtr->delegate = delegate;
}


WMTabViewItem*
WMAddTabViewItemWithView(WMTabView *tPtr, WMView *view, int identifier,
			 char *label)
{
    WMTabViewItem *item;
    
    item = WMCreateTabViewItemWithIdentifier(identifier);
    WMSetTabViewItemView(item, view);
    WMAddItemInTabView(tPtr, item);
    WMSetTabViewItemLabel(item, label);

    return item;
}


void
WMAddItemInTabView(WMTabView *tPtr, WMTabViewItem *item)
{
    WMInsertItemInTabView(tPtr, tPtr->itemCount, item);
}


void
WMInsertItemInTabView(WMTabView *tPtr, int index, WMTabViewItem *item)
{
    wassertr(W_TabViewItemView(item) != NULL);

    if (tPtr->maxItems == tPtr->itemCount) {
	WMTabViewItem **items;

	items = wrealloc(tPtr->items,
			 sizeof(WMTabViewItem*) * (tPtr->maxItems + 10));
	memset(&items[tPtr->maxItems], 0, sizeof(WMTabViewItem*) * 10);
	tPtr->items = items;
	tPtr->maxItems += 10;
    }

    if (index > tPtr->itemCount)
	index = tPtr->itemCount;

    if (index == 0 && tPtr->items[0]) {
	W_UnmapTabViewItem(tPtr->items[0]);
    }

    if (index < tPtr->itemCount) {
	memmove(&tPtr->items[index + 1], &tPtr->items[index],
		tPtr->itemCount - index);
    }

    tPtr->items[index] = item;

    tPtr->itemCount++;

    recalcTabWidth(tPtr);

    W_SetTabViewItemParent(item, tPtr);

    W_UnmapTabViewItem(item);

    W_ReparentView(W_TabViewItemView(item), tPtr->view, 1, 
		   tPtr->tabHeight + 1);

    W_ResizeView(W_TabViewItemView(item), tPtr->view->size.width - 3,
		 tPtr->view->size.height - tPtr->tabHeight - 3);

    if (index == 0) {
	W_MapTabViewItem(item);
    }
    if (tPtr->delegate && tPtr->delegate->didChangeNumberOfItems)
	(*tPtr->delegate->didChangeNumberOfItems)(tPtr->delegate, tPtr);
}


void
WMRemoveTabViewItem(WMTabView *tPtr, WMTabViewItem *item)
{
    int i;

    for (i = 0; i < tPtr->itemCount; i++) {
	if (tPtr->items[i] == item) {
	    if (i < tPtr->itemCount - 1)
		memmove(&tPtr->items[i], &tPtr->items[i + 1],
			tPtr->itemCount - i - 1);
	    else
		tPtr->items[i] = NULL;

	    W_SetTabViewItemParent(item, NULL);

	    tPtr->itemCount--;
	    break;
	}
    }
    if (tPtr->delegate && tPtr->delegate->didChangeNumberOfItems)
	(*tPtr->delegate->didChangeNumberOfItems)(tPtr->delegate, tPtr);
}



static Bool
isInside(int x, int y, int width, int height, int px, int py)
{
    if (py >= y + height - 3 && py <= y + height
	&& px >= x + py - (y + height - 3)
	&& px <= x + width - (py - (y + height - 3))) {

	return True;
    }
    if (py >= y + 3 && py < y + height - 3
	&& px >= x + 3 + ((y + 3) - py)*3/7
	&& px <= x + width - 3 - ((y + 3) - py)*3/7) {

	return True;
    }
    if (py >= y && py < y + 3
	&& px >= x + 7 + py - y
	&& px <= x + width - 7 - (py - y)) {

	return True;
    }
    return False;
}


WMTabViewItem*
WMTabViewItemAtPoint(WMTabView *tPtr, int x, int y)
{
    int i;
    int offset;
    int count = tPtr->visibleTabs;
    int first = tPtr->firstVisible;
    
    if (tPtr->flags.dontFitAll) {
	offset = BUTTONED_SIDE_OFFSET;

	i = tPtr->selectedItem - tPtr->firstVisible;
	if (i >= 0 && i < tPtr->visibleTabs
	    && isInside(offset + (tPtr->tabWidth-10)*i, 0, tPtr->tabWidth,
			tPtr->tabHeight, x, y)) {
	    return tPtr->items[tPtr->selectedItem];
	}
    } else {
	offset = NORMAL_SIDE_OFFSET;

	i = tPtr->selectedItem;
	if (isInside(offset + (tPtr->tabWidth-10)*i, 0, tPtr->tabWidth,
		     tPtr->tabHeight, x, y)) {
	    return tPtr->items[i];
	}
    }

    for (i = 0; i < count; i++) {
	if (isInside(offset + (tPtr->tabWidth-10)*i, 0, tPtr->tabWidth,
		     tPtr->tabHeight, x, y)) {
	    return tPtr->items[i+first];
	}
    }
    return NULL;
}


void
WMSelectFirstTabViewItem(WMTabView *tPtr)
{
    WMSelectTabViewItemAtIndex(tPtr, 0);
}


void
WMSelectLastTabViewItem(WMTabView *tPtr)
{
    WMSelectTabViewItemAtIndex(tPtr, tPtr->itemCount);
}


void
WMSelectNextTabViewItem(WMTabView *tPtr)
{
    WMSelectTabViewItemAtIndex(tPtr, tPtr->selectedItem + 1);
}


void
WMSelectPreviousTabViewItem(WMTabView *tPtr)
{
    WMSelectTabViewItemAtIndex(tPtr, tPtr->selectedItem - 1);
}


WMTabViewItem*
WMGetSelectedTabViewItem(WMTabView *tPtr)
{
    return tPtr->items[tPtr->selectedItem];
}


void
WMSelectTabViewItem(WMTabView *tPtr, WMTabViewItem *item)
{
    int i;

    for (i = 0; i < tPtr->itemCount; i++) {
	if (tPtr->items[i] == item) {
	    WMSelectTabViewItemAtIndex(tPtr, i);
	    break;
	}
    }
}


void
WMSelectTabViewItemAtIndex(WMTabView *tPtr, int index)
{
    WMTabViewItem *item;

    if (index == tPtr->selectedItem)
	return;

    if (index < 0)
	index = 0;
    else if (index >= tPtr->itemCount)
	index = tPtr->itemCount - 1;

    item = tPtr->items[tPtr->selectedItem];

    if (tPtr->delegate && tPtr->delegate->shouldSelectItem)
	if (!(*tPtr->delegate->shouldSelectItem)(tPtr->delegate, tPtr, 
						 tPtr->items[index]))
	    return;

    if (tPtr->delegate && tPtr->delegate->willSelectItem)
	(*tPtr->delegate->willSelectItem)(tPtr->delegate, tPtr, 
					  tPtr->items[index]);

    W_UnmapTabViewItem(item);


    item = tPtr->items[index];

    W_MapTabViewItem(item);

    tPtr->selectedItem = index;

    if (tPtr->delegate && tPtr->delegate->didSelectItem)
	(*tPtr->delegate->didSelectItem)(tPtr->delegate, tPtr, 
					 tPtr->items[index]);
}


static void
recalcTabWidth(TabView *tPtr)
{
    int i;
    int twidth = W_VIEW(tPtr)->size.width;
    int width;

    tPtr->tabWidth = 0;
    for (i = 0; i < tPtr->itemCount; i++) {
	char *str = WMGetTabViewItemLabel(tPtr->items[i]);

	if (str) {
	    width = WMWidthOfString(tPtr->font, str, strlen(str));
	    if (width > tPtr->tabWidth)
		tPtr->tabWidth = width;
	}
    }
    tPtr->tabWidth += 30;
    if ((tPtr->tabWidth + 2) * tPtr->itemCount > twidth - 2*NORMAL_SIDE_OFFSET) {
	tPtr->flags.dontFitAll = 1;
	tPtr->firstVisible = 0;
	tPtr->visibleTabs = (twidth - 2*BUTTONED_SIDE_OFFSET) / (tPtr->tabWidth-10);
    } else {
	tPtr->flags.dontFitAll = 0;
	tPtr->firstVisible = 0;
	tPtr->visibleTabs = tPtr->itemCount;
    }
}


static void
drawRelief(W_Screen *scr, Drawable d, int x, int y, unsigned int width,
	   unsigned int height)
{
    Display *dpy = scr->display;
    GC bgc = WMColorGC(scr->black);
    GC wgc = WMColorGC(scr->white);
    GC dgc = WMColorGC(scr->darkGray);

    XDrawLine(dpy, d, wgc, x, y, x, y+height-1);

    XDrawLine(dpy, d, bgc, x, y+height-1, x+width-1, y+height-1);
    XDrawLine(dpy, d, dgc, x+1, y+height-2, x+width-2, y+height-2);

    XDrawLine(dpy, d, bgc, x+width-1, y, x+width-1, y+height-1);
    XDrawLine(dpy, d, dgc, x+width-2, y+1, x+width-2, y+height-2);
}


static void
drawTab(TabView *tPtr, Drawable d, int x, int y, 
	unsigned width, unsigned height, Bool selected)
{
    WMScreen *scr = W_VIEW(tPtr)->screen;
    Display *dpy = scr->display;
    GC white = WMColorGC(selected ? scr->white : tPtr->lightGray);
    GC black = WMColorGC(scr->black);
    GC dark = WMColorGC(scr->darkGray);
    GC light = WMColorGC(scr->gray);
    XPoint trap[8];

    trap[0].x = x + (selected ? 0 : 1);
    trap[0].y = y + height - (selected ? 0 : 1);

    trap[1].x = x + 3;
    trap[1].y = y + height - 3;

    trap[2].x = x + 10 - 3;
    trap[2].y = y + 3;

    trap[3].x = x + 10;
    trap[3].y = y;

    trap[4].x = x + width - 10;
    trap[4].y = y;

    trap[5].x = x + width - 10 + 3;
    trap[5].y = y + 3;

    trap[6].x = x + width - 3;
    trap[6].y = y + height - 3;

    trap[7].x = x + width - (selected ? 0 : 1);
    trap[7].y = y + height - (selected ? 0 : 1);

    XFillPolygon(dpy, d, selected ? light : WMColorGC(tPtr->tabColor), trap, 8,
		 Convex, CoordModeOrigin);

    XDrawLine(dpy, d, white, trap[0].x, trap[0].y, trap[1].x, trap[1].y);
    XDrawLine(dpy, d, white, trap[1].x, trap[1].y, trap[2].x, trap[2].y);
    XDrawLine(dpy, d, white, trap[2].x, trap[2].y, trap[3].x, trap[3].y);
    XDrawLine(dpy, d, white, trap[3].x, trap[3].y, trap[4].x, trap[4].y);
    XDrawLine(dpy, d, dark, trap[4].x, trap[4].y, trap[5].x, trap[5].y);
    XDrawLine(dpy, d, black, trap[5].x, trap[5].y, trap[6].x, trap[6].y);
    XDrawLine(dpy, d, black, trap[6].x, trap[6].y, trap[7].x, trap[7].y);

    XDrawLine(dpy, d, selected ? light : WMColorGC(scr->white), 
	      trap[0].x, trap[0].y, trap[7].x, trap[7].y);
}


static void
paintDot(TabView *tPtr, Drawable d, int x, int y)
{
    WMScreen *scr = W_VIEW(tPtr)->screen;
    Display *dpy = scr->display;
    GC white = WMColorGC(scr->white);
    GC black = WMColorGC(scr->black);

    XFillRectangle(dpy, d, black, x, y, 2, 2);
    XDrawPoint(dpy, d, white, x, y);
}



static void
paintTabView(TabView *tPtr)
{
    Pixmap buffer;
    WMScreen *scr = W_VIEW(tPtr)->screen;
    Display *dpy = scr->display;
    GC white = WMColorGC(scr->white);
    int i;
    WMRect rect;

    if (tPtr->flags.type == WTTopTabsBevelBorder) {
	int count = tPtr->visibleTabs;
	int first = tPtr->firstVisible;
	int offs;
	int moreAtLeft;
	int moreAtRight;
	int selectedIsVisible;
	
	buffer = XCreatePixmap(dpy, W_VIEW(tPtr)->window,
			       W_VIEW(tPtr)->size.width, tPtr->tabHeight,
			       W_VIEW(tPtr)->screen->depth);

	XFillRectangle(dpy, buffer, WMColorGC(W_VIEW(tPtr)->backColor),
		       0, 0, W_VIEW(tPtr)->size.width, tPtr->tabHeight);

	rect.pos.y = 2;
	if (tPtr->flags.dontFitAll) {
	    rect.pos.x = 15 + BUTTONED_SIDE_OFFSET;
	    offs = BUTTONED_SIDE_OFFSET;
	    moreAtLeft = first > 0;
	    moreAtRight = (first + count) < tPtr->itemCount;
	    if (tPtr->selectedItem >= first
		&& tPtr->selectedItem < first + count)
		selectedIsVisible = 1;
	    else
		selectedIsVisible = 0;
	} else {
	    rect.pos.x = 15 + NORMAL_SIDE_OFFSET;
	    offs = NORMAL_SIDE_OFFSET;
	    moreAtLeft = 0;
	    moreAtRight = 0;
	    selectedIsVisible = 1;
	}
	rect.size.width = tPtr->tabWidth;
	rect.size.height = tPtr->tabHeight;

	for (i = count - (moreAtRight ? 0 : 1);
	     i >= (moreAtLeft ? -1 : 0); i--) {
	    if (!selectedIsVisible || i != (tPtr->selectedItem-first)) {
		drawTab(tPtr, buffer, offs + (rect.size.width-10)*i, 0,
			rect.size.width, rect.size.height, False);
	    }
	}
	
	if (selectedIsVisible) {
	    drawTab(tPtr, buffer, 
		    offs + (rect.size.width-10) * (tPtr->selectedItem - first),
		    0, rect.size.width, rect.size.height, True);

	    XDrawLine(dpy, buffer, white, 0, tPtr->tabHeight - 1,
		      offs, tPtr->tabHeight - 1);

	    XDrawLine(dpy, buffer, white, 
		      offs + 10 + (rect.size.width-10) * count,
		      tPtr->tabHeight - 1, W_VIEW(tPtr)->size.width - 1,
		      tPtr->tabHeight - 1);
	} else {
	    XDrawLine(dpy, buffer, white, 0, tPtr->tabHeight - 1,
		      W_VIEW(tPtr)->size.width, tPtr->tabHeight - 1);	    
	}

	for (i = 0; i < count; i++) {
	    W_DrawLabel(tPtr->items[first+i], buffer, rect);

	    rect.pos.x += rect.size.width - 10;
	}

	if (moreAtLeft) {
	    paintDot(tPtr, buffer, 4, 10);
	    paintDot(tPtr, buffer, 7, 10);
	    paintDot(tPtr, buffer, 10, 10);
	}
	if (moreAtRight) {
	    int x;
	    
	    x = BUTTONED_SIDE_OFFSET - 5 + tPtr->visibleTabs * (tPtr->tabWidth - 10);
	    
	    x = x + (W_VIEW(tPtr)->size.width - x)/2;
	    
	    paintDot(tPtr, buffer, x + 5, 10);
	    paintDot(tPtr, buffer, x + 8, 10);
	    paintDot(tPtr, buffer, x + 11, 10);	    
	}

	XCopyArea(dpy, buffer, W_VIEW(tPtr)->window, scr->copyGC, 0, 0,
		  W_VIEW(tPtr)->size.width, tPtr->tabHeight, 0, 0);

	XFreePixmap(dpy, buffer);
    }
    switch (tPtr->flags.type) {
     case WTTopTabsBevelBorder:
	drawRelief(scr, W_VIEW(tPtr)->window, 0, tPtr->tabHeight - 1,
		   W_VIEW(tPtr)->size.width,
		   W_VIEW(tPtr)->size.height - tPtr->tabHeight + 1);
	break;

     case WTNoTabsBevelBorder:
	W_DrawRelief(scr, W_VIEW(tPtr)->window, 0, 0, W_VIEW(tPtr)->size.width,
		     W_VIEW(tPtr)->size.height, WRRaised);
	break;

     case WTNoTabsLineBorder:
	W_DrawRelief(scr, W_VIEW(tPtr)->window, 0, 0, W_VIEW(tPtr)->size.width,
		     W_VIEW(tPtr)->size.height, WRSimple);
	break;

     case WTNoTabsNoBorder:
	break;
    }
}


static void
destroyTabView(TabView *tPtr)
{
    int i;

    for (i = 0; i < tPtr->itemCount; i++) {
	WMSetTabViewItemView(tPtr->items[i], NULL);
	WMDestroyTabViewItem(tPtr->items[i]);
    }
    wfree(tPtr->items);

    WMReleaseColor(tPtr->lightGray);
    WMReleaseColor(tPtr->tabColor);
    WMReleaseFont(tPtr->font);

    wfree(tPtr);
}

/******************************************************************/


typedef struct W_TabViewItem {
    WMTabView *tabView;

    W_View *view;

    char *label;

    int identifier;

    struct {
	unsigned visible:1;
    } flags;
} TabViewItem;


static void
W_SetTabViewItemParent(WMTabViewItem *item, WMTabView *parent)
{
    item->tabView = parent;
}


static void
W_DrawLabel(WMTabViewItem *item, Drawable d, WMRect rect)
{
    WMScreen *scr = W_VIEW(item->tabView)->screen;

    if (!item->label)
	return;

    WMDrawString(scr, d, WMColorGC(scr->black), item->tabView->font,
		 rect.pos.x, rect.pos.y, item->label, strlen(item->label));
}


static void
W_UnmapTabViewItem(WMTabViewItem *item)
{
    wassertr(item->view);

    W_UnmapView(item->view);

    item->flags.visible = 0;
}


static void
W_MapTabViewItem(WMTabViewItem *item)
{
    wassertr(item->view);

    W_MapView(item->view);

    item->flags.visible = 1;
}


static WMView*
W_TabViewItemView(WMTabViewItem *item)
{
    return item->view;
}


WMTabViewItem*
WMCreateTabViewItemWithIdentifier(int identifier)
{
    WMTabViewItem *item;

    item = wmalloc(sizeof(WMTabViewItem));
    memset(item, 0, sizeof(WMTabViewItem));

    item->identifier = identifier;

    return item;
}


WMTabViewItem*
WMCreateTabViewItem(int identifier, char *label)
{
    WMTabViewItem *item;

    item = wmalloc(sizeof(WMTabViewItem));
    memset(item, 0, sizeof(WMTabViewItem));

    item->identifier = identifier;
    WMSetTabViewItemLabel(item, label);

    return item;
}



int
WMGetTabViewItemIdentifier(WMTabViewItem *item)
{
    return item->identifier;
}


void
WMSetTabViewFont(WMTabView *tPtr, WMFont *font)
{
    if (tPtr->font)
	WMReleaseFont(tPtr->font);

    tPtr->font = WMRetainFont(font);
    tPtr->tabHeight = WMFontHeight(tPtr->font) + 3;
    recalcTabWidth(tPtr);
}


void
WMSetTabViewItemLabel(WMTabViewItem *item, char *label)
{
    if (item->label)
	wfree(item->label);

    item->label = wstrdup(label);

    if (item->tabView)
	recalcTabWidth(item->tabView);
}


char*
WMGetTabViewItemLabel(WMTabViewItem *item)
{
    return item->label;
}


void
WMSetTabViewItemView(WMTabViewItem *item, WMView *view)
{
    item->view = view;
}


WMView*
WMGetTabViewItemView(WMTabViewItem *item)
{
    return item->view;
}


void
WMDestroyTabViewItem(WMTabViewItem *item)
{
    if (item->label)
	wfree(item->label);

    if (item->view)
	W_DestroyView(item->view);

    wfree(item);
}
