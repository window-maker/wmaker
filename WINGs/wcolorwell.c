



#include "WINGsP.h"


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


static void destroyColorWell(ColorWell *cPtr);
static void paintColorWell(ColorWell *cPtr);

static void handleEvents(XEvent *event, void *data);

static void handleDragEvents(XEvent *event, void *data);

static void handleActionEvents(XEvent *event, void *data);

static void resizeColorWell();

W_ViewProcedureTable _ColorWellViewProcedures = {
    NULL,
	resizeColorWell,
	NULL
};


#if 0
static WMDragSourceProcs dragProcs = {
    
};
#endif

#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		30
#define DEFAULT_BORDER_WIDTH	6

#define MIN_WIDTH	16
#define MIN_HEIGHT	8



WMColorWell*
WMCreateColorWell(WMWidget *parent)
{
    ColorWell *cPtr;

    cPtr = wmalloc(sizeof(ColorWell));
    memset(cPtr, 0, sizeof(ColorWell));

    cPtr->widgetClass = WC_ColorWell;
    
    cPtr->view = W_CreateView(W_VIEW(parent));
    if (!cPtr->view) {
	free(cPtr);
	return NULL;
    }
    cPtr->view->self = cPtr;
    
    cPtr->colorView = W_CreateView(cPtr->view);
    if (!cPtr->colorView) {
	W_DestroyView(cPtr->view);
	free(cPtr);
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
    
    resizeColorWell(cPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);

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
	resizeColorWell(cPtr, cPtr->view->size.width, cPtr->view->size.height);
    }
}


#define MIN(a,b)	((a) > (b) ? (b) : (a))

static void
resizeColorWell(WMColorWell *cPtr, unsigned int width, unsigned int height)
{
    int bw;
    
    if (cPtr->flags.bordered) {

	if (width < MIN_WIDTH)
	    width = MIN_WIDTH;
	if (height < MIN_HEIGHT)
	    height = MIN_HEIGHT;

	bw = (int)((float)MIN(width, height)*0.24);
	
	W_ResizeView(cPtr->view, width, height);
    
	W_ResizeView(cPtr->colorView, width-2*bw, height-2*bw);

	if (cPtr->colorView->pos.x!=bw || cPtr->colorView->pos.y!=bw)
	    W_MoveView(cPtr->colorView, bw, bw);
    } else {
	W_ResizeView(cPtr->view, width, height);
    
	W_ResizeView(cPtr->colorView, width, height);

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
slideView(WMView *view, int srcX, int srcY, int dstX, int dstY)
{
    double x, y, dx, dy;
    int i;

    srcX -= 8;
    srcY -= 8;
    dstX -= 8;
    dstY -= 8;
    
    x = srcX;
    y = srcY;
    
    dx = (double)(dstX-srcX)/20.0;
    dy = (double)(dstY-srcY)/20.0;

    for (i = 0; i < 20; i++) {
	W_MoveView(view, x, y);
	XFlush(view->screen->display);
	
	x += dx;
	y += dy;
    }
}


static void
dragColor(ColorWell *cPtr, XEvent *event, WMPixmap *image)
{
    WMView *dragView;
    WMScreen *scr = cPtr->view->screen;
    Display *dpy = scr->display;
    XColor black = {0, 0,0,0, DoRed|DoGreen|DoBlue};
    XColor green = {0, 0x4500,0xb000,0x4500, DoRed|DoGreen|DoBlue};
    XColor back = {0, 0xffff,0xffff,0xffff, DoRed|DoGreen|DoBlue};
    Bool done = False;
    WMColorWell *activeWell = NULL;

    dragView = W_CreateTopView(scr);

    W_ResizeView(dragView, 16, 16);
    dragView->attribFlags |= CWOverrideRedirect | CWSaveUnder;
    dragView->attribs.event_mask = StructureNotifyMask;
    dragView->attribs.override_redirect = True;
    dragView->attribs.save_under = True;

    W_MoveView(dragView, event->xmotion.x_root-8, event->xmotion.y_root-8);

    W_RealizeView(dragView);

    W_MapView(dragView);

    XSetWindowBackgroundPixmap(dpy, dragView->window, WMGetPixmapXID(image));
    XClearWindow(dpy, dragView->window);
    

    XGrabPointer(dpy, dragView->window, True,
		 ButtonMotionMask|ButtonReleaseMask|EnterWindowMask,
		 GrabModeSync, GrabModeAsync, 
		 scr->rootWin, scr->defaultCursor, CurrentTime);

    while (!done) {
	XEvent ev;
	WMView *view;

	XAllowEvents(dpy, SyncPointer, CurrentTime);
	WMNextEvent(dpy, &ev);

	switch (ev.type) {
	 case ButtonRelease:
	    if (activeWell != NULL) {
		WMSetColorWellColor(activeWell, cPtr->color);
	    } else {
		slideView(dragView, ev.xbutton.x_root, ev.xbutton.y_root,
			  event->xmotion.x_root, event->xmotion.y_root);
	    }
	    
	    done = True;
	    break;

	 case EnterNotify:
	    view = W_GetViewForXWindow(dpy, ev.xcrossing.window);

	    if (view && view->self && W_CLASS(view->self) == WC_ColorWell
		&& view->self != activeWell && view->self != cPtr) {

		activeWell = view->self;
		XRecolorCursor(dpy, scr->defaultCursor, &green, &back);
	    } else if (view->self!=NULL && view->self != activeWell) {
		XRecolorCursor(dpy, scr->defaultCursor, &black, &back);
		activeWell = NULL;
	    }
	    break;
	    
	 case MotionNotify:
	    W_MoveView(dragView, ev.xmotion.x_root-8, ev.xmotion.y_root-8);
	    break;

	 default:
	    WMHandleEvent(&ev);
	    break;
	}
    }
    XUngrabPointer(dpy, CurrentTime);
    XRecolorCursor(dpy, scr->defaultCursor, &black, &back);

    W_DestroyView(dragView);
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

		offs.width = 2;
		offs.height = 2;
		pixmap = makeDragPixmap(cPtr);

		/*
		WMDragImageFromView(cPtr->view, pixmap, cPtr->view->pos,
				   offs, event, True);
		 * */
		
		dragColor(cPtr, event, pixmap);
		
		WMReleasePixmap(pixmap);
	    }
	}
	break;
    }
}



static void
handleActionEvents(XEvent *event, void *data)
{
/*    WMColorWell *cPtr = (ColorWell*)data;*/
    
}


static void
destroyColorWell(ColorWell *cPtr)
{
    if (cPtr->color)
	WMReleaseColor(cPtr->color);
   
    free(cPtr);
}

