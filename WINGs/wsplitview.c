



#include "WINGsP.h"




typedef struct W_SplitView {
    W_Class widgetClass;
    W_View *view;
    
/*    WMSplitViewResizeSubviewsProc *resizeSubviewsProc;
 */
    WMSplitViewConstrainProc *constrainProc;
    
    struct {
	unsigned int splitViewIsFull:1;	       /* already added 2 subviews */
    } flags;
} SplitView;


#define DIVIDER_THICKNESS	8



static void destroySplitView(SplitView *sPtr);
static void paintSplitView(SplitView *sPtr);


static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);


W_ViewProcedureTable _SplitViewViewProcedures = {
    NULL,
	NULL,
	NULL
};



static int
subviewCount(SplitView *sPtr)
{
    int count = 0;
    WMView *view;
    
    for (view=sPtr->view->childrenList; view != NULL; view=view->nextSister)
	count++;
    
    return count;
}


static void
handleViewResized(void *self, WMNotification *notification)
{
    SplitView *sPtr = (SplitView*)self;
    int oldHeight;
    WMView *view = sPtr->view;
    int newWidth = view->size.width;
    WMView *upper, *lower;

    if (!view->childrenList)
	return;
    
    if (view->childrenList->nextSister==NULL) {
	if (view->self)
	    WMResizeWidget(view->childrenList->self, newWidth, view->size.height);
	else
	    W_ResizeView(view->childrenList, newWidth, view->size.height);
    } else {
	upper = view->childrenList;
	lower = upper->nextSister;
	oldHeight = upper->size.height+DIVIDER_THICKNESS+lower->size.height;
	
	if (oldHeight > view->size.height
	    && upper->size.height+DIVIDER_THICKNESS >= view->size.height) {
	    WMAdjustSplitViewSubviews(sPtr);
	} else {
	    if (upper->self) {
		WMResizeWidget(upper->self, newWidth, upper->size.height);
	    } else {
		W_ResizeView(upper, newWidth, upper->size.height);
	    }
	    if (lower->self) {
		WMResizeWidget(lower->self, newWidth,
			       view->size.height-lower->pos.y);
	    } else {
		W_ResizeView(lower, newWidth, view->size.height-lower->pos.y);
	    }
	}
    }
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
	free(sPtr);
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
    
    return sPtr;
}


void
WMAddSplitViewSubview(WMSplitView *sPtr, WMView *subview)
{
    int wasMapped;

    assert(!sPtr->flags.splitViewIsFull);
    
    wasMapped = subview->flags.mapped;
    if (wasMapped) {
	W_UnmapView(subview);
    }

    W_ReparentView(subview, sPtr->view);

#if 0
    if (sPtr->resizeSubviewsProc && subviewCount(sPtr)>1) {
	(*sPtr->resizeSubviewsProc)(sPtr, sPtr->view->size.width, 
				    sPtr->view->size.height);
	/* check if there is free space for the new subview and
	 * put the subview in it */
    } else {
    }
#endif
    WMAdjustSplitViewSubviews(sPtr);
    
    if (subviewCount(sPtr)==2)
	sPtr->flags.splitViewIsFull = 1;
    
    if (wasMapped) {
	W_MapView(subview);
    }
}


void 
WMSetSplitViewConstrainProc(WMSplitView *sPtr, WMSplitViewConstrainProc *proc)
{
    sPtr->constrainProc = proc;
}


void
WMAdjustSplitViewSubviews(WMSplitView *sPtr)
{
    int theight = sPtr->view->size.height;
    int width = sPtr->view->size.width;
    int height;
    int y, count;
    W_View *view;

    count = subviewCount(sPtr);

    height = (theight - (count-1)*DIVIDER_THICKNESS)/count;
    
    
    view = sPtr->view->childrenList;
    if (view->self) {
	WMMoveWidget(view->self, 0, 0);
	WMResizeWidget(view->self, width, height);
    } else {
	W_MoveView(view, 0, 0);
	W_ResizeView(view, width, height);
    }
    
    y = height + DIVIDER_THICKNESS;
    
    for (view = view->nextSister; view != NULL; view = view->nextSister) {
	if (view->self) {
	    WMMoveWidget(view->self, 0, y);
	    WMResizeWidget(view->self, width, height);
	} else {
	    W_MoveView(view, 0, y);
	    W_ResizeView(view, width, height);
	}
	y += height + DIVIDER_THICKNESS;
    }
}

#if 0
void
WMSetSplitViewResizeSubviewsProc(WMSplitView *sPtr, 
				 WMSplitViewResizeSubviewsProc *proc)
{
    sPtr->resizeSubviewsProc = proc;
}
#endif


int
WMGetSplitViewDividerThickness(WMSplitView *sPtr)
{
    return DIVIDER_THICKNESS;
}


static void
paintSplitView(SplitView *sPtr)
{
    W_Screen *scr = sPtr->view->screen;
    int y, x;
    W_View *ptr;
    WMPixmap *dimple = scr->scrollerDimple;

    XClearWindow(scr->display, sPtr->view->window);

    x = (sPtr->view->size.width - dimple->width)/2;

    ptr = sPtr->view->childrenList;

    y = ptr->size.height;
    XSetClipMask(scr->display, scr->clipGC, dimple->mask);
    while (ptr->nextSister) {
	y += (DIVIDER_THICKNESS - dimple->width)/2;

	XSetClipOrigin(scr->display, scr->clipGC, x, y);
	XCopyArea(scr->display, dimple->pixmap, sPtr->view->window,
		  scr->clipGC, 0, 0, dimple->width, dimple->height, x, y);

	y += ptr->size.height;

	ptr = ptr->nextSister;
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
dragDivider(SplitView *sPtr, int clickY)
{
    int divider;
    WMView *view, *view1=NULL, *view2=NULL;
    int y;
    int ofsY;
    int done;
    int dragging;
    int minCoord;
    int maxCoord;
    XEvent ev;
    WMScreen *scr = sPtr->view->screen;

    view = sPtr->view->childrenList;
    divider = 0;
    ofsY = 0;
    y = 0;
    done = 0;
    while (view) {
	y += view->size.height+DIVIDER_THICKNESS;
	if (clickY < y) {
	    /* offset from point where use clicked and the top of the 
	     * divider */
	    ofsY = clickY - y + DIVIDER_THICKNESS;
	    view1 = view;
	    view2 = view->nextSister;
	    /* can't be NULL. It would mean the divider is at the bottom */
	    assert(view2!=NULL);
	    done = 1;
	    break;
	}
	view = view->nextSister;
	divider++;
    }
    assert(done);
    
    minCoord = view1->pos.y;
    maxCoord = view2->pos.y+view2->size.height-DIVIDER_THICKNESS;

    if (sPtr->constrainProc)
	(*sPtr->constrainProc)(sPtr, divider, &minCoord, &maxCoord);

    done = 0;
    dragging = 0;
    while (!done) {
	WMMaskEvent(scr->display, ButtonMotionMask|ButtonReleaseMask
		    |ExposureMask, &ev);

	switch (ev.type) {
	 case ButtonRelease:
	    done = 1;
	    if (dragging) {
		XFillRectangle(scr->display, sPtr->view->window, scr->ixorGC,
			       0, y, sPtr->view->size.width,DIVIDER_THICKNESS);
	    }
	    break;
	    
	 case MotionNotify:
	    if (dragging) {
		XFillRectangle(scr->display, sPtr->view->window, scr->ixorGC,
			       0, y, sPtr->view->size.width,DIVIDER_THICKNESS);
	    }
	    if (ev.xmotion.y-ofsY < minCoord)
		y = minCoord;
	    else if (ev.xmotion.y-ofsY > maxCoord)
		y = maxCoord;
	    else
		y = ev.xmotion.y-ofsY;
	    XFillRectangle(scr->display, sPtr->view->window, scr->ixorGC,
			   0, y, sPtr->view->size.width, DIVIDER_THICKNESS);
	    dragging = 1;
	    break;
	    
	 default:
	    WMHandleEvent(&ev);
	    break;
	}
    }
    
    if (dragging) {
	int theight;
	
	theight = view1->size.height + view2->size.height + DIVIDER_THICKNESS;
	
	WMResizeWidget(view1->self, sPtr->view->size.width, y - view1->pos.y);
	
	WMResizeWidget(view2->self, sPtr->view->size.width,
		       theight - view1->size.height - DIVIDER_THICKNESS);
	WMMoveWidget(view2->self, 0, 
		     view1->pos.y+view1->size.height+DIVIDER_THICKNESS);
    }
}


static void
handleActionEvents(XEvent *event, void *data)
{
    
    CHECK_CLASS(data, WC_SplitView);


    switch (event->type) {
     case ButtonPress:
	if (event->xbutton.button == Button1)
	    dragDivider(data, event->xbutton.y);
	break;
    }
}



static void
destroySplitView(SplitView *sPtr)
{
    WMRemoveNotificationObserver(sPtr);
   
    free(sPtr);
}

