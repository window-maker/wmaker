



#include "WINGsP.h"


char *WMColorWellDidChangeNotification = "WMColorWellDidChangeNotification";


typedef struct W_ColorWell {
    W_Class widgetClass;
    WMView *view;
    
    WMView *colorView;

    WMColor *color;

    WMAction *action;
    void *clientData;

    WMPoint ipoint;

    struct {
	unsigned int active:1;
	unsigned int bordered:1;
    } flags;
} ColorWell;

static char *_ColorWellActivatedNotification = "_ColorWellActivatedNotification";



static void destroyColorWell(ColorWell *cPtr);
static void paintColorWell(ColorWell *cPtr);

static void handleEvents(XEvent *event, void *data);

static void handleDragEvents(XEvent *event, void *data);

static void handleActionEvents(XEvent *event, void *data);

static void willResizeColorWell();



W_ViewDelegate _ColorWellViewDelegate = {
    NULL,
    NULL,
    NULL,
    NULL,
    willResizeColorWell
};


static unsigned draggingSourceOperation(WMView *self, Bool local);

static WMData* fetchDragData(WMView *self, char *type);

static WMDragSourceProcs _DragSourceProcs = {
    draggingSourceOperation,
    NULL,
    NULL,
    fetchDragData
};


static unsigned draggingEntered(WMView *self, WMDraggingInfo *info);
static unsigned draggingUpdated(WMView *self, WMDraggingInfo *info);
static void draggingExited(WMView *self, WMDraggingInfo *info);
static char *prepareForDragOperation(WMView *self, WMDraggingInfo *info);
static Bool performDragOperation(WMView *self, WMDraggingInfo *info, 
				 WMData *data);
static void concludeDragOperation(WMView *self, WMDraggingInfo *info);

static WMDragDestinationProcs _DragDestinationProcs = {
    draggingEntered,
    draggingUpdated,
    draggingExited,
    prepareForDragOperation,
    performDragOperation,
    concludeDragOperation
};


#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		30
#define DEFAULT_BORDER_WIDTH	6

#define MIN_WIDTH	16
#define MIN_HEIGHT	8



static void
colorChangedObserver(void *data, WMNotification *notification)
{
    WMColorPanel *panel = (WMColorPanel*)WMGetNotificationObject(notification);
    WMColorWell *cPtr = (WMColorWell*)data;
    WMColor *color;

    if (!cPtr->flags.active)
	return;

    color = WMGetColorPanelColor(panel);
    
    WMSetColorWellColor(cPtr, color);
    WMPostNotificationName(WMColorWellDidChangeNotification, cPtr, NULL);
}


static void
updateColorCallback(void *self, void *data)
{
    WMColorPanel *panel = (WMColorPanel*)self;
    WMColorWell *cPtr = (ColorWell*)data;
    WMColor *color;

    color = WMGetColorPanelColor(panel);
    WMSetColorWellColor(cPtr, color);
    WMPostNotificationName(WMColorWellDidChangeNotification, cPtr, NULL);
}



static void
activatedObserver(void *data, WMNotification *notification)
{
/*
    WMColorWell *cPtr = (WMColorWell*)data;

    if (!cPtr->flags.active || WMGetNotificationObject(notification) == cPtr)
	return;

    W_SetViewBackgroundColor(cPtr->view, WMWidgetScreen(cPtr)->gray);
    paintColorWell(cPtr);

    cPtr->flags.active = 0;
 */
}



WMColorWell*
WMCreateColorWell(WMWidget *parent)
{
    ColorWell *cPtr;

    cPtr = wmalloc(sizeof(ColorWell));
    memset(cPtr, 0, sizeof(ColorWell));

    cPtr->widgetClass = WC_ColorWell;
    
    cPtr->view = W_CreateView(W_VIEW(parent));
    if (!cPtr->view) {
	wfree(cPtr);
	return NULL;
    }
    cPtr->view->self = cPtr;

    cPtr->view->delegate = &_ColorWellViewDelegate;
    
    cPtr->colorView = W_CreateView(cPtr->view);
    if (!cPtr->colorView) {
	W_DestroyView(cPtr->view);
	wfree(cPtr);
	return NULL;
    }
    cPtr->colorView->self = cPtr;
	
    WMCreateEventHandler(cPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, cPtr);
    
    WMCreateEventHandler(cPtr->colorView, ExposureMask, handleEvents, cPtr);

    WMCreateEventHandler(cPtr->colorView, ButtonPressMask|ButtonMotionMask
			 |EnterWindowMask, handleDragEvents, cPtr);

    WMCreateEventHandler(cPtr->view, ButtonPressMask, handleActionEvents, 
			 cPtr);

    cPtr->colorView->flags.mapWhenRealized = 1;

    cPtr->flags.bordered = 1;

    W_ResizeView(cPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    WMAddNotificationObserver(activatedObserver, cPtr, 
			      _ColorWellActivatedNotification, NULL);

    cPtr->color = WMBlackColor(WMWidgetScreen(cPtr));

    WMAddNotificationObserver(colorChangedObserver, cPtr,
			      WMColorPanelColorChangedNotification, NULL);
    
    WMSetViewDragSourceProcs(cPtr->view, &_DragSourceProcs);
    WMSetViewDragDestinationProcs(cPtr->view, &_DragDestinationProcs);

    {
	char *types[2] = {"application/X-color", NULL};
	
	WMRegisterViewForDraggedTypes(cPtr->view, types);
    }
    
    return cPtr;
}


void
WMSetColorWellColor(WMColorWell *cPtr, WMColor *color)
{
    if (cPtr->color)
	WMReleaseColor(cPtr->color);
    
    cPtr->color = WMRetainColor(color);
    
    if (cPtr->colorView->flags.realized && cPtr->colorView->flags.mapped)
	paintColorWell(cPtr);
}


WMColor*
WMGetColorWellColor(WMColorWell *cPtr)
{
    return cPtr->color;
}


void
WSetColorWellBordered(WMColorWell *cPtr, Bool flag)
{
    if (cPtr->flags.bordered != flag) {
	cPtr->flags.bordered = flag;
	W_ResizeView(cPtr->view, cPtr->view->size.width, cPtr->view->size.height);
    }
}


static void
willResizeColorWell(W_ViewDelegate *self, WMView *view,
		    unsigned int *width, unsigned int *height)
{
    WMColorWell *cPtr = (WMColorWell*)view->self;
    int bw;
    
    if (cPtr->flags.bordered) {

	if (*width < MIN_WIDTH)
	    *width = MIN_WIDTH;
	if (*height < MIN_HEIGHT)
	    *height = MIN_HEIGHT;

	bw = (int)((float)WMIN(*width, *height)*0.24);

	W_ResizeView(cPtr->colorView, *width-2*bw, *height-2*bw);

	if (cPtr->colorView->pos.x!=bw || cPtr->colorView->pos.y!=bw)
	    W_MoveView(cPtr->colorView, bw, bw);
    } else {    
	W_ResizeView(cPtr->colorView, *width, *height);

	W_MoveView(cPtr->colorView, 0, 0);
    }
}


static void
paintColorWell(ColorWell *cPtr)
{
    W_Screen *scr = cPtr->view->screen;

    W_DrawRelief(scr, cPtr->view->window, 0, 0, cPtr->view->size.width,
		 cPtr->view->size.height, WRRaised);
    
    W_DrawRelief(scr, cPtr->colorView->window, 0, 0, 
		 cPtr->colorView->size.width, cPtr->colorView->size.height, 
		 WRSunken);

    if (cPtr->color)
	WMPaintColorSwatch(cPtr->color, cPtr->colorView->window,
			   2, 2, cPtr->colorView->size.width-4, 
			   cPtr->colorView->size.height-4);
}



static void
handleEvents(XEvent *event, void *data)
{
    ColorWell *cPtr = (ColorWell*)data;

    CHECK_CLASS(data, WC_ColorWell);


    switch (event->type) {	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintColorWell(cPtr);
	break;
	
     case DestroyNotify:
	destroyColorWell(cPtr);
	break;
	
    }
}


static unsigned 
draggingSourceOperation(WMView *self, Bool local)
{
    return WDOperationCopy;
}


static WMData* 
fetchDragData(WMView *self, char *type)
{
    char *color = WMGetColorRGBDescription(((WMColorWell*)self->self)->color);
    WMData *data;
    
    data = WMCreateDataWithBytes(color, strlen(color)+1);
    
    wfree(color);
    
    return data;
}


static WMPixmap*
makeDragPixmap(WMColorWell *cPtr)
{
    WMScreen *scr = cPtr->view->screen;
    Pixmap pix;
    
    pix = XCreatePixmap(scr->display, W_DRAWABLE(scr), 16, 16, scr->depth);
    
    XFillRectangle(scr->display, pix, WMColorGC(cPtr->color), 0, 0, 15, 15);
    
    XDrawRectangle(scr->display, pix, WMColorGC(scr->black), 0, 0, 15, 15);
    
    return WMCreatePixmapFromXPixmaps(scr, pix, None, 16, 16, scr->depth);
}


static void
handleDragEvents(XEvent *event, void *data)
{
    WMColorWell *cPtr = (ColorWell*)data;

    switch (event->type) {
     case ButtonPress:
	if (event->xbutton.button == Button1) {
	    cPtr->ipoint.x = event->xbutton.x;
	    cPtr->ipoint.y = event->xbutton.y;
	}
	break;

     case MotionNotify:
	if (event->xmotion.state & Button1Mask) {
	    if (abs(cPtr->ipoint.x - event->xmotion.x) > 4
		|| abs(cPtr->ipoint.y - event->xmotion.y) > 4) {
		WMSize offs;
		WMPixmap *pixmap;
		char *types[2] = {"application/X-color", NULL};

		offs.width = 2;
		offs.height = 2;
		pixmap = makeDragPixmap(cPtr);

		WMDragImageFromView(cPtr->view, pixmap, types,
				    wmkpoint(event->xmotion.x_root,
					     event->xmotion.y_root),
				    offs, event, True);
				   
		
		WMReleasePixmap(pixmap);
	    }
	}
	break;
    }
}


static void
handleActionEvents(XEvent *event, void *data)
{
    WMColorWell *cPtr = (ColorWell*)data;
    WMScreen *scr = WMWidgetScreen(cPtr);
    WMColorPanel *cpanel;
    
    if (cPtr->flags.active)
	W_SetViewBackgroundColor(cPtr->view, scr->gray);
    else
	W_SetViewBackgroundColor(cPtr->view, scr->white);
    paintColorWell(cPtr);

    cPtr->flags.active ^= 1;

    if (cPtr->flags.active) {
	WMPostNotificationName(_ColorWellActivatedNotification, cPtr, NULL);
    }
    cpanel = WMGetColorPanel(scr);

    WMSetColorPanelAction(cpanel, updateColorCallback, cPtr);

    if (cPtr->color)
	WMSetColorPanelColor(cpanel, cPtr->color);
    WMShowColorPanel(cpanel);
}


static void
destroyColorWell(ColorWell *cPtr)
{
    WMRemoveNotificationObserver(cPtr);

    if (cPtr->color)
	WMReleaseColor(cPtr->color);
   
    wfree(cPtr);
}



static unsigned
draggingEntered(WMView *self, WMDraggingInfo *info)
{
    return WDOperationCopy;
}


static unsigned
draggingUpdated(WMView *self, WMDraggingInfo *info)
{
    return WDOperationCopy;
}


static void
draggingExited(WMView *self, WMDraggingInfo *info)
{

}


static char*
prepareForDragOperation(WMView *self, WMDraggingInfo *info)
{
    return "application/X-color";
}


static Bool
performDragOperation(WMView *self, WMDraggingInfo *info, WMData *data)
{
    char *colorName = (char*)WMDataBytes(data);
    WMColor *color;
    
    color = WMCreateNamedColor(W_VIEW_SCREEN(self), colorName, True);
    
    WMSetColorWellColor(self->self, color);
    
    WMReleaseColor(color);
    
    return True;
}


static void
concludeDragOperation(WMView *self, WMDraggingInfo *info)
{
}
