
#include "WINGsP.h"


typedef struct W_Frame {
    W_Class widgetClass;
    W_View *view;

    char *caption;

    
    struct {
	WMReliefType relief:3;
	WMTitlePosition titlePosition:3;
    } flags;
} Frame;



struct W_ViewProcedureTable _FrameViewProcedures = {
	NULL,
	NULL,
	NULL
};


#define DEFAULT_RELIEF 	WRGroove
#define DEFAULT_TITLE_POSITION	WTPAtTop
#define DEFAULT_WIDTH		40
#define DEFAULT_HEIGHT		40


static void destroyFrame(Frame *fPtr);
static void paintFrame(Frame *fPtr);



void
WMSetFrameTitlePosition(WMFrame *fPtr, WMTitlePosition position)
{
    fPtr->flags.titlePosition = position;
    
   if (fPtr->view->flags.realized) {
	paintFrame(fPtr);
    }
}


void
WMSetFrameRelief(WMFrame *fPtr, WMReliefType relief)
{
    fPtr->flags.relief = relief;
    
   if (fPtr->view->flags.realized) {
	paintFrame(fPtr);
    }
}


void
WMSetFrameTitle(WMFrame *fPtr, char *title)
{
    if (fPtr->caption)
	free(fPtr->caption);
    if (title)
	fPtr->caption = wstrdup(title);
    else
	fPtr->caption = NULL;

   if (fPtr->view->flags.realized) {
	paintFrame(fPtr);
    }
}


static void
paintFrame(Frame *fPtr)
{
    W_View *view = fPtr->view;
    W_Screen *scrPtr = view->screen;
    int tx, ty, tw, th;
    int fy, fh;

    if (fPtr->caption!=NULL)
	th = WMFontHeight(scrPtr->normalFont);
    else {
	th = 0;
    }
    
    fh = view->size.height;
    fy = 0;
    
    switch (fPtr->flags.titlePosition) {
     case WTPAboveTop:
	ty = 0;
	fy = th + 4;
	fh = view->size.height - fy;
	break;
	
     case WTPAtTop:
	ty = 0;
	fy = th/2;
	fh = view->size.height - fy;
	break;
	
     case WTPBelowTop:
	ty = 4;
	fy = 0;
	fh = view->size.height;
	break;
	
     case WTPAboveBottom:
	ty = view->size.height - th - 4;
	fy = 0;
	fh = view->size.height;
	break;
	
     case WTPAtBottom:
	ty = view->size.height - th;
	fy = 0;
	fh = view->size.height - th/2;
	break;
	
     case WTPBelowBottom:
	ty = view->size.height - th;
	fy = 0;
	fh = view->size.height - th - 4;
	break;
	
     default:
	ty = 0;
	fy = 0;
	fh = view->size.height;
    }
/*
    XClearArea(scrPtr->display, view->window, fy+2, 2, fh-4, view->size.width-4,
	       False);
 */
    XClearWindow(scrPtr->display, view->window);
    
    W_DrawRelief(scrPtr, view->window, 0, fy, view->size.width, fh, 
		 fPtr->flags.relief);

    if (fPtr->caption!=NULL && fPtr->flags.titlePosition!=WTPNoTitle) {
	
	
	tw = WMWidthOfString(scrPtr->normalFont, fPtr->caption, 
			 strlen(fPtr->caption));
    	
	tx = (view->size.width - tw) / 2;
    
	XFillRectangle(scrPtr->display, view->window, W_GC(scrPtr->gray),
		       tx, ty, tw, th);

	WMDrawString(scrPtr, view->window, W_GC(scrPtr->black), 
		     scrPtr->normalFont, tx, ty, fPtr->caption,
		     strlen(fPtr->caption));
    }
}





static void
handleEvents(XEvent *event, void *data)
{
    Frame *fPtr = (Frame*)data;

    CHECK_CLASS(data, WC_Frame);

    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintFrame(fPtr);
	break;
	
     case DestroyNotify:
	destroyFrame(fPtr);
	break;
    }
}


WMFrame*
WMCreateFrame(WMWidget *parent)
{
    Frame *fPtr;
    
    fPtr = wmalloc(sizeof(Frame));
    memset(fPtr, 0, sizeof(Frame));

    fPtr->widgetClass = WC_Frame;

    fPtr->view = W_CreateView(W_VIEW(parent));
    if (!fPtr->view) {
	free(fPtr);
	return NULL;
    }
    fPtr->view->self = fPtr;
    
    WMCreateEventHandler(fPtr->view, ExposureMask|StructureNotifyMask,
			 handleEvents, fPtr);


    fPtr->flags.relief = DEFAULT_RELIEF;
    fPtr->flags.titlePosition = DEFAULT_TITLE_POSITION;

    WMResizeWidget(fPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    return fPtr;
}


static void
destroyFrame(Frame *fPtr)
{    
    if (fPtr->caption)
	free(fPtr->caption);

    free(fPtr);
}
