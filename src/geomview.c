


#include "WINGsP.h"



typedef struct W_GeometryView {
    W_Class widgetClass;
    WMView *view;
    
    GC gc;
    WMFont *font;
    
    WMSize textSize;
    
    union {
	struct {
	    int x, y;
	} pos;
	struct {
	    unsigned width, height;
	} size;
    } data;
    
    unsigned showPosition:1;
} WGeometryView;



static void handleEvents(XEvent *event, void *clientData);
static void paint(WGeometryView *gview);


WGeometryView*
WCreateGeometryView(WMScreen *scr)
{
    WGeometryView *gview;
    char buffer[64];
    static W_Class widgetClass = 0;
    
    if (!widgetClass) {
	widgetClass = W_RegisterUserWidget();
    }
    
    gview = malloc(sizeof(WGeometryView));
    if (!gview) {
	return NULL;
    }
    memset(gview, 0, sizeof(WGeometryView));
    
    gview->widgetClass = widgetClass;

    gview->view = W_CreateTopView(scr);
    if (!gview->view) {
	free(gview);

	return NULL;
    }
    gview->view->self = gview;

    gview->font = WMSystemFontOfSize(scr, 12);
    if (!gview->font) {
	W_DestroyView(gview->view);
	free(gview);

	return NULL;
    }

    {
	WMColor *black = WMBlackColor(scr);
	WMColor *gray = WMGrayColor(scr);
	XGCValues gcv;
	
	gcv.foreground = WMColorPixel(black);
	gcv.background = WMColorPixel(gray);
	
	gview->gc = XCreateGC(WMScreenDisplay(scr), W_DRAWABLE(scr),
			      GCForeground|GCBackground, &gcv);

	WMReleaseColor(black);
	WMReleaseColor(gray);
    }

    WMCreateEventHandler(gview->view, ExposureMask, handleEvents, gview);

    sprintf(buffer, "%+05i,  %+05i", 0, 0);

    gview->textSize.width = WMWidthOfString(gview->font, buffer, 
					    strlen(buffer));
    gview->textSize.height = WMFontHeight(gview->font);
    
    W_ResizeView(gview->view, gview->textSize.width+8,
		 gview->textSize.height+6);

    return gview;
}


void
WSetGeometryViewShownPosition(WGeometryView *gview, int x, int y)
{
    gview->showPosition = 1;
    gview->data.pos.x = x;
    gview->data.pos.y = y;
    
    paint(gview);
}


void
WSetGeometryViewShownSize(WGeometryView *gview,
			  unsigned width, unsigned height)
{
    gview->showPosition = 0;
    gview->data.size.width = width;
    gview->data.size.height = height;
    
    paint(gview);
}



static void
paint(WGeometryView *gview)
{
    char buffer[64];

    if (gview->showPosition) {
	sprintf(buffer, "%+5i , %+5i    ", gview->data.pos.x, gview->data.pos.y);
    } else {
	sprintf(buffer, "%+5i x %+5i    ", gview->data.size.width, gview->data.size.height);
    }

    WMDrawImageString(W_VIEW_SCREEN(gview->view),
		      W_VIEW_DRAWABLE(gview->view),
		      gview->gc, gview->font,
		      (W_VIEW_WIDTH(gview->view)-gview->textSize.width)/2,
		      (W_VIEW_HEIGHT(gview->view)-gview->textSize.height)/2,
		      buffer, strlen(buffer));

    W_DrawRelief(W_VIEW_SCREEN(gview->view), W_VIEW_DRAWABLE(gview->view),
		 0, 0, W_VIEW_WIDTH(gview->view), W_VIEW_HEIGHT(gview->view),
		 WRRaised);
}



static void
handleEvents(XEvent *event, void *clientData)
{
    WGeometryView *gview = (WGeometryView*)clientData;
    
    switch (event->type) {
     case Expose:
	paint(gview);
	break;
	
    }
}

