



#include "WINGsP.h"

/*
char *WMSplitViewDidResizeSubviewsNotification
    = "WMSplitViewDidResizeSubviewsNotification";
char *WMSplitViewWillResizeSubviewsNotification
    = "WMSplitViewWillResizeSubviewsNotification";
*/

typedef struct _T_SplitViewSubview {
    WMView *view;
    int minSize;
    int maxSize;
    int size;
    int pos;
} T_SplitViewSubview;


typedef struct W_SplitView {
    W_Class widgetClass;
    W_View *view;
    
    WMBag *subviewsBag;
    
    WMSplitViewConstrainProc *constrainProc;
    
    struct {
	unsigned int vertical:1;	
	unsigned int adjustOnPaint:1;
	unsigned int subviewsWereManuallyMoved:1;
    } flags;

    /* WMSplitViewResizeSubviewsProc *resizeSubviewsProc; */

} SplitView;


#define DIVIDER_THICKNESS   8
#define MIN_SUBVIEW_SIZE    4
#define MAX_SUBVIEW_SIZE    -1


#define _GetSubviewsCount() WMGetBagItemCount(sPtr->subviewsBag)

#define _AddPSubviewStruct(P) \
(WMPutInBag(sPtr->subviewsBag,((void*)P)))

#define _GetPSubviewStructAt(i) \
((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))

#define _GetSubviewAt(i) \
(((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))->view)

#define _GetMinSizeAt(i) \
(((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))->minSize)

#define _GetMaxSizeAt(i) \
(((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))->maxSize)

#define _GetSizeAt(i) \
(((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))->size)

#define _GetPosAt(i) \
(((T_SplitViewSubview*)WMGetFromBag(sPtr->subviewsBag,(i)))->pos)

#define _GetSplitViewSize() \
((sPtr->flags.vertical) ? sPtr->view->size.width : sPtr->view->size.height)

static void destroySplitView(SplitView *sPtr);
static void paintSplitView(SplitView *sPtr);

static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);


static void
getConstraints(SplitView *sPtr, int index, int *minSize, int *maxSize)
{
    *minSize = MIN_SUBVIEW_SIZE;
    *maxSize = MAX_SUBVIEW_SIZE;

    if (sPtr->constrainProc)
	(*sPtr->constrainProc)(sPtr, index, minSize, maxSize);

    if (*minSize < MIN_SUBVIEW_SIZE)
	*minSize = MIN_SUBVIEW_SIZE;

    if (*maxSize < MIN_SUBVIEW_SIZE)
    	*maxSize = MAX_SUBVIEW_SIZE;
    else if (*maxSize < *minSize)
    	*maxSize = *minSize;
}


static void
updateConstraints(SplitView *sPtr)
{
    T_SplitViewSubview *p;
    int i, count;

    count = _GetSubviewsCount();
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
    	getConstraints(sPtr, i, &(p->minSize), &(p->maxSize));
    }
}


static void
resizeView(SplitView *sPtr, WMView *view, int size)
{
    int width, height;

    if (sPtr->flags.vertical) {
	width = size;
    	height = sPtr->view->size.height;
    } else {
	width = sPtr->view->size.width;
    	height = size;
    }

    if (view->self)
	WMResizeWidget(view->self, width, height);
    else
	W_ResizeView(view, width, height);
}


static void
reparentView(SplitView *sPtr, WMView *view, int pos)
{
    int x, y;

    if (sPtr->flags.vertical) {
    	x = pos;
	y = 0;
    } else {
    	x = 0;
	y = pos;
    }

    W_ReparentView(view, sPtr->view, x, y);
}


static void
moveView(SplitView *sPtr, WMView *view, int pos)
{
    int x, y;

    if (sPtr->flags.vertical) {
    	x = pos;
	y = 0;
    } else {
    	x = 0;
	y = pos;
    }

    if (view->self)
	WMMoveWidget(view->self, x, y);
    else
	W_MoveView(view, x, y);
}


static int
checkSizes(SplitView *sPtr)
{
    int i, count, offset;
    T_SplitViewSubview *p;

    count = _GetSubviewsCount();
    offset = 0;
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
	if (p->size < p->minSize) {
	    offset += p->minSize - p->size;
	    p->size = p->minSize;
	} else if (p->maxSize != MAX_SUBVIEW_SIZE && p->size > p->maxSize) {
	    offset += p->maxSize - p->size;
	    p->size = p->maxSize;
	}
    }
    
    return (offset);
}


static void
checkPositions(SplitView *sPtr)
{
    int i, count, pos;
    T_SplitViewSubview *p;

    count = _GetSubviewsCount();
    pos = 0;
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
	p->pos = pos;
	pos += p->size + DIVIDER_THICKNESS;
    }
}


static void
updateSubviewsGeom(SplitView *sPtr)
{
    int i, count;
    T_SplitViewSubview *p;

    count = _GetSubviewsCount();
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
	resizeView(sPtr, p->view, p->size);
	moveView(sPtr, p->view, p->pos);
    }
}


static int
getTotalSize(SplitView *sPtr)
{
    int i, count, totSize;

    count = _GetSubviewsCount();
    if (!count)
    	return (0);

    totSize = 0;
    for (i = 0; i < count; i++)
    	totSize += _GetSizeAt(i) + DIVIDER_THICKNESS;
    
    return (totSize - DIVIDER_THICKNESS);
}


static Bool
distributeOffsetEqually(SplitView *sPtr, int offset)
{
    T_SplitViewSubview *p;
    int i, count, sizeChanged, forced;

    if ((count = _GetSubviewsCount()) < 1)
    	return (True);

    forced = False;
    while (offset != 0) {
	sizeChanged = 0;
    	for (i = 0; i < count && offset != 0; i++) {
    	    p = _GetPSubviewStructAt(i);
	    if (offset < 0) {
	    	if (p->size > p->minSize) {
		    offset++;
	    	    p->size--;
		    sizeChanged = 1;
		}
	    } else if (p->maxSize == MAX_SUBVIEW_SIZE || p->size < p->maxSize) {
		offset--;
	    	p->size++;
		sizeChanged = 1;
	    }
    	}
	if (offset != 0 && !sizeChanged) {
	    p = _GetPSubviewStructAt(count-1);
	    if (offset > 0) {
	    	p->size += offset;
	    	p->maxSize = MAX_SUBVIEW_SIZE;
	    }
	    offset = 0;
	    forced = True;
	}
    }
    
    return (forced);
}


static Bool
distributeOffsetFormEnd(SplitView *sPtr, int offset)
{
    T_SplitViewSubview *p;
    int i, count, sizeTmp;

    if ((count = _GetSubviewsCount()) < 1)
    	return (True);

    for (i = count-1; i >= 0 && offset != 0; i--) {
	p = _GetPSubviewStructAt(i);
	sizeTmp = p->size;
	if (offset > 0) {
	    if (p->maxSize == MAX_SUBVIEW_SIZE || p->size + offset < p->maxSize)
	    	p->size += offset;
	    else
		p->size = p->maxSize;
	} else {
	    if (p->size + offset >= p->minSize)
	    	p->size += offset;
	    else
		p->size = p->minSize;
	}
	offset -= p->size - sizeTmp;
    }

    return (offset == 0);
}


static void
adjustSplitViewSubviews(WMSplitView *sPtr)
{
    T_SplitViewSubview *p;
    int i, count, adjSize, adjPad;

    CHECK_CLASS(sPtr, WC_SplitView);

#if 0
    printf("---- (adjustSplitViewSubviews - 1) ----\n");
    dumpSubviews(sPtr);
#endif

    if ((count = _GetSubviewsCount()) < 1)
	return;

    adjSize = (_GetSplitViewSize() - ((count-1) * DIVIDER_THICKNESS))  / count;
    adjPad =  (_GetSplitViewSize() - ((count-1) * DIVIDER_THICKNESS)) % count;
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
	p->size = adjSize;
    }
    
    distributeOffsetEqually(sPtr, adjPad - checkSizes(sPtr));
    
    checkPositions(sPtr);
    updateSubviewsGeom(sPtr);
    
    sPtr->flags.subviewsWereManuallyMoved = 0;

#if 0    
    printf("---- (adjustSplitViewSubviews - 2) ----\n");
    dumpSubviews(sPtr);
#endif
}

#if 0
static void
handleSubviewResized(void *self, WMNotification *notif)
{
    SplitView *sPtr = (SplitView*)self;

    CHECK_CLASS(sPtr, WC_SplitView);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) {
    	T_SplitViewSubview *p;
    	int i, count, done;
    	WMView *view = WMGetNotificationObject(notif);

    	count = _GetSubviewsCount();
    	done = 0;
	for (i = 0; i < count; i++) {
    	    p = _GetPSubviewStructAt(i);
	    if (p->view == view) {
	    	done = 1;
		break;
	    }
	}
	
	if (done) {
	    /* TODO !!! */
	    resizeView(sPtr, p->view, p->size);
	    moveView(sPtr, p->view, p->pos);
	}
    }
}
#endif

static void
handleViewResized(void *self, WMNotification *notification)
{
    SplitView *sPtr = (SplitView*)self;

#if 0
    printf("---- (handleViewResized - 1) ----\n");
    dumpSubviews(sPtr);
#endif

    updateConstraints(sPtr);
    checkSizes(sPtr);
    
    if (sPtr->constrainProc || sPtr->flags.subviewsWereManuallyMoved) {
    	distributeOffsetFormEnd(sPtr, _GetSplitViewSize() - getTotalSize(sPtr));
    	checkPositions(sPtr);
    	updateSubviewsGeom(sPtr);
    } else
    	adjustSplitViewSubviews(sPtr);

    assert(checkSizes(sPtr) == 0);

#if 0
    printf("---- (handleViewResized - 2) ----\n");
    dumpSubviews(sPtr);
#endif
}


static void
paintSplitView(SplitView *sPtr)
{
    T_SplitViewSubview *p;
    W_Screen *scr = sPtr->view->screen;
    int x, y, i, count;
    WMPixmap *dimple = scr->scrollerDimple;

#if 0
    printf("---- (paintSplitView - 1) ----\n");
    dumpSubviews(sPtr);
#endif
    
    if (!sPtr->view->flags.mapped || !sPtr->view->flags.realized)
    	return;

    XClearWindow(scr->display, sPtr->view->window);

    count = _GetSubviewsCount();
    if (count == 0)
    	return;

    if (sPtr->flags.adjustOnPaint) {
	handleViewResized(sPtr, NULL);
	sPtr->flags.adjustOnPaint = 0;
    }

    XSetClipMask(scr->display, scr->clipGC, dimple->mask);

    if (sPtr->flags.vertical) {
	x = ((DIVIDER_THICKNESS - dimple->width) / 2);
	y = (sPtr->view->size.height - dimple->height)/2;
    } else {
	x = (sPtr->view->size.width - dimple->width)/2;
	y = ((DIVIDER_THICKNESS - dimple->height) / 2);
    }

    for (i = 0; i < count-1; i++) {
    	p = _GetPSubviewStructAt(i);

	if (sPtr->flags.vertical)
    	    x += p->size;
	else
    	    y += p->size;

	XSetClipOrigin(scr->display, scr->clipGC, x, y);
	XCopyArea(scr->display, dimple->pixmap, sPtr->view->window,
		  scr->clipGC, 0, 0, dimple->width, dimple->height, x, y);
	
	if (sPtr->flags.vertical)
    	    x += DIVIDER_THICKNESS;
	else
	    y += DIVIDER_THICKNESS;
    }

#if 0
    printf("---- (paintSplitView - 2) ----\n");
    dumpSubviews(sPtr);
#endif
}


static void
drawDragingRectangle(SplitView *sPtr, int pos)
{
    int x, y, w, h;
    
    if (sPtr->flags.vertical) {
    	x = pos;
	y = 0;
	w = DIVIDER_THICKNESS;
	h = sPtr->view->size.height;
    } else {
    	x = 0;
	y = pos;
	w = sPtr->view->size.width;
	h = DIVIDER_THICKNESS;
    }
    
    XFillRectangle(sPtr->view->screen->display, sPtr->view->window,
		   sPtr->view->screen->ixorGC, x, y, w, h);
}


static void
getMinMaxDividerCoord(SplitView *sPtr, int divider, int *minC, int *maxC)
{
    int relMinC, relMaxC;
    int totSize = _GetSizeAt(divider) + _GetSizeAt(divider+1);

    relMinC = _GetMinSizeAt(divider);
    if (_GetMaxSizeAt(divider+1) != MAX_SUBVIEW_SIZE
     && relMinC < totSize - _GetMaxSizeAt(divider+1))
    	relMinC = totSize - _GetMaxSizeAt(divider+1);

    relMaxC = totSize - _GetMinSizeAt(divider+1);
    if (_GetMaxSizeAt(divider) != MAX_SUBVIEW_SIZE
     && relMaxC > _GetMaxSizeAt(divider))
    	relMaxC = _GetMaxSizeAt(divider);
    
    *minC = _GetPosAt(divider) + relMinC;
    *maxC = _GetPosAt(divider) + relMaxC;
}


static void
dragDivider(SplitView *sPtr, int clickX, int clickY)
{
    int divider, pos, ofs, done, dragging;
    int i, count;
    XEvent ev;
    WMScreen *scr;
    int minCoord, maxCoord, coord;

    if (sPtr->constrainProc) {
	updateConstraints(sPtr);
	checkSizes(sPtr);
	distributeOffsetFormEnd(sPtr, _GetSplitViewSize() - getTotalSize(sPtr));
	checkPositions(sPtr);
	updateSubviewsGeom(sPtr);
    }

    scr = sPtr->view->screen;
    divider = ofs = pos = done = 0;
    coord = (sPtr->flags.vertical) ? clickX : clickY;
    count = _GetSubviewsCount();
    if (count < 2)
    	return;

    for (i = 0; i < count-1; i++) {
	pos += _GetSizeAt(i) + DIVIDER_THICKNESS;
	if (coord < pos) {
	    ofs = coord - pos + DIVIDER_THICKNESS;
	    done = 1;
	    break;
	}
	divider++;
    }

    if (!done)
    	return;

    getMinMaxDividerCoord(sPtr, divider, &minCoord, &maxCoord);

    done = 0;
    dragging = 0;
    while (!done) {
	WMMaskEvent(scr->display, ButtonMotionMask|ButtonReleaseMask
		    |ExposureMask, &ev);

    	coord = (sPtr->flags.vertical) ? ev.xmotion.x : ev.xmotion.y;

	switch (ev.type) {
	 case ButtonRelease:
	    done = 1;
	    if (dragging)
		drawDragingRectangle(sPtr, pos);
	    break;
	    
	 case MotionNotify:
	    if (dragging)
		drawDragingRectangle(sPtr, pos);
	    if (coord - ofs < minCoord)
		pos = minCoord;
	    else if (coord - ofs > maxCoord)
		pos = maxCoord;
	    else
		pos = coord - ofs;
	    drawDragingRectangle(sPtr, pos);
	    dragging = 1;
	    break;
	    
	 default:
	    WMHandleEvent(&ev);
	    break;
	}
    }
    
    if (dragging) {
    	T_SplitViewSubview *p1, *p2;
	int totSize;
	
	p1 = _GetPSubviewStructAt(divider);
	p2 = _GetPSubviewStructAt(divider+1);
	
	totSize = p1->size + DIVIDER_THICKNESS + p2->size;
	
	p1->size = pos - p1->pos;
	p2->size = totSize - p1->size - DIVIDER_THICKNESS;
	p2->pos = p1->pos + p1->size  + DIVIDER_THICKNESS;
	
	resizeView(sPtr, p1->view, p1->size);
	moveView(sPtr, p2->view, p2->pos);
	resizeView(sPtr, p2->view, p2->size);
	sPtr->flags.subviewsWereManuallyMoved = 1;
    }
}


static void
handleEvents(XEvent *event, void *data)
{
    SplitView *sPtr = (SplitView*)data;

    CHECK_CLASS(data, WC_SplitView);


    switch (event->type) {	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintSplitView(sPtr);
	break;
	
     case DestroyNotify:
	destroySplitView(sPtr);
	break;
    }
}


static void
handleActionEvents(XEvent *event, void *data)
{
    
    CHECK_CLASS(data, WC_SplitView);

    switch (event->type) {
     case ButtonPress:
	if (event->xbutton.button == Button1)
	    dragDivider(data, event->xbutton.x, event->xbutton.y);
	break;
    }
}



static void
destroySplitView(SplitView *sPtr)
{
    int i, count;

    count = _GetSubviewsCount();
    for (i = 0; i < count; i++)
    	wfree(WMGetFromBag(sPtr->subviewsBag, i));
    WMFreeBag(sPtr->subviewsBag);
   
    WMRemoveNotificationObserver(sPtr);
    
    wfree(sPtr);
}

WMSplitView*
WMCreateSplitView(WMWidget *parent)
{
    SplitView *sPtr;

    sPtr = wmalloc(sizeof(SplitView));
    memset(sPtr, 0, sizeof(SplitView));

    sPtr->widgetClass = WC_SplitView;
    
    sPtr->view = W_CreateView(W_VIEW(parent));
    if (!sPtr->view) {
	wfree(sPtr);
	return NULL;
    }
    sPtr->view->self = sPtr;

    WMSetViewNotifySizeChanges(sPtr->view, True);

    WMCreateEventHandler(sPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, sPtr);


    WMCreateEventHandler(sPtr->view, ButtonPressMask|ButtonReleaseMask
			 |EnterWindowMask|LeaveWindowMask, 
			 handleActionEvents, sPtr);

    
    WMAddNotificationObserver(handleViewResized, sPtr, 
			      WMViewSizeDidChangeNotification, sPtr->view);
    
    sPtr->subviewsBag = WMCreateBag(8);

    return sPtr;
}


void
WMAdjustSplitViewSubviews(WMSplitView *sPtr)
{
    CHECK_CLASS(sPtr, WC_SplitView);
    
    checkSizes(sPtr);

    adjustSplitViewSubviews(sPtr);

    assert(checkSizes(sPtr) == 0);
}


void
WMAddSplitViewSubview(WMSplitView *sPtr, WMView *subview)
{
    int wasMapped, count;
    T_SplitViewSubview *p;

    CHECK_CLASS(sPtr, WC_SplitView);

    if (!(p = (T_SplitViewSubview*)wmalloc(sizeof(T_SplitViewSubview))))
    	return;

    wasMapped = subview->flags.mapped;
    if (wasMapped)
	W_UnmapView(subview);

    count = _GetSubviewsCount();
    p->view = subview;
    getConstraints(sPtr, count, &(p->minSize), &(p->maxSize));
    if (sPtr->flags.vertical)
    	p->size = subview->size.width;
    else
    	p->size = subview->size.height;

    WMPutInBag(sPtr->subviewsBag,(void*)p);
    reparentView(sPtr, subview, 0);
    
/*
    We should have something like that...
    
    WMSetViewNotifySizeChanges(subview, True);
    WMAddNotificationObserver(handleSubviewResized, sPtr, 
			      WMViewSizeDidChangeNotification,
			      subview);
    WMSetViewNotifyMoveChanges(subview, True);
    WMAddNotificationObserver(handleSubviewResized, sPtr, 
			      WMViewMoveDidChangeNotification,
			      subview);
*/
    if (wasMapped) {
	W_MapView(subview);

	sPtr->flags.adjustOnPaint = 1;
	paintSplitView(sPtr);
    } else {
    	handleViewResized(sPtr, NULL);	
    }
}


WMView*
WMGetSplitViewSubviewAt(WMSplitView *sPtr, int index)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    if (index > 0 && index < _GetSubviewsCount())
    	return (_GetSubviewAt(index));
    else
    	return (NULL);
}


void
WMRemoveSplitViewSubview(WMSplitView *sPtr, WMView *view)
{
    T_SplitViewSubview *p;
    int i, count;

    CHECK_CLASS(sPtr, WC_SplitView);

    count = _GetSubviewsCount();
    for (i = 0; i < count; i++) {
    	p = _GetPSubviewStructAt(i);
	if (p->view == view) {
    	    wfree(p);
    	    WMDeleteFromBag(sPtr->subviewsBag, i);
    	    sPtr->flags.adjustOnPaint = 1;
    	    paintSplitView(sPtr);
	    break;
	}
    }
}


void
WMRemoveSplitViewSubviewAt(WMSplitView *sPtr, int index)
{
    T_SplitViewSubview *p;

    CHECK_CLASS(sPtr, WC_SplitView);

    if (index > 0 && index < _GetSubviewsCount()) {
    	p = _GetPSubviewStructAt(index);
    	wfree(p);
    	WMDeleteFromBag(sPtr->subviewsBag, index);
    	sPtr->flags.adjustOnPaint = 1;
	paintSplitView(sPtr);
    }
}


void 
WMSetSplitViewConstrainProc(WMSplitView *sPtr, WMSplitViewConstrainProc *proc)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    sPtr->constrainProc = proc;
}


int
WMGetSplitViewSubviewsCount(WMSplitView *sPtr)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    return (_GetSubviewsCount());
}


Bool
WMGetSplitViewVertical(WMSplitView *sPtr)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    return (sPtr->flags.vertical == 1);
}

void
WMSetSplitViewVertical(WMSplitView *sPtr, Bool flag)
{
    int vertical;
    
    CHECK_CLASS(sPtr, WC_SplitView);

    vertical = (flag) ? 1 : 0;
    if (sPtr->flags.vertical == vertical)
    	return;
    
    sPtr->flags.vertical = vertical;
    
/*    if (sPtr->view->flags.mapped && sPtr->view->flags.realized)*/
    	handleViewResized(sPtr, NULL);
/*    else
    	sPtr->flags.adjustOnPaint = 1;
 */
}


int
WMGetSplitViewDividerThickness(WMSplitView *sPtr)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    return (DIVIDER_THICKNESS);
}

#if 0
void
WMSetSplitViewResizeSubviewsProc(WMSplitView *sPtr, 
				 WMSplitViewResizeSubviewsProc *proc)
{
    CHECK_CLASS(sPtr, WC_SplitView);

    sPtr->resizeSubviewsProc = proc;
}
#endif
