

#include "WINGsP.h"


typedef struct {
    WMView *view;
    int minSize;
    int maxSize;
    int space;
    unsigned expand:1;
    unsigned fill:1;
    unsigned end:1;
} SubviewItem;


typedef struct W_Box {
    W_Class widgetClass;
    W_View *view;

    SubviewItem *subviews;
    int subviewCount;

    short borderWidth;

    unsigned horizontal:1;
} Box;


#define DEFAULT_WIDTH		40
#define DEFAULT_HEIGHT		40



static void destroyBox(Box *bPtr);

static void handleEvents(XEvent *event, void *data);

static void didResize(struct W_ViewDelegate*, WMView*);

static W_ViewDelegate delegate = {
    NULL,
	NULL,
	didResize,
	NULL,
	NULL
};




WMBox*
WMCreateBox(WMWidget *parent)
{
    Box *bPtr;
    
    bPtr = wmalloc(sizeof(Box));
    memset(bPtr, 0, sizeof(Box));

    bPtr->widgetClass = WC_Box;

    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	wfree(bPtr);
	return NULL;
    }
    bPtr->view->self = bPtr;
    
    bPtr->view->delegate = &delegate;
    
    WMCreateEventHandler(bPtr->view, StructureNotifyMask,
			 handleEvents, bPtr);

    WMResizeWidget(bPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    bPtr->subviews = NULL;
    bPtr->subviewCount = 0;
    
    return bPtr;
}


static void
rearrange(WMBox *box)
{
    int i;
    int x, y;
    int xe, ye;
    int w = 1, h = 1;
    int total;
    int expands = 0;
    
    x = box->borderWidth;
    y = box->borderWidth;
    
    if (box->horizontal) {
	ye = box->borderWidth;
	xe = WMWidgetWidth(box) - box->borderWidth;
	h = WMWidgetHeight(box) - 2 * box->borderWidth;
	total = WMWidgetWidth(box) - 2 * box->borderWidth;
    } else {
	xe = box->borderWidth;
	ye = WMWidgetHeight(box) - box->borderWidth;	
	w = WMWidgetWidth(box) - 2 * box->borderWidth;
	total = WMWidgetHeight(box) - 2 * box->borderWidth;
    }

    if (w <= 0 || h <= 0 || total <= 0) {
	return;
    }	

    for (i = 0; i < box->subviewCount; i++) {
	total -= box->subviews[i].minSize;
	total -= box->subviews[i].space;
	if (box->subviews[i].expand) {
	    expands++;
	}
    }

    for (i = 0; i < box->subviewCount; i++) {
	if (box->horizontal) {
	    w = box->subviews[i].minSize;
	    if (box->subviews[i].expand)
		w += total/expands;
	} else {
	    h = box->subviews[i].minSize;
	    if (box->subviews[i].expand)
		h += total/expands;
	}
	if (!box->subviews[i].end) {
	    W_MoveView(box->subviews[i].view, x, y);
	}
	W_ResizeView(box->subviews[i].view, w, h);
	if (box->horizontal) {
	    if (box->subviews[i].end)
		xe -= w + box->subviews[i].space;
	    else
		x += w + box->subviews[i].space;
	} else {
	    if (box->subviews[i].end)
		ye -= h + box->subviews[i].space;
	    else
		y += h + box->subviews[i].space;
	}
	if (box->subviews[i].end) {
	    W_MoveView(box->subviews[i].view, xe, ye);
	}
    }
}


void
WMSetBoxBorderWidth(WMBox *box, unsigned width)
{
    box->borderWidth = width;
    
    rearrange(box);
}


void
WMAddBoxSubview(WMBox *bPtr, WMView *view, Bool expand, Bool fill,
		int minSize, int maxSize, int space)
{
    int i = bPtr->subviewCount;
    
    bPtr->subviewCount++;
    if (!bPtr->subviews)
	bPtr->subviews = wmalloc(sizeof(SubviewItem));
    else
	bPtr->subviews = wrealloc(bPtr->subviews, 
				  bPtr->subviewCount*sizeof(SubviewItem));
    bPtr->subviews[i].view = view;
    bPtr->subviews[i].minSize = minSize;
    bPtr->subviews[i].maxSize = maxSize;
    bPtr->subviews[i].expand = expand;
    bPtr->subviews[i].fill = fill;
    bPtr->subviews[i].space = space;
    bPtr->subviews[i].end = 0;

    rearrange(bPtr);
}



void
WMAddBoxSubviewAtEnd(WMBox *bPtr, WMView *view, Bool expand, Bool fill,
		     int minSize, int maxSize, int space)
{
    int i = bPtr->subviewCount;
    
    bPtr->subviewCount++;
    if (!bPtr->subviews)
	bPtr->subviews = wmalloc(sizeof(SubviewItem));
    else
	bPtr->subviews = wrealloc(bPtr->subviews, 
				  bPtr->subviewCount*sizeof(SubviewItem));
    bPtr->subviews[i].view = view;
    bPtr->subviews[i].minSize = minSize;
    bPtr->subviews[i].maxSize = maxSize;
    bPtr->subviews[i].expand = expand;
    bPtr->subviews[i].fill = fill;
    bPtr->subviews[i].space = space;
    bPtr->subviews[i].end = 1;

    rearrange(bPtr);
}


void
WMRemoveBoxSubview(WMBox *bPtr, WMView *view)
{
    int i;
    
    for (i = 0; i < bPtr->subviewCount; i++) {
	if (bPtr->subviews[i].view == view) {
	    memmove(&bPtr->subviews[i], &bPtr->subviews[i+1],
		    (bPtr->subviewCount - i - 1) * sizeof(void*));
	    bPtr->subviewCount--;
	    break;
	}
    }
    rearrange(bPtr);
}


void
WMSetBoxHorizontal(WMBox *box, Bool flag)
{
    box->horizontal = flag;
    rearrange(box);
}


static void
destroyBox(Box *bPtr)
{
    WMRemoveNotificationObserver(bPtr);
    wfree(bPtr);
}


static void
didResize(struct W_ViewDelegate *delegate, WMView *view)
{
    rearrange(view->self);
}


static void
handleEvents(XEvent *event, void *data)
{
    Box *bPtr = (Box*)data;

    CHECK_CLASS(data, WC_Box);

    switch (event->type) {	
     case DestroyNotify:
	destroyBox(bPtr);
	break;
	
     case ConfigureNotify:
	rearrange(bPtr);
	break;
    }
}

