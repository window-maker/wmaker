



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
#if 0
static void handleDragEvents(XEvent *event, void *data);
#endif
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
    
    cPtr->colorView = W_CreateView(cPtr->view);
    if (!cPtr->colorView) {
	W_DestroyView(cPtr->view);
	free(cPtr);
	return NULL;
    }

    WMCreateEventHandler(cPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, cPtr);
    
    WMCreateEventHandler(cPtr->colorView, ExposureMask, handleEvents, cPtr);
#if 0
    WMCreateEventHandler(cPtr->colorView, ButtonPressMask|Button1MotionMask,
			 handleDragEvents, cPtr);
#endif
    WMCreateEventHandler(cPtr->view, ButtonPressMask, handleActionEvents, 
			 cPtr);

    cPtr->colorView->flags.mapWhenRealized = 1;
    
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

#define MIN(a,b)	((a) > (b) ? (b) : (a))

static void
resizeColorWell(WMColorWell *cPtr, unsigned int width, unsigned int height)
{
    int bw;
    
    if (width < MIN_WIDTH)
	width = MIN_WIDTH;
    if (height < MIN_HEIGHT)
	height = MIN_HEIGHT;

    bw = (int)((float)MIN(width, height)*0.24);
    
    W_ResizeView(cPtr->view, width, height);
    
    W_ResizeView(cPtr->colorView, width-2*bw, height-2*bw);

    if (cPtr->colorView->pos.x!=bw || cPtr->colorView->pos.y!=bw)
	W_MoveView(cPtr->colorView, bw, bw);
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

#if 0
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
		
		offs.width = 2;
		offs.height = 2;
		pixmap = makeDragPixmap(cPtr);
		
		WMDragImageFromView(cPtr->view, pixmap, cPtr->view->pos,
				   offs, event, True);
		
		WMReleasePixmap(pixmap);
	    }
	}
	break;
    }
}
#endif


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

